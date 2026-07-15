#include "Controller/DummyData/DummyDataController.h"

namespace Controller {

DummyDataController::DummyDataController(Model::DummyDataGenerator& generator, View::DummyDataView& view)
    : generator_(generator), view_(view) {}

void DummyDataController::Run() {
    Model::DummyDataOptions options;
    view_.PromptOptions(options);

    if (options.sampleCount < 0 || options.orderCount < 0) {
        view_.ShowInvalidCount();
        return;
    }

    Model::DummyDataResult result = generator_.Generate(options);
    view_.ShowResult(result);
}

}  // namespace Controller
