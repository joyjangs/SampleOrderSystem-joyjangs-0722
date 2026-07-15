#include "Model/Order/AvailableStockCalculator.h"

#include <algorithm>

namespace Model {

int CalculateAvailableStock(const Sample& sample, const std::vector<Order>& orders) {
    int allocated = 0;
    for (const Order& order : orders) {
        if (order.GetSampleId() != sample.GetId()) {
            continue;
        }
        if (order.GetStatus() == OrderStatus::Confirmed || order.GetStatus() == OrderStatus::Producing) {
            allocated += order.GetQuantity();
        }
    }
    return std::max(sample.GetStock() - allocated, 0);
}

}  // namespace Model
