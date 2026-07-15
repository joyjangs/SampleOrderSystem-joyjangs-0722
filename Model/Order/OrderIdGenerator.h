#pragma once

#include <string>
#include <vector>

#include "Model/Order/Order.h"

namespace Model {

// Produces PRD 8.2's "ORD-YYYYMMDD-NNNN" order numbers (NNNN: 4-digit,
// zero-padded, per-day sequence).
class OrderIdGenerator {
public:
    // existingOrders: the full order list (e.g. OrderRepository::FindAll())
    // used to find today's highest already-issued sequence number.
    std::string GenerateNextId(const std::vector<Order>& existingOrders, const std::string& today) const;
};

}  // namespace Model
