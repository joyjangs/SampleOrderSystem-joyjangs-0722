#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <regex>
#include <string>
#include <vector>

#include "Model/Order.h"
#include "Model/OrderRepository.h"
#include "Model/OrderService.h"
#include "Model/Sample.h"
#include "Model/SampleRepository.h"

// Phase 2 §7.1: Model::OrderService::PlaceOrder, exercised against GMock
// doubles for both of its collaborators (SampleRepository, OrderRepository)
// so we can assert on the exact repository calls made — same pattern as
// Tests/SampleControllerTests.cpp's MockSampleRepository.
namespace {

class MockSampleRepository : public Model::SampleRepository {
public:
    MockSampleRepository() : Model::SampleRepository("data/test_mock_order_samples_never_written.json") {}

    MOCK_METHOD(std::optional<Model::Sample>, FindById, (const std::string& id), (const, override));
    MOCK_METHOD(void, Update, (const Model::Sample& sample), (override));
};

class MockOrderRepository : public Model::OrderRepository {
public:
    MockOrderRepository() : Model::OrderRepository("data/test_mock_orders_never_written.json") {}

    MOCK_METHOD(std::vector<Model::Order>, FindAll, (), (const, override));
    MOCK_METHOD(void, Add, (const Model::Order& order), (override));
};

using ::testing::_;
using ::testing::Return;

class OrderServiceTest : public ::testing::Test {
protected:
    MockSampleRepository sampleRepository;
    MockOrderRepository orderRepository;
    Model::OrderService service{sampleRepository, orderRepository};

    static Model::Sample RegisteredSample() { return Model::Sample("S-001", "Wafer-A", 10.0, 0.9, 20); }
};

TEST_F(OrderServiceTest, UnregisteredSampleIsRejectedAndOrderIsNotPersisted) {
    EXPECT_CALL(sampleRepository, FindById("S-999")).WillOnce(Return(std::nullopt));
    EXPECT_CALL(orderRepository, FindAll()).Times(0);
    EXPECT_CALL(orderRepository, Add(_)).Times(0);

    Model::PlaceOrderResult result = service.PlaceOrder("S-999", "Acme", 50);

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.order.has_value());
    EXPECT_FALSE(result.failureReason.empty());
}

TEST_F(OrderServiceTest, ZeroQuantityIsRejectedAndOrderIsNotPersisted) {
    EXPECT_CALL(sampleRepository, FindById("S-001")).WillOnce(Return(RegisteredSample()));
    EXPECT_CALL(orderRepository, Add(_)).Times(0);

    Model::PlaceOrderResult result = service.PlaceOrder("S-001", "Acme", 0);

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.order.has_value());
}

TEST_F(OrderServiceTest, NegativeQuantityIsRejectedAndOrderIsNotPersisted) {
    EXPECT_CALL(sampleRepository, FindById("S-001")).WillOnce(Return(RegisteredSample()));
    EXPECT_CALL(orderRepository, Add(_)).Times(0);

    Model::PlaceOrderResult result = service.PlaceOrder("S-001", "Acme", -10);

    EXPECT_FALSE(result.success);
}

TEST_F(OrderServiceTest, EmptyCustomerNameIsRejectedAndOrderIsNotPersisted) {
    EXPECT_CALL(sampleRepository, FindById("S-001")).WillOnce(Return(RegisteredSample()));
    EXPECT_CALL(orderRepository, Add(_)).Times(0);

    Model::PlaceOrderResult result = service.PlaceOrder("S-001", "", 50);

    EXPECT_FALSE(result.success);
}

TEST_F(OrderServiceTest, BlankCustomerNameIsRejectedAndOrderIsNotPersisted) {
    EXPECT_CALL(sampleRepository, FindById("S-001")).WillOnce(Return(RegisteredSample()));
    EXPECT_CALL(orderRepository, Add(_)).Times(0);

    Model::PlaceOrderResult result = service.PlaceOrder("S-001", "   ", 50);

    EXPECT_FALSE(result.success);
}

TEST_F(OrderServiceTest, ValidRequestCreatesReservedOrderAndPersistsItWithMatchingCreatedAndUpdatedTimestamp) {
    EXPECT_CALL(sampleRepository, FindById("S-001")).WillOnce(Return(RegisteredSample()));
    EXPECT_CALL(orderRepository, FindAll()).WillOnce(Return(std::vector<Model::Order>{}));
    EXPECT_CALL(orderRepository, Add(::testing::AllOf(
                                     ::testing::Property(&Model::Order::GetSampleId, "S-001"),
                                     ::testing::Property(&Model::Order::GetCustomerName, "Acme"),
                                     ::testing::Property(&Model::Order::GetQuantity, 50),
                                     ::testing::Property(&Model::Order::GetStatus, Model::OrderStatus::Reserved))))
        .Times(1);

    Model::PlaceOrderResult result = service.PlaceOrder("S-001", "Acme", 50);

    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.order.has_value());
    EXPECT_EQ(result.order->GetStatus(), Model::OrderStatus::Reserved);
    EXPECT_EQ(result.order->GetCreatedAt(), result.order->GetUpdatedAt());
    EXPECT_TRUE(std::regex_match(result.order->GetOrderId(), std::regex("ORD-[0-9]{8}-[0-9]{4}")));
}

TEST_F(OrderServiceTest, PlaceOrderNeverTouchesSampleStock) {
    // Phase2.md §6 / OrderService.h comment: order placement must never
    // touch Sample.stock/availableStock — that's Phase 3's responsibility.
    EXPECT_CALL(sampleRepository, FindById("S-001")).WillOnce(Return(RegisteredSample()));
    EXPECT_CALL(orderRepository, FindAll()).WillOnce(Return(std::vector<Model::Order>{}));
    EXPECT_CALL(orderRepository, Add(_)).Times(1);
    EXPECT_CALL(sampleRepository, Update(_)).Times(0);

    Model::PlaceOrderResult result = service.PlaceOrder("S-001", "Acme", 50);

    EXPECT_TRUE(result.success);
}

}  // namespace
