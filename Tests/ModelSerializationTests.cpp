#include <gtest/gtest.h>

#include "Common/Json.h"
#include "Model/Order/Order.h"
#include "Model/Order/OrderStatus.h"
#include "Model/Production/ProductionJob.h"
#include "Model/Sample/Sample.h"

TEST(JsonValueTest, RoundTripsObjectFields) {
    Common::JsonValue value = Common::JsonValue::MakeObject();
    value["name"] = std::string("SampleA");
    value["stock"] = 50;
    value["yieldRate"] = 0.9;

    Common::JsonValue parsed = Common::JsonValue::Parse(value.Dump());

    EXPECT_EQ(parsed.At("name").AsString(), "SampleA");
    EXPECT_EQ(parsed.At("stock").AsInt(), 50);
    EXPECT_DOUBLE_EQ(parsed.At("yieldRate").AsDouble(), 0.9);
}

TEST(JsonValueTest, RoundTripsNullAndArray) {
    Common::JsonValue array = Common::JsonValue::MakeArray();
    array.PushBack(Common::JsonValue(nullptr));
    array.PushBack(Common::JsonValue(std::string("x")));

    Common::JsonValue parsed = Common::JsonValue::Parse(array.Dump());

    ASSERT_EQ(parsed.AsArray().size(), 2u);
    EXPECT_TRUE(parsed.AsArray()[0].IsNull());
    EXPECT_EQ(parsed.AsArray()[1].AsString(), "x");
}

TEST(SampleTest, ToJsonFromJsonRoundTrips) {
    Model::Sample sample("S-001", "SampleA", 10.0, 0.9, 50);

    Model::Sample restored = Model::Sample::FromJson(sample.ToJson());

    EXPECT_EQ(restored.GetId(), "S-001");
    EXPECT_EQ(restored.GetName(), "SampleA");
    EXPECT_DOUBLE_EQ(restored.GetAvgProductionTime(), 10.0);
    EXPECT_DOUBLE_EQ(restored.GetYieldRate(), 0.9);
    EXPECT_EQ(restored.GetStock(), 50);
}

// --- Phase 1 §6.1: Model::Sample validation / TryCreate ---

TEST(SampleTryCreateTest, ValidValuesSucceedAndFieldsAreStoredAsGiven) {
    auto sample = Model::Sample::TryCreate("S-001", "Wafer-A", 10.0, 0.9);

    ASSERT_TRUE(sample.has_value());
    EXPECT_EQ(sample->GetId(), "S-001");
    EXPECT_EQ(sample->GetName(), "Wafer-A");
    EXPECT_DOUBLE_EQ(sample->GetAvgProductionTime(), 10.0);
    EXPECT_DOUBLE_EQ(sample->GetYieldRate(), 0.9);
}

TEST(SampleTryCreateTest, StockDefaultsToZeroOnRegistration) {
    auto sample = Model::Sample::TryCreate("S-001", "Wafer-A", 10.0, 0.9);

    ASSERT_TRUE(sample.has_value());
    EXPECT_EQ(sample->GetStock(), 0);
}

TEST(SampleTryCreateTest, YieldRateOfOneIsAllowed) {
    EXPECT_TRUE(Model::Sample::TryCreate("S-001", "Wafer-A", 10.0, 1.0).has_value());
}

TEST(SampleTryCreateTest, YieldRateOfZeroIsRejectedToAvoidDivisionByZeroLater) {
    // PRD 7.4 / Phase1 §5: actualQuantity = ceil(shortage / yieldRate) would
    // divide by zero in Phase 3 if yieldRate == 0 were allowed through here.
    EXPECT_FALSE(Model::Sample::TryCreate("S-001", "Wafer-A", 10.0, 0.0).has_value());
}

TEST(SampleTryCreateTest, YieldRateAboveOneIsRejected) {
    EXPECT_FALSE(Model::Sample::TryCreate("S-001", "Wafer-A", 10.0, 1.0001).has_value());
}

TEST(SampleTryCreateTest, NegativeYieldRateIsRejected) {
    EXPECT_FALSE(Model::Sample::TryCreate("S-001", "Wafer-A", 10.0, -0.1).has_value());
}

TEST(SampleTryCreateTest, ZeroAvgProductionTimeIsRejected) {
    EXPECT_FALSE(Model::Sample::TryCreate("S-001", "Wafer-A", 0.0, 0.9).has_value());
}

TEST(SampleTryCreateTest, NegativeAvgProductionTimeIsRejected) {
    EXPECT_FALSE(Model::Sample::TryCreate("S-001", "Wafer-A", -1.0, 0.9).has_value());
}

TEST(SampleTryCreateTest, EmptyIdIsRejected) {
    EXPECT_FALSE(Model::Sample::TryCreate("", "Wafer-A", 10.0, 0.9).has_value());
}

TEST(SampleTryCreateTest, WhitespaceOnlyIdIsRejected) {
    EXPECT_FALSE(Model::Sample::TryCreate("   ", "Wafer-A", 10.0, 0.9).has_value());
}

TEST(SampleTryCreateTest, EmptyNameIsRejected) {
    EXPECT_FALSE(Model::Sample::TryCreate("S-001", "", 10.0, 0.9).has_value());
}

TEST(SampleTryCreateTest, WhitespaceOnlyNameIsRejected) {
    EXPECT_FALSE(Model::Sample::TryCreate("S-001", "   ", 10.0, 0.9).has_value());
}

TEST(SampleValidationHelperTest, IsValidYieldRateBoundaries) {
    EXPECT_FALSE(Model::Sample::IsValidYieldRate(0.0));
    EXPECT_TRUE(Model::Sample::IsValidYieldRate(1.0));
    EXPECT_FALSE(Model::Sample::IsValidYieldRate(1.0001));
    EXPECT_FALSE(Model::Sample::IsValidYieldRate(-0.5));
}

TEST(SampleValidationHelperTest, IsValidAvgProductionTimeBoundaries) {
    EXPECT_FALSE(Model::Sample::IsValidAvgProductionTime(0.0));
    EXPECT_FALSE(Model::Sample::IsValidAvgProductionTime(-1.0));
    EXPECT_TRUE(Model::Sample::IsValidAvgProductionTime(0.0001));
}

TEST(SampleValidationHelperTest, IsValidIdAndNameRejectBlankStrings) {
    EXPECT_FALSE(Model::Sample::IsValidId(""));
    EXPECT_FALSE(Model::Sample::IsValidId("  \t "));
    EXPECT_TRUE(Model::Sample::IsValidId("S-001"));
    EXPECT_FALSE(Model::Sample::IsValidName(""));
    EXPECT_FALSE(Model::Sample::IsValidName("  \t "));
    EXPECT_TRUE(Model::Sample::IsValidName("Wafer-A"));
}

TEST(OrderTest, AllStatusValuesRoundTripThroughJsonString) {
    const Model::OrderStatus statuses[] = {Model::OrderStatus::Reserved, Model::OrderStatus::Rejected,
                                            Model::OrderStatus::Producing, Model::OrderStatus::Confirmed,
                                            Model::OrderStatus::Release};
    for (Model::OrderStatus status : statuses) {
        Model::Order order("ORD-20260416-0043", "S-001", "Acme", 50, status, "2026-04-16T10:00:00",
                            "2026-04-16T10:00:00");

        Model::Order restored = Model::Order::FromJson(order.ToJson());

        EXPECT_EQ(restored.GetStatus(), status);
        EXPECT_EQ(restored.GetOrderId(), "ORD-20260416-0043");
        EXPECT_EQ(restored.GetSampleId(), "S-001");
        EXPECT_EQ(restored.GetCustomerName(), "Acme");
        EXPECT_EQ(restored.GetQuantity(), 50);
    }
}

TEST(OrderTest, UnknownStatusStringThrows) {
    EXPECT_THROW(Model::OrderStatusFromString("UNKNOWN"), std::invalid_argument);
}

TEST(OrderTest, EmptyStatusStringThrows) {
    EXPECT_THROW(Model::OrderStatusFromString(""), std::invalid_argument);
}

TEST(OrderTest, LowercaseStatusStringIsRejected) {
    // Status strings must match the PRD 8.2 canonical uppercase spelling
    // exactly (defensive coding: lowercase variants are not silently accepted).
    EXPECT_THROW(Model::OrderStatusFromString("reserved"), std::invalid_argument);
}

TEST(OrderTest, TimestampsRoundTripThroughJson) {
    Model::Order order("ORD-20260416-0043", "S-001", "Acme", 50, Model::OrderStatus::Confirmed,
                        "2026-04-16T10:00:00", "2026-04-16T11:30:00");

    Model::Order restored = Model::Order::FromJson(order.ToJson());

    EXPECT_EQ(restored.GetCreatedAt(), "2026-04-16T10:00:00");
    EXPECT_EQ(restored.GetUpdatedAt(), "2026-04-16T11:30:00");
}

TEST(ProductionJobTest, StartedAtRoundTripsAsNullWhenQueued) {
    Model::ProductionJob queued("ORD-1", "S-001", 50, 100, 1000.0);

    Model::ProductionJob restored = Model::ProductionJob::FromJson(queued.ToJson());

    EXPECT_FALSE(restored.GetStartedAt().has_value());
    EXPECT_EQ(restored.GetOrderId(), "ORD-1");
    EXPECT_EQ(restored.GetSampleId(), "S-001");
    EXPECT_EQ(restored.GetShortage(), 50);
    EXPECT_EQ(restored.GetActualQuantity(), 100);
    EXPECT_DOUBLE_EQ(restored.GetEstimatedTime(), 1000.0);
}

TEST(ProductionJobTest, StartedAtRoundTripsAsValueWhenStarted) {
    Model::ProductionJob started("ORD-2", "S-001", 50, 100, 1000.0, std::string("2026-04-16T10:00:00"));

    Model::ProductionJob restored = Model::ProductionJob::FromJson(started.ToJson());

    ASSERT_TRUE(restored.GetStartedAt().has_value());
    EXPECT_EQ(*restored.GetStartedAt(), "2026-04-16T10:00:00");
}

TEST(ProductionJobTest, EstimatedTimeRoundTripsWithoutRoundingLoss) {
    // Guards against precision loss (e.g. truncation to int) when a
    // fractional estimatedTime is written/read via the JSON number path.
    Model::ProductionJob job("ORD-3", "S-001", 7, 34, 123.456789);

    Model::ProductionJob restored = Model::ProductionJob::FromJson(job.ToJson());

    EXPECT_DOUBLE_EQ(restored.GetEstimatedTime(), 123.456789);
}
