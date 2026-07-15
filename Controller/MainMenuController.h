#pragma once

#include <functional>
#include <map>
#include <string>

#include "Controller/ISubMenuController.h"
#include "Model/MonitoringService.h"
#include "View/MainMenuView.h"

namespace Controller {

// PRD 7.1's full main menu (0=exit, 1~6=submenus), plus 7 for the
// developer-convenience dummy-data menu (not part of PRD 7.1's numbered menu).
enum class MainMenuOption {
    Exit = 0,
    SampleManagement = 1,
    OrderPlacement = 2,
    OrderApproval = 3,
    Monitoring = 4,
    ProductionLine = 5,
    Release = 6,
    DummyDataGeneration = 7,
};

// Converts a MainMenuOption to the digit string a user types for it (e.g.
// MainMenuOption::SampleManagement -> "1"). Used both to build the
// subMenuControllers map keys (main.cpp) and to compare user input here.
std::string ToMenuOptionKey(MainMenuOption option);

// Routes a menu digit to its registered submenu controller (map keyed by the
// digit string, e.g. "1" -> SampleController, "2" -> OrderController). Any
// known-but-unregistered digit ("3"~"6" until their owning Phase wires them
// up) reports "not implemented". Depends on ISubMenuController, not a
// concrete controller (DIP), so registering a new submenu never requires
// changing this class's constructor signature again. reference_wrapper (not
// a raw pointer) keeps the "always a valid controller" contract explicit.
class MainMenuController {
public:
    MainMenuController(View::MainMenuView& view, Model::MonitoringService& monitoringService,
                        std::map<std::string, std::reference_wrapper<ISubMenuController>> subMenuControllers);

    void Run();
    void HandleInput(const std::string& input);
    bool IsExitRequested() const { return isExitRequested_; }

private:
    View::MainMenuView& view_;
    Model::MonitoringService& monitoringService_;
    std::map<std::string, std::reference_wrapper<ISubMenuController>> subMenuControllers_;
    bool isExitRequested_ = false;
};

}  // namespace Controller
