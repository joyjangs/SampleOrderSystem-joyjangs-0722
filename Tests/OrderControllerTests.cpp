#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include "Controller/OrderController.h"
#include "Model/Order.h"
#include "Model/OrderRepository.h"
#include "Model/OrderService.h"
#include "Model/Sample.h"
#include "Model/SampleRepository.h"
#include "View/OrderView.h"

// Phase 2 §7.2: OrderController::HandlePlaceOrder, exercised against a real
// Model::OrderService backed by GMock repository doubles and a GMock
// View::OrderView, so we can assert on the exact View calls the controller
// makes for both the success and failure paths without touching a real
// console or JSON file.
namespace {

class MockSampleRepository : public Model::SampleRepository {
public:
    MockSampleRepository() : Model::SampleRepository("data/test_mock_order_ctrl_samples_never_written.json") {}

    MOCK_METHOD(std::optional<Model::Sample>, FindById, (const std::string& id), (const, override));
};

class MockOrderRepository : public Model::OrderRepository {
public:
    MockOrderRepository() : Model::OrderRepository("data/test_mock_order_ctrl_orders_never_written.json") {}

    MOCK_METHOD(std::vector<Model::Order>, FindAll, (), (const, override));
    MOCK_METHOD(void, Add, (const Model::Order& order), (override));
};

class MockOrderView : public View::OrderView {
public:
    MOCK_METHOD(std::string, PromptSampleId, (), (const, override));
    MOCK_METHOD(std::string, PromptCustomerName, (), (const, override));
    MOCK_METHOD(bool, PromptQuantity, (int& quantity), (const, override));
    MOCK_METHOD(void, ShowOrderPlaced, (const std::string& orderId, Model::OrderStatus status), (const, override));
    MOCK_METHOD(void, ShowOrderRejectedInput, (const std::string& reason), (const, override));
};

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;

class OrderControllerTest : public ::testing::Test {
protected:
    MockSampleRepository sampleRepository;
    MockOrderRepository orderRepository;
    Model::OrderService orderService{sampleRepository, orderRepository};
    MockOrderView view;
    Controller::OrderController controller{orderService, view};

    static Model::Sample RegisteredSample() { return Model::Sample("S-001", "Wafer-A", 10.0, 0.9, 20); }
};

TEST_F(OrderControllerTest, SuccessfulPlacementShowsOrderPlacedWithReservedStatus) {
    EXPECT_CALL(view, PromptSampleId()).WillOnce(Return("S-001"));
    EXPECT_CALL(view, PromptCustomerName()).WillOnce(Return("Acme"));
    EXPECT_CALL(view, PromptQuantity(_)).WillOnce(DoAll(SetArgReferee<0>(50), Return(true)));
    EXPECT_CALL(sampleRepository, FindById("S-001")).WillOnce(Return(RegisteredSample()));
    EXPECT_CALL(orderRepository, FindAll()).WillOnce(Return(std::vector<Model::Order>{}));
    EXPECT_CALL(orderRepository, Add(_)).Times(1);
    EXPECT_CALL(view, ShowOrderPlaced(_, Model::OrderStatus::Reserved)).Times(1);
    EXPECT_CALL(view, ShowOrderRejectedInput(_)).Times(0);

    controller.Run();
}

TEST_F(OrderControllerTest, UnregisteredSampleIdShowsRejectedInputAndDoesNotPersistOrder) {
    EXPECT_CALL(view, PromptSampleId()).WillOnce(Return("S-999"));
    EXPECT_CALL(view, PromptCustomerName()).WillOnce(Return("Acme"));
    EXPECT_CALL(view, PromptQuantity(_)).WillOnce(DoAll(SetArgReferee<0>(50), Return(true)));
    EXPECT_CALL(sampleRepository, FindById("S-999")).WillOnce(Return(std::nullopt));
    EXPECT_CALL(orderRepository, Add(_)).Times(0);
    EXPECT_CALL(view, ShowOrderRejectedInput(_)).Times(1);
    EXPECT_CALL(view, ShowOrderPlaced(_, _)).Times(0);

    controller.Run();
}

TEST_F(OrderControllerTest, InvalidQuantityInputShowsRejectedInputWithoutCallingOrderService) {
    EXPECT_CALL(view, PromptSampleId()).WillOnce(Return("S-001"));
    EXPECT_CALL(view, PromptCustomerName()).WillOnce(Return("Acme"));
    EXPECT_CALL(view, PromptQuantity(_)).WillOnce(Return(false));
    EXPECT_CALL(sampleRepository, FindById(_)).Times(0);
    EXPECT_CALL(orderRepository, Add(_)).Times(0);
    EXPECT_CALL(view, ShowOrderRejectedInput(_)).Times(1);

    controller.Run();
}

TEST_F(OrderControllerTest, ZeroQuantityFromServiceValidationShowsRejectedInput) {
    EXPECT_CALL(view, PromptSampleId()).WillOnce(Return("S-001"));
    EXPECT_CALL(view, PromptCustomerName()).WillOnce(Return("Acme"));
    EXPECT_CALL(view, PromptQuantity(_)).WillOnce(DoAll(SetArgReferee<0>(0), Return(true)));
    EXPECT_CALL(sampleRepository, FindById("S-001")).WillOnce(Return(RegisteredSample()));
    EXPECT_CALL(orderRepository, Add(_)).Times(0);
    EXPECT_CALL(view, ShowOrderRejectedInput(_)).Times(1);

    controller.Run();
}

}  // namespace
