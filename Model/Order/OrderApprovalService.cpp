#include "Model/Order/OrderApprovalService.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "Common/Clock.h"
#include "Model/Order/AvailableStockCalculator.h"

namespace Model {

OrderApprovalService::OrderApprovalService(SampleRepository& sampleRepository, OrderRepository& orderRepository)
    : sampleRepository_(sampleRepository), orderRepository_(orderRepository) {}

ApprovalResult OrderApprovalService::Approve(const Order& order) {
    if (order.GetStatus() != OrderStatus::Reserved) {
        throw std::invalid_argument("Approve() requires a RESERVED order");
    }
    std::optional<Sample> sample = sampleRepository_.FindById(order.GetSampleId());
    if (!sample.has_value()) {
        throw std::runtime_error("Approve() called for an order whose sample no longer exists: " +
                                 order.GetSampleId());
    }

    const int available = CalculateAvailableStock(*sample, orderRepository_.FindAll());
    const int shortage = std::max(order.GetQuantity() - available, 0);
    const std::string now = Common::CurrentTimestampIso8601();

    if (shortage == 0) {
        Order confirmed = order;
        confirmed.SetStatus(OrderStatus::Confirmed);
        confirmed.SetUpdatedAt(now);
        orderRepository_.Update(confirmed);
        return ApprovalResult{false, std::nullopt};
    }

    const int actualQuantity = static_cast<int>(std::ceil(static_cast<double>(shortage) / sample->GetYieldRate()));
    const double estimatedTime = sample->GetAvgProductionTime() * actualQuantity;
    ProductionJob job(order.GetOrderId(), order.GetSampleId(), shortage, actualQuantity, estimatedTime);

    Order producing = order;
    producing.SetStatus(OrderStatus::Producing);
    producing.SetUpdatedAt(now);
    orderRepository_.Update(producing);

    return ApprovalResult{true, job};
}

void OrderApprovalService::Reject(const Order& order) {
    if (order.GetStatus() != OrderStatus::Reserved) {
        throw std::invalid_argument("Reject() requires a RESERVED order");
    }
    Order rejected = order;
    rejected.SetStatus(OrderStatus::Rejected);
    rejected.SetUpdatedAt(Common::CurrentTimestampIso8601());
    orderRepository_.Update(rejected);
}

}  // namespace Model
