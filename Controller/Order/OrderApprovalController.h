#pragma once

#include "Controller/ISubMenuController.h"
#include "Model/Order/OrderApprovalService.h"
#include "Model/Order/OrderRepository.h"
#include "Model/Production/ProductionLine.h"
#include "View/Order/OrderApprovalView.h"

namespace Controller {

// The digit the user types at View::OrderApprovalView::PromptApproveOrReject().
enum class ApprovalDecision {
    Approve = 1,
    Reject = 2,
};

// PRD 7.4 approval/rejection — a single action per menu visit (same
// single-shot pattern as OrderController): show the RESERVED list for
// context, but always act on the oldest (first-received) reserved order.
// Approval must consume stock in receipt order (CLAUDE.md/PRD 7.4), so the
// UI never lets the user pick an arbitrary order out of sequence.
class OrderApprovalController : public ISubMenuController {
public:
    OrderApprovalController(Model::OrderApprovalService& approvalService, Model::OrderRepository& orderRepository,
                            Model::ProductionLine& productionLine, View::OrderApprovalView& view);

    void Run() override;

private:
    void HandleApproveOrReject();

    Model::OrderApprovalService& approvalService_;
    Model::OrderRepository& orderRepository_;
    Model::ProductionLine& productionLine_;
    View::OrderApprovalView& view_;
};

}  // namespace Controller
