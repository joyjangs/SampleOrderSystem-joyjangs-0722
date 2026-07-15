#include "Model/Order/Order.h"

#include "Common/StringUtils.h"

namespace Model {

Order::Order(std::string orderId, std::string sampleId, std::string customerName, int quantity,
             OrderStatus status, std::string createdAt, std::string updatedAt)
    : orderId_(std::move(orderId)),
      sampleId_(std::move(sampleId)),
      customerName_(std::move(customerName)),
      quantity_(quantity),
      status_(status),
      createdAt_(std::move(createdAt)),
      updatedAt_(std::move(updatedAt)) {}

Common::JsonValue Order::ToJson() const {
    Common::JsonValue json = Common::JsonValue::MakeObject();
    json["orderId"] = orderId_;
    json["sampleId"] = sampleId_;
    json["customerName"] = customerName_;
    json["quantity"] = quantity_;
    json["status"] = ToString(status_);
    json["createdAt"] = createdAt_;
    json["updatedAt"] = updatedAt_;
    return json;
}

Order Order::FromJson(const Common::JsonValue& json) {
    return Order(json.At("orderId").AsString(), json.At("sampleId").AsString(),
                 json.At("customerName").AsString(), json.At("quantity").AsInt(),
                 OrderStatusFromString(json.At("status").AsString()), json.At("createdAt").AsString(),
                 json.At("updatedAt").AsString());
}

bool Order::IsValidQuantity(int quantity) { return quantity >= 1; }

bool Order::IsValidCustomerName(const std::string& customerName) { return !Common::IsBlank(customerName); }

std::optional<Order> Order::CreateReserved(std::string orderId, std::string sampleId,
                                           std::string customerName, int quantity,
                                           std::string createdAt) {
    if (!IsValidQuantity(quantity) || !IsValidCustomerName(customerName)) {
        return std::nullopt;
    }
    std::string updatedAt = createdAt;
    return Order(std::move(orderId), std::move(sampleId), std::move(customerName), quantity,
                 OrderStatus::Reserved, std::move(createdAt), std::move(updatedAt));
}

}  // namespace Model
