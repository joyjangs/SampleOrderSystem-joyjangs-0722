#pragma once

#include <optional>
#include <string>

#include "Common/Json.h"
#include "Model/Order/OrderStatus.h"

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

    static bool IsValidQuantity(int quantity);
    static bool IsValidCustomerName(const std::string& customerName);

    // Validates quantity/customerName itself and returns std::nullopt on
    // failure (never throws) — same self-validating convention as
    // Sample::TryCreate, so an Order can never be constructed in an invalid
    // state regardless of caller (OrderService, a future dummy-data
    // generator, etc.).
    static std::optional<Order> CreateReserved(std::string orderId, std::string sampleId,
                                               std::string customerName, int quantity,
                                               std::string createdAt);

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
