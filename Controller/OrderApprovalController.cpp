#include "Controller/OrderApprovalController.h"

#include <algorithm>

#include "Model/OrderStatus.h"

namespace Controller {

namespace {

std::vector<Model::Order> ReservedOrdersSortedByOrderId(const std::vector<Model::Order>& allOrders) {
    std::vector<Model::Order> reserved;
    for (const Model::Order& order : allOrders) {
        if (order.GetStatus() == Model::OrderStatus::Reserved) {
            reserved.push_back(order);
        }
    }
    // orderId is "ORD-YYYYMMDD-NNNN", so ascending string order is also
    // ascending receipt order — required so approval consumes stock in the
    // order orders arrived (PRD 7.4).
    std::sort(reserved.begin(), reserved.end(),
              [](const Model::Order& a, const Model::Order& b) { return a.GetOrderId() < b.GetOrderId(); });
    return reserved;
}

}  // namespace

OrderApprovalController::OrderApprovalController(Model::OrderApprovalService& approvalService,
                                                  Model::OrderRepository& orderRepository,
                                                  Model::ProductionLine& productionLine,
                                                  View::OrderApprovalView& view)
    : approvalService_(approvalService),
      orderRepository_(orderRepository),
      productionLine_(productionLine),
      view_(view) {}

void OrderApprovalController::Run() { HandleApproveOrReject(); }

void OrderApprovalController::HandleApproveOrReject() {
    std::vector<Model::Order> reserved = ReservedOrdersSortedByOrderId(orderRepository_.FindAll());
    if (reserved.empty()) {
        view_.ShowNoReservedOrders();
        return;
    }
    view_.ShowReservedOrders(reserved);

    // Always the oldest reserved order — never let the user jump ahead of it,
    // since approval must consume stock in receipt order (PRD 7.4).
    const Model::Order& selected = reserved.front();
    view_.ShowNextToProcess(selected);

    auto decision = static_cast<ApprovalDecision>(view_.PromptApproveOrReject());
    if (decision == ApprovalDecision::Approve) {
        Model::ApprovalResult result = approvalService_.Approve(selected);
        if (result.wasQueued) {
            productionLine_.Enqueue(*result.job);
            view_.ShowQueuedForProduction(selected.GetOrderId(), result.job->GetShortage(),
                                          result.job->GetActualQuantity(), result.job->GetEstimatedTime());
        } else {
            view_.ShowConfirmedImmediately(selected.GetOrderId());
        }
    } else if (decision == ApprovalDecision::Reject) {
        approvalService_.Reject(selected);
        view_.ShowRejected(selected.GetOrderId());
    } else {
        view_.ShowInvalidSelection();
    }
}

}  // namespace Controller
