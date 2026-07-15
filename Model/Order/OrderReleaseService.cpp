#include "Model/Order/OrderReleaseService.h"

#include <stdexcept>

#include "Common/Clock.h"

namespace Model {

OrderReleaseService::OrderReleaseService(SampleRepository& sampleRepository, OrderRepository& orderRepository)
    : sampleRepository_(sampleRepository), orderRepository_(orderRepository) {}

void OrderReleaseService::Release(const Order& order) {
    if (order.GetStatus() != OrderStatus::Confirmed) {
        throw std::invalid_argument("Release() requires a CONFIRMED order");
    }
    std::optional<Sample> sample = sampleRepository_.FindById(order.GetSampleId());
    if (!sample.has_value()) {
        throw std::runtime_error("Release() called for an order whose sample no longer exists: " +
                                  order.GetSampleId());
    }

    Sample updatedSample = *sample;
    updatedSample.SetStock(updatedSample.GetStock() - order.GetQuantity());
    sampleRepository_.Update(updatedSample);

    Order released = order;
    released.SetStatus(OrderStatus::Release);
    released.SetUpdatedAt(Common::CurrentTimestampIso8601());
    orderRepository_.Update(released);
}

}  // namespace Model
