#pragma once

#include "Controller/ISubMenuController.h"
#include "Model/DummyDataGenerator.h"
#include "View/DummyDataView.h"

namespace Controller {

// Developer-convenience submenu (not part of PRD 7.1's numbered menu):
// prompts for counts/seed, delegates generation to Model::DummyDataGenerator,
// and hands the result straight to View::DummyDataView — no generation logic
// here.
class DummyDataController : public ISubMenuController {
public:
    DummyDataController(Model::DummyDataGenerator& generator, View::DummyDataView& view);

    void Run() override;

private:
    Model::DummyDataGenerator& generator_;
    View::DummyDataView& view_;
};

}  // namespace Controller
