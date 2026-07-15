#include "Controller/MonitoringController.h"

namespace Controller {

MonitoringController::MonitoringController(Model::MonitoringService& service, View::MonitoringView& view)
    : service_(service), view_(view) {}

void MonitoringController::Run() {
    view_.PrintOrderStatusSummary(service_.GetOrderStatusSummary());
    view_.PrintSampleStockStatus(service_.GetSampleStockViews());
}

}  // namespace Controller
