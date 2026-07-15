#pragma once

#include <string>

#include "View/MainMenuView.h"

namespace Controller {

// Phase 0 scope: the exit path ("0") is fully functional; every other menu
// item just reports "not implemented" until its owning Phase fills it in.
class MainMenuController {
public:
    explicit MainMenuController(View::MainMenuView& view);

    void Run();
    void HandleInput(const std::string& input);
    bool IsExitRequested() const { return isExitRequested_; }

private:
    View::MainMenuView& view_;
    bool isExitRequested_ = false;
};

}  // namespace Controller
