#pragma once

#include "Controller/ISubMenuController.h"
#include "Model/OrderApprovalService.h"
#include "Model/OrderRepository.h"
#include "Model/ProductionLine.h"
#include "View/OrderApprovalView.h"

namespace Controller {

// PRD 7.4 approval/rejection — a single action per menu visit (same
// single-shot pattern as OrderController): show the RESERVED list, let the
// user pick one order and a decision, then return to the main menu.
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
