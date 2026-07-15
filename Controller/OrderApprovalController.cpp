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

    int selection = view_.PromptOrderSelection(static_cast<int>(reserved.size()));
    if (selection < 1 || selection > static_cast<int>(reserved.size())) {
        view_.ShowInvalidSelection();
        return;
    }
    const Model::Order& selected = reserved[static_cast<std::size_t>(selection - 1)];

    int decision = view_.PromptApproveOrReject();
    if (decision == 1) {
        Model::ApprovalResult result = approvalService_.Approve(selected);
        if (result.wasQueued) {
            productionLine_.Enqueue(*result.job);
            view_.ShowQueuedForProduction(selected.GetOrderId(), result.job->GetShortage(),
                                          result.job->GetActualQuantity(), result.job->GetEstimatedTime());
        } else {
            view_.ShowConfirmedImmediately(selected.GetOrderId());
        }
    } else if (decision == 2) {
        approvalService_.Reject(selected);
        view_.ShowRejected(selected.GetOrderId());
    } else {
        view_.ShowInvalidSelection();
    }
}

}  // namespace Controller
