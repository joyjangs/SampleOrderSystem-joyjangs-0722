#include "Model/OrderIdGenerator.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace Model {

std::string OrderIdGenerator::GenerateNextId(const std::vector<Order>& existingOrders,
                                              const std::string& today) const {
    const std::string prefix = "ORD-" + today + "-";
    int maxSequence = 0;
    for (const Order& order : existingOrders) {
        const std::string& id = order.GetOrderId();
        if (id.compare(0, prefix.size(), prefix) == 0) {
            maxSequence = std::max(maxSequence, std::stoi(id.substr(prefix.size())));
        }
    }

    std::ostringstream out;
    out << prefix << std::setw(4) << std::setfill('0') << (maxSequence + 1);
    return out.str();
}

}  // namespace Model
