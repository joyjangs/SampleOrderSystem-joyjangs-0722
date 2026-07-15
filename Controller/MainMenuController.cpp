#include "Controller/MainMenuController.h"

#include <iostream>

namespace Controller {

namespace {
constexpr const char* kExitOption = "0";
constexpr const char* kSampleManagementOption = "1";
constexpr const char* kOrderPlacementOption = "2";
constexpr const char* kOrderApprovalOption = "3";
constexpr const char* kMonitoringOption = "4";
constexpr const char* kProductionLineOption = "5";
constexpr const char* kReleaseOption = "6";
}  // namespace

MainMenuController::MainMenuController(View::MainMenuView& view, Controller::ISubMenuController& sampleController)
    : view_(view), sampleController_(sampleController) {}

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
    if (input == kSampleManagementOption) {
        sampleController_.Run();
        return;
    }
    if (input == kOrderPlacementOption || input == kOrderApprovalOption || input == kMonitoringOption ||
        input == kProductionLineOption || input == kReleaseOption) {
        view_.ShowNotImplemented();
        return;
    }
    view_.ShowInvalidSelection();
}

}  // namespace Controller
