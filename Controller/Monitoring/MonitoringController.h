#pragma once

#include "Controller/ISubMenuController.h"
#include "Model/Monitoring/MonitoringService.h"
#include "View/Monitoring/MonitoringView.h"

namespace Controller {

// PRD 7.5 monitoring screen: pulls the two aggregates from MonitoringService
// and hands them straight to MonitoringView — no aggregation logic here.
class MonitoringController : public ISubMenuController {
public:
    MonitoringController(Model::MonitoringService& service, View::MonitoringView& view);

    void Run() override;

private:
    Model::MonitoringService& service_;
    View::MonitoringView& view_;
};

}  // namespace Controller
