#include "Model/OrderStatus.h"

#include <stdexcept>

namespace Model {

std::string ToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::Reserved: return "RESERVED";
        case OrderStatus::Rejected: return "REJECTED";
        case OrderStatus::Producing: return "PRODUCING";
        case OrderStatus::Confirmed: return "CONFIRMED";
        case OrderStatus::Release: return "RELEASE";
    }
    throw std::invalid_argument("Unknown OrderStatus value");
}

OrderStatus OrderStatusFromString(const std::string& text) {
    if (text == "RESERVED") return OrderStatus::Reserved;
    if (text == "REJECTED") return OrderStatus::Rejected;
    if (text == "PRODUCING") return OrderStatus::Producing;
    if (text == "CONFIRMED") return OrderStatus::Confirmed;
    if (text == "RELEASE") return OrderStatus::Release;
    throw std::invalid_argument("Unknown order status string: " + text);
}

}  // namespace Model
