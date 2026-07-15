#pragma once

#include <string>

namespace Model {

enum class OrderStatus { Reserved, Rejected, Producing, Confirmed, Release };

std::string ToString(OrderStatus status);
OrderStatus OrderStatusFromString(const std::string& text);

}  // namespace Model
