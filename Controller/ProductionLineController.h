#pragma once

#include "Controller/ISubMenuController.h"
#include "Model/OrderRepository.h"
#include "Model/ProductionLine.h"
#include "Model/SampleRepository.h"
#include "View/ProductionLineView.h"

namespace Controller {

// PRD 7.6 production line: a repeating submenu (like SampleController) —
// "새로고침" re-shows the current job/queue with progress recomputed for
// "now", "생산 완료 처리" completes the front job and reflects it into
// Sample.stock / the Order's status.
class ProductionLineController : public ISubMenuController {
public:
    ProductionLineController(Model::ProductionLine& productionLine, Model::SampleRepository& sampleRepository,
                             Model::OrderRepository& orderRepository, View::ProductionLineView& view);

    void Run() override;

private:
    void ShowStatus() const;
    void HandleCompleteCurrentJob();

    Model::ProductionLine& productionLine_;
    Model::SampleRepository& sampleRepository_;
    Model::OrderRepository& orderRepository_;
    View::ProductionLineView& view_;
};

}  // namespace Controller
