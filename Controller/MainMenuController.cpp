#include "Controller/MainMenuController.h"

#include <iostream>

namespace Controller {

MainMenuController::MainMenuController(View::MainMenuView& view) : view_(view) {}

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
    if (input == "0") {
        isExitRequested_ = true;
        view_.ShowExitMessage();
        return;
    }
    if (input == "1" || input == "2" || input == "3" || input == "4" || input == "5" || input == "6") {
        view_.ShowNotImplemented();
        return;
    }
    view_.ShowInvalidSelection();
}

}  // namespace Controller
