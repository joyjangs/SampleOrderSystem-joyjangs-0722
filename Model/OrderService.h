#pragma once

#include <optional>
#include <string>

#include "Model/Order.h"
#include "Model/OrderRepository.h"
#include "Model/SampleRepository.h"

namespace Model {

struct PlaceOrderResult {
    bool success = false;
    std::optional<Order> order;   // valid only when success == true
    std::string failureReason;    // valid only when success == false
};

// Use-case layer for PRD 7.3 (order placement): validates the request,
// checks the sample is registered, generates the order number, and
// persists the RESERVED order. Never touches Sample.stock/availableStock —
// that's Phase 3's responsibility.
class OrderService {
public:
    OrderService(SampleRepository& sampleRepository, OrderRepository& orderRepository);

    PlaceOrderResult PlaceOrder(const std::string& sampleId, const std::string& customerName, int quantity);

private:
    SampleRepository& sampleRepository_;
    OrderRepository& orderRepository_;
};

}  // namespace Model
