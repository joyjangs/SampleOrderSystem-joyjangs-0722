#include "Model/OrderService.h"

#include "Common/Clock.h"
#include "Model/OrderIdGenerator.h"

namespace Model {

OrderService::OrderService(SampleRepository& sampleRepository, OrderRepository& orderRepository)
    : sampleRepository_(sampleRepository), orderRepository_(orderRepository) {}

PlaceOrderResult OrderService::PlaceOrder(const std::string& sampleId, const std::string& customerName,
                                           int quantity) {
    if (!sampleRepository_.FindById(sampleId).has_value()) {
        return PlaceOrderResult{false, std::nullopt, "등록되지 않은 시료 ID입니다: " + sampleId};
    }
    if (!Order::IsValidQuantity(quantity)) {
        return PlaceOrderResult{false, std::nullopt, "수량은 1 이상이어야 합니다."};
    }
    if (!Order::IsValidCustomerName(customerName)) {
        return PlaceOrderResult{false, std::nullopt, "고객명은 비어있을 수 없습니다."};
    }

    const OrderIdGenerator idGenerator;
    const std::string today = Common::CurrentDateYyyymmdd();
    const std::string orderId = idGenerator.GenerateNextId(orderRepository_.FindAll(), today);
    const std::string createdAt = Common::CurrentTimestampIso8601();

    Order order = Order::CreateReserved(orderId, sampleId, customerName, quantity, createdAt);
    orderRepository_.Add(order);

    return PlaceOrderResult{true, order, ""};
}

}  // namespace Model
