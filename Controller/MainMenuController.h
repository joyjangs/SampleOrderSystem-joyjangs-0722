#pragma once

#include <functional>
#include <map>
#include <string>

#include "Controller/ISubMenuController.h"
#include "View/MainMenuView.h"

namespace Controller {

// Routes a menu digit to its registered submenu controller (map keyed by the
// digit string, e.g. "1" -> SampleController, "2" -> OrderController). Any
// known-but-unregistered digit ("3"~"6" until their owning Phase wires them
// up) reports "not implemented". Depends on ISubMenuController, not a
// concrete controller (DIP), so registering a new submenu never requires
// changing this class's constructor signature again. reference_wrapper (not
// a raw pointer) keeps the "always a valid controller" contract explicit.
class MainMenuController {
public:
    MainMenuController(View::MainMenuView& view,
                        std::map<std::string, std::reference_wrapper<ISubMenuController>> subMenuControllers);

    void Run();
    void HandleInput(const std::string& input);
    bool IsExitRequested() const { return isExitRequested_; }

private:
    View::MainMenuView& view_;
    std::map<std::string, std::reference_wrapper<ISubMenuController>> subMenuControllers_;
    bool isExitRequested_ = false;
};

}  // namespace Controller
