#include "Model/Order/OrderIdGenerator.h"

#include <algorithm>
#include <charconv>
#include <iomanip>
#include <sstream>

namespace Model {

std::string OrderIdGenerator::GenerateNextId(const std::vector<Order>& existingOrders,
                                              const std::string& today) const {
    const std::string prefix = "ORD-" + today + "-";
    int maxSequence = 0;
    for (const Order& order : existingOrders) {
        const std::string& id = order.GetOrderId();
        if (id.compare(0, prefix.size(), prefix) != 0) {
            continue;
        }
        // Defends against hand-edited/corrupted data/orders.json: a
        // non-numeric suffix is skipped instead of crashing the app.
        const std::string suffix = id.substr(prefix.size());
        int sequence = 0;
        auto result = std::from_chars(suffix.data(), suffix.data() + suffix.size(), sequence);
        if (result.ec == std::errc{}) {
            maxSequence = std::max(maxSequence, sequence);
        }
    }

    std::ostringstream out;
    out << prefix << std::setw(4) << std::setfill('0') << (maxSequence + 1);
    return out.str();
}

}  // namespace Model
