#include "Controller/MainMenuController.h"

#include <iostream>

namespace Controller {

namespace {
constexpr const char* kExitOption = "0";
// PRD 7.1's full menu (1~6); options not present in subMenuControllers_ yet
// report "not implemented" instead of "invalid selection".
constexpr const char* kAllMenuOptions[] = {"1", "2", "3", "4", "5", "6"};
}  // namespace

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
    if (input == kExitOption) {
        isExitRequested_ = true;
        view_.ShowExitMessage();
        return;
    }

    auto it = subMenuControllers_.find(input);
    if (it != subMenuControllers_.end()) {
        it->second.get().Run();
        return;
    }

    for (const char* option : kAllMenuOptions) {
        if (input == option) {
            view_.ShowNotImplemented();
            return;
        }
    }
    view_.ShowInvalidSelection();
}

}  // namespace Controller
