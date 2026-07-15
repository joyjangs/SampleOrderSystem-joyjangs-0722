#include "Controller/ProductionLineController.h"

#include "Common/Clock.h"
#include "Model/OrderStatus.h"

namespace Controller {

ProductionLineController::ProductionLineController(Model::ProductionLine& productionLine,
                                                      Model::SampleRepository& sampleRepository,
                                                      Model::OrderRepository& orderRepository,
                                                      View::ProductionLineView& view)
    : productionLine_(productionLine),
      sampleRepository_(sampleRepository),
      orderRepository_(orderRepository),
      view_(view) {}

void ProductionLineController::Run() {
    while (true) {
        TryAutoCompleteFrontJob();
        ShowStatus();
        view_.ShowMenu();
        auto option = static_cast<ProductionLineMenuOption>(view_.PromptMenuChoice());
        if (option == ProductionLineMenuOption::Exit) {
            return;
        }
        if (option == ProductionLineMenuOption::Refresh) {
            continue;  // "새로고침": loop back, re-checking completion with a fresh "now".
        }
        view_.ShowInvalidSelection();
    }
}

void ProductionLineController::ShowStatus() const {
    const std::string now = Common::CurrentTimestampIso8601();
    view_.ShowCurrentJob(productionLine_.Peek(), productionLine_.PeekProgress(now));
    view_.ShowQueue(productionLine_.ListQueue());
}

void ProductionLineController::TryAutoCompleteFrontJob() {
    const std::string now = Common::CurrentTimestampIso8601();
    std::optional<Model::ProductionJob> completed = productionLine_.CompleteFrontJobIfDue(now);
    if (!completed.has_value()) {
        return;
    }
    ApplyCompletion(*completed);
    view_.ShowJobCompleted(completed->GetOrderId(), completed->GetActualQuantity());
}

void ProductionLineController::ApplyCompletion(const Model::ProductionJob& completed) {
    std::optional<Model::Sample> sample = sampleRepository_.FindById(completed.GetSampleId());
    if (sample.has_value()) {
        Model::Sample updatedSample = *sample;
        updatedSample.SetStock(updatedSample.GetStock() + completed.GetActualQuantity());
        sampleRepository_.Update(updatedSample);
    }

    std::optional<Model::Order> order = orderRepository_.FindById(completed.GetOrderId());
    if (order.has_value()) {
        Model::Order confirmed = *order;
        confirmed.SetStatus(Model::OrderStatus::Confirmed);
        confirmed.SetUpdatedAt(Common::CurrentTimestampIso8601());
        orderRepository_.Update(confirmed);
    }
}

}  // namespace Controller
