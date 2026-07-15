#pragma once

#include <string>

#include "Controller/SampleController.h"
#include "View/MainMenuView.h"

namespace Controller {

// "1" (시료 관리) delegates to SampleController; "2"~"6" still report "not
// implemented" until their owning Phase fills them in.
class MainMenuController {
public:
    MainMenuController(View::MainMenuView& view, Controller::SampleController& sampleController);

    void Run();
    void HandleInput(const std::string& input);
    bool IsExitRequested() const { return isExitRequested_; }

private:
    View::MainMenuView& view_;
    Controller::SampleController& sampleController_;
    bool isExitRequested_ = false;
};

}  // namespace Controller
