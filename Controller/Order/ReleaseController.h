#pragma once

#include "Controller/ISubMenuController.h"
#include "Model/Order/OrderReleaseService.h"
#include "Model/Order/OrderRepository.h"
#include "View/Order/ReleaseView.h"

namespace Controller {

// PRD 7.7 shipment/release — unlike OrderApprovalController's forced
// "always the oldest" single-shot pattern, release has no ordering
// constraint (releasing one CONFIRMED order never changes another's
// available-stock calculation — that calculation only cares about
// CONFIRMED/PRODUCING quantities, not release order), so the user freely
// picks any CONFIRMED order by list number (PRD 10 "번호 선택").
class ReleaseController : public ISubMenuController {
public:
    ReleaseController(Model::OrderReleaseService& releaseService, Model::OrderRepository& orderRepository,
                       View::ReleaseView& view);

    void Run() override;

private:
    void HandleRelease();

    Model::OrderReleaseService& releaseService_;
    Model::OrderRepository& orderRepository_;
    View::ReleaseView& view_;
};

}  // namespace Controller
