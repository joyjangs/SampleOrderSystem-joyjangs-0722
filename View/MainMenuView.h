#pragma once

#include "Model/Monitoring/MonitoringTypes.h"

namespace View {

// Console output only — no domain logic, no calls into Model.
class MainMenuView {
public:
    void PrintSummary(const Model::MainMenuSummary& summary) const;
    void ShowMainMenu() const;
    void ShowNotImplemented() const;
    void ShowInvalidSelection() const;
    void ShowExitMessage() const;
};

}  // namespace View
