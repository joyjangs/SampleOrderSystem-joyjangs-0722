#pragma once

#include <string>

#include "Common/Json.h"
#include "Model/OrderStatus.h"

namespace Model {

// Order is a plain data container in Phase 0 — status transition rules
// (approve/reject/production-complete/release) belong to Phase 2/3/4.
class Order {
public:
    Order() = default;
    Order(std::string orderId, std::string sampleId, std::string customerName, int quantity,
          OrderStatus status, std::string createdAt, std::string updatedAt);

    // GetId() is the Repository-facing key accessor (see JsonFileRepository);
    // GetOrderId() is the domain-facing name for the same field.
    const std::string& GetId() const { return orderId_; }
    const std::string& GetOrderId() const { return orderId_; }
    const std::string& GetSampleId() const { return sampleId_; }
    const std::string& GetCustomerName() const { return customerName_; }
    int GetQuantity() const { return quantity_; }
    OrderStatus GetStatus() const { return status_; }
    const std::string& GetCreatedAt() const { return createdAt_; }
    const std::string& GetUpdatedAt() const { return updatedAt_; }

    void SetStatus(OrderStatus status) { status_ = status; }
    void SetUpdatedAt(std::string updatedAt) { updatedAt_ = std::move(updatedAt); }

    Common::JsonValue ToJson() const;
    static Order FromJson(const Common::JsonValue& json);

private:
    std::string orderId_;
    std::string sampleId_;
    std::string customerName_;
    int quantity_ = 0;
    OrderStatus status_ = OrderStatus::Reserved;
    std::string createdAt_;
    std::string updatedAt_;
};

}  // namespace Model
