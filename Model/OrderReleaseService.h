#pragma once

#include "Model/Order.h"
#include "Model/OrderRepository.h"
#include "Model/SampleRepository.h"

namespace Model {

// PRD 7.7's release use-case: validates the order is CONFIRMED, deducts the
// physical Sample::stock by order.quantity (the symmetric counterpart to
// ProductionLineController's stock += actualQuantity on production
// completion — see AvailableStockCalculator.h's "RELEASE orders are already
// deducted from physical stock" precondition), and persists the Order's new
// RELEASE status. Same throw-on-contract-violation convention as
// OrderApprovalService::Approve/Reject.
class OrderReleaseService {
public:
    OrderReleaseService(SampleRepository& sampleRepository, OrderRepository& orderRepository);

    void Release(const Order& order);

private:
    SampleRepository& sampleRepository_;
    OrderRepository& orderRepository_;
};

}  // namespace Model
