#pragma once

#include <string>

#include "Controller/ISubMenuController.h"
#include "View/MainMenuView.h"

namespace Controller {

// "1" (시료 관리) delegates to the injected submenu controller; "2"~"6"
// still report "not implemented" until their owning Phase fills them in.
// Depends on ISubMenuController, not a concrete controller (DIP), so later
// Phases can swap it without changing this class.
class MainMenuController {
public:
    MainMenuController(View::MainMenuView& view, Controller::ISubMenuController& sampleController);

    void Run();
    void HandleInput(const std::string& input);
    bool IsExitRequested() const { return isExitRequested_; }

private:
    View::MainMenuView& view_;
    Controller::ISubMenuController& sampleController_;
    bool isExitRequested_ = false;
};

}  // namespace Controller
