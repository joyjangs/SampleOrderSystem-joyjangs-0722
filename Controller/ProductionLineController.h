#pragma once

#include "Controller/ISubMenuController.h"
#include "Model/OrderRepository.h"
#include "Model/ProductionLine.h"
#include "Model/SampleRepository.h"
#include "View/ProductionLineView.h"

namespace Controller {

// The digit the user types at View::ProductionLineView::PromptMenuChoice().
// There is no menu option for manual completion — see the class comment.
enum class ProductionLineMenuOption {
    Exit = 0,
    Refresh = 1,
};

// PRD 7.6 production line: a repeating submenu (like SampleController) —
// every entry/"새로고침" re-checks whether the front job's elapsed time has
// reached its estimate (Model::ProductionLine::CompleteFrontJobIfDue) and,
// if so, automatically reflects it into Sample.stock / the Order's status
// before showing the (possibly now-updated) current job/queue. There is no
// manual "생산 완료 처리" — completion is always time-driven.
class ProductionLineController : public ISubMenuController {
public:
    ProductionLineController(Model::ProductionLine& productionLine, Model::SampleRepository& sampleRepository,
                             Model::OrderRepository& orderRepository, View::ProductionLineView& view);

    void Run() override;

private:
    void ShowStatus() const;
    void TryAutoCompleteFrontJob();
    void ApplyCompletion(const Model::ProductionJob& completed);

    Model::ProductionLine& productionLine_;
    Model::SampleRepository& sampleRepository_;
    Model::OrderRepository& orderRepository_;
    View::ProductionLineView& view_;
};

}  // namespace Controller
