#include "Controller/Order/OrderController.h"

namespace Controller {

OrderController::OrderController(Model::OrderService& orderService, View::OrderView& view)
    : orderService_(orderService), view_(view) {}

void OrderController::Run() { HandlePlaceOrder(); }

void OrderController::HandlePlaceOrder() {
    std::string sampleId = view_.PromptSampleId();
    std::string customerName = view_.PromptCustomerName();

    int quantity = 0;
    if (!view_.PromptQuantity(quantity)) {
        view_.ShowOrderRejectedInput("수량 입력 형식이 올바르지 않습니다.");
        return;
    }

    Model::PlaceOrderResult result = orderService_.PlaceOrder(sampleId, customerName, quantity);
    if (!result.success) {
        view_.ShowOrderRejectedInput(result.failureReason);
        return;
    }
    view_.ShowOrderPlaced(result.order->GetOrderId(), result.order->GetStatus());
}

}  // namespace Controller
