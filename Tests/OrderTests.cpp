#include <gtest/gtest.h>

#include <regex>
#include <vector>

#include "Model/Order.h"
#include "Model/OrderIdGenerator.h"

// Phase 2 §7.1: Model::Order validation helpers and the CreateReserved
// factory (Order.h: "std::optional<Order> CreateReserved(...)" — never
// throws, mirrors Sample::TryCreate's std::nullopt-on-failure convention).
namespace {

TEST(OrderIsValidQuantityTest, OneIsValid) { EXPECT_TRUE(Model::Order::IsValidQuantity(1)); }

TEST(OrderIsValidQuantityTest, ZeroIsRejected) { EXPECT_FALSE(Model::Order::IsValidQuantity(0)); }

TEST(OrderIsValidQuantityTest, NegativeIsRejected) { EXPECT_FALSE(Model::Order::IsValidQuantity(-1)); }

TEST(OrderIsValidQuantityTest, LargeQuantityHasNoUpperBound) {
    // Phase2.md §6 confirms: no upper bound on order quantity.
    EXPECT_TRUE(Model::Order::IsValidQuantity(1000000));
}

TEST(OrderIsValidCustomerNameTest, EmptyStringIsRejected) { EXPECT_FALSE(Model::Order::IsValidCustomerName("")); }

TEST(OrderIsValidCustomerNameTest, WhitespaceOnlyIsRejected) {
    EXPECT_FALSE(Model::Order::IsValidCustomerName("   \t "));
}

TEST(OrderIsValidCustomerNameTest, NonBlankNameIsValid) { EXPECT_TRUE(Model::Order::IsValidCustomerName("Acme")); }

TEST(OrderCreateReservedTest, ValidInputsProduceReservedOrderWithMatchingCreatedAndUpdatedTimestamp) {
    auto order = Model::Order::CreateReserved("ORD-20260416-0001", "S-001", "Acme", 50, "2026-04-16T10:00:00");

    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->GetOrderId(), "ORD-20260416-0001");
    EXPECT_EQ(order->GetSampleId(), "S-001");
    EXPECT_EQ(order->GetCustomerName(), "Acme");
    EXPECT_EQ(order->GetQuantity(), 50);
    EXPECT_EQ(order->GetStatus(), Model::OrderStatus::Reserved);
    EXPECT_EQ(order->GetCreatedAt(), "2026-04-16T10:00:00");
    EXPECT_EQ(order->GetUpdatedAt(), order->GetCreatedAt());
}

TEST(OrderCreateReservedTest, ZeroQuantityFailsAndReturnsNullopt) {
    auto order = Model::Order::CreateReserved("ORD-20260416-0001", "S-001", "Acme", 0, "2026-04-16T10:00:00");
    EXPECT_FALSE(order.has_value());
}

TEST(OrderCreateReservedTest, NegativeQuantityFailsAndReturnsNullopt) {
    auto order = Model::Order::CreateReserved("ORD-20260416-0001", "S-001", "Acme", -5, "2026-04-16T10:00:00");
    EXPECT_FALSE(order.has_value());
}

TEST(OrderCreateReservedTest, BlankCustomerNameFailsAndReturnsNullopt) {
    auto order = Model::Order::CreateReserved("ORD-20260416-0001", "S-001", "   ", 50, "2026-04-16T10:00:00");
    EXPECT_FALSE(order.has_value());
}

TEST(OrderCreateReservedTest, EmptyCustomerNameFailsAndReturnsNullopt) {
    auto order = Model::Order::CreateReserved("ORD-20260416-0001", "S-001", "", 50, "2026-04-16T10:00:00");
    EXPECT_FALSE(order.has_value());
}

// --- Phase 2 §7.1: Model::OrderIdGenerator ---

TEST(OrderIdGeneratorTest, EmptyOrderListStartsAtSequenceOne) {
    Model::OrderIdGenerator generator;
    std::string id = generator.GenerateNextId({}, "20260416");
    EXPECT_EQ(id, "ORD-20260416-0001");
}

TEST(OrderIdGeneratorTest, ExistingOrderOnSameDayIncrementsSequence) {
    Model::OrderIdGenerator generator;
    std::vector<Model::Order> existing = {
        Model::Order("ORD-20260416-0004", "S-001", "Acme", 10, Model::OrderStatus::Reserved,
                     "2026-04-16T10:00:00", "2026-04-16T10:00:00"),
    };

    std::string id = generator.GenerateNextId(existing, "20260416");

    EXPECT_EQ(id, "ORD-20260416-0005");
}

TEST(OrderIdGeneratorTest, TakesTheMaxSequenceAmongMultipleSameDayOrdersRegardlessOfListOrder) {
    Model::OrderIdGenerator generator;
    std::vector<Model::Order> existing = {
        Model::Order("ORD-20260416-0002", "S-001", "Acme", 10, Model::OrderStatus::Reserved,
                     "2026-04-16T10:00:00", "2026-04-16T10:00:00"),
        Model::Order("ORD-20260416-0007", "S-001", "Globex", 20, Model::OrderStatus::Reserved,
                     "2026-04-16T10:05:00", "2026-04-16T10:05:00"),
        Model::Order("ORD-20260416-0003", "S-001", "Initech", 30, Model::OrderStatus::Reserved,
                     "2026-04-16T10:10:00", "2026-04-16T10:10:00"),
    };

    std::string id = generator.GenerateNextId(existing, "20260416");

    EXPECT_EQ(id, "ORD-20260416-0008");
}

TEST(OrderIdGeneratorTest, DifferentDateOrdersAreIgnoredAndSequenceRestartsAtOne) {
    Model::OrderIdGenerator generator;
    std::vector<Model::Order> existing = {
        Model::Order("ORD-20260415-0099", "S-001", "Acme", 10, Model::OrderStatus::Reserved,
                     "2026-04-15T10:00:00", "2026-04-15T10:00:00"),
    };

    std::string id = generator.GenerateNextId(existing, "20260416");

    EXPECT_EQ(id, "ORD-20260416-0001");
}

TEST(OrderIdGeneratorTest, SequenceWrapsPastFourDigitsWithoutTruncatingWidth) {
    Model::OrderIdGenerator generator;
    std::vector<Model::Order> existing = {
        Model::Order("ORD-20260416-9999", "S-001", "Acme", 10, Model::OrderStatus::Reserved,
                     "2026-04-16T10:00:00", "2026-04-16T10:00:00"),
    };

    std::string id = generator.GenerateNextId(existing, "20260416");

    EXPECT_EQ(id, "ORD-20260416-10000");
}

TEST(OrderIdGeneratorTest, CorruptedNonNumericSuffixIsSkippedWithoutCrashing) {
    // Defensive std::from_chars parsing (OrderIdGenerator.cpp comment):
    // a hand-edited/corrupted orders.json entry with a non-numeric suffix
    // must not crash id generation, and must not count toward maxSequence.
    Model::OrderIdGenerator generator;
    std::vector<Model::Order> existing = {
        Model::Order("ORD-20260416-abcd", "S-001", "Acme", 10, Model::OrderStatus::Reserved,
                     "2026-04-16T10:00:00", "2026-04-16T10:00:00"),
        Model::Order("ORD-20260416-0002", "S-001", "Globex", 20, Model::OrderStatus::Reserved,
                     "2026-04-16T10:05:00", "2026-04-16T10:05:00"),
    };

    std::string id = generator.GenerateNextId(existing, "20260416");

    EXPECT_EQ(id, "ORD-20260416-0003");
}

TEST(OrderIdGeneratorTest, CorruptedSuffixOnlyEntriesStillStartAtOne) {
    Model::OrderIdGenerator generator;
    std::vector<Model::Order> existing = {
        Model::Order("ORD-20260416-xxxx", "S-001", "Acme", 10, Model::OrderStatus::Reserved,
                     "2026-04-16T10:00:00", "2026-04-16T10:00:00"),
    };

    std::string id = generator.GenerateNextId(existing, "20260416");

    EXPECT_EQ(id, "ORD-20260416-0001");
}

TEST(OrderIdGeneratorTest, OrdersWithMismatchedPrefixAreIgnoredEvenIfSuffixLooksLikeToday) {
    // An id belonging to a different (or malformed) date prefix must never be
    // mistaken for "today"'s sequence.
    Model::OrderIdGenerator generator;
    std::vector<Model::Order> existing = {
        Model::Order("ORD-2026041-0099", "S-001", "Acme", 10, Model::OrderStatus::Reserved,
                     "2026-04-16T10:00:00", "2026-04-16T10:00:00"),
    };

    std::string id = generator.GenerateNextId(existing, "20260416");

    EXPECT_EQ(id, "ORD-20260416-0001");
}

}  // namespace
