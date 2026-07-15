#include "Controller/MainMenuController.h"

#include <iostream>
#include <string>

namespace Controller {

namespace {
// PRD 7.1's full menu (1~6); options not present in subMenuControllers_ yet
// report "not implemented" instead of "invalid selection".
constexpr MainMenuOption kAllMenuOptions[] = {
    MainMenuOption::SampleManagement, MainMenuOption::OrderPlacement, MainMenuOption::OrderApproval,
    MainMenuOption::Monitoring,       MainMenuOption::ProductionLine, MainMenuOption::Release,
};
}  // namespace

std::string ToMenuOptionKey(MainMenuOption option) { return std::to_string(static_cast<int>(option)); }

MainMenuController::MainMenuController(
    View::MainMenuView& view, std::map<std::string, std::reference_wrapper<ISubMenuController>> subMenuControllers)
    : view_(view), subMenuControllers_(std::move(subMenuControllers)) {}

void MainMenuController::Run() {
    while (!isExitRequested_) {
        view_.ShowMainMenu();
        std::string input;
        if (!std::getline(std::cin, input)) {
            break;
        }
        HandleInput(input);
    }
}

void MainMenuController::HandleInput(const std::string& input) {
    if (input == ToMenuOptionKey(MainMenuOption::Exit)) {
        isExitRequested_ = true;
        view_.ShowExitMessage();
        return;
    }

    auto it = subMenuControllers_.find(input);
    if (it != subMenuControllers_.end()) {
        it->second.get().Run();
        return;
    }

    for (MainMenuOption option : kAllMenuOptions) {
        if (input == ToMenuOptionKey(option)) {
            view_.ShowNotImplemented();
            return;
        }
    }
    view_.ShowInvalidSelection();
}

}  // namespace Controller
