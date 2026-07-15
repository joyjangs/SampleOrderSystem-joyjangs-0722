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
        ShowStatus();
        view_.ShowMenu();
        int choice = view_.PromptMenuChoice();
        if (choice == 0) {
            return;
        }
        if (choice == 1) {
            continue;  // "새로고침": loop back to ShowStatus with a fresh "now".
        }
        if (choice == 2) {
            HandleCompleteCurrentJob();
            continue;
        }
        view_.ShowInvalidSelection();
    }
}

void ProductionLineController::ShowStatus() const {
    const std::string now = Common::CurrentTimestampIso8601();
    view_.ShowCurrentJob(productionLine_.Peek(), productionLine_.PeekProgress(now));
    view_.ShowQueue(productionLine_.ListQueue());
}

void ProductionLineController::HandleCompleteCurrentJob() {
    std::optional<Model::ProductionJob> completed = productionLine_.CompleteFrontJob();
    if (!completed.has_value()) {
        view_.ShowNoJobToComplete();
        return;
    }

    std::optional<Model::Sample> sample = sampleRepository_.FindById(completed->GetSampleId());
    if (sample.has_value()) {
        Model::Sample updatedSample = *sample;
        updatedSample.SetStock(updatedSample.GetStock() + completed->GetActualQuantity());
        sampleRepository_.Update(updatedSample);
    }

    std::optional<Model::Order> order = orderRepository_.FindById(completed->GetOrderId());
    if (order.has_value()) {
        Model::Order confirmed = *order;
        confirmed.SetStatus(Model::OrderStatus::Confirmed);
        confirmed.SetUpdatedAt(Common::CurrentTimestampIso8601());
        orderRepository_.Update(confirmed);
    }

    view_.ShowJobCompleted(completed->GetOrderId(), completed->GetActualQuantity());
}

}  // namespace Controller
