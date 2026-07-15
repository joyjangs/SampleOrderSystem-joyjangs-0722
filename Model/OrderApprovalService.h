#pragma once

#include <optional>

#include "Model/Order.h"
#include "Model/OrderRepository.h"
#include "Model/ProductionJob.h"
#include "Model/SampleRepository.h"

namespace Model {

struct ApprovalResult {
    bool wasQueued = false;             // true -> PRODUCING (queued), false -> CONFIRMED
    std::optional<ProductionJob> job;   // set only when wasQueued == true
};

// PRD 7.4's approval use-case: computes the approval-time available stock
// (AvailableStockCalculator, using the sample/orders it looks up itself via
// the injected repositories), decides CONFIRMED vs PRODUCING, and persists
// the Order's new status via OrderRepository::Update — Order has no
// transition methods of its own (Phase 0), so this composes
// SetStatus/SetUpdatedAt on a copy. Never touches Sample::stock; that only
// changes on production completion (ProductionLine) or release (Phase 4).
//
// Approve()/Reject() require order.GetStatus() == Reserved and throw
// std::invalid_argument otherwise — unlike PlaceOrder's user-input
// validation (which fails gracefully via a result value), calling these on
// an already-processed order is a caller contract violation: the
// Controller only ever offers RESERVED orders for approval/rejection.
class OrderApprovalService {
public:
    OrderApprovalService(SampleRepository& sampleRepository, OrderRepository& orderRepository);

    ApprovalResult Approve(const Order& order);
    void Reject(const Order& order);

private:
    SampleRepository& sampleRepository_;
    OrderRepository& orderRepository_;
};

}  // namespace Model
