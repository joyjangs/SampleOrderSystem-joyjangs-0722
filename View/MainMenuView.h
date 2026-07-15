#pragma once

namespace View {

// Console output only — no domain logic, no calls into Model.
class MainMenuView {
public:
    void ShowMainMenu() const;
    void ShowNotImplemented() const;
    void ShowInvalidSelection() const;
    void ShowExitMessage() const;
};

}  // namespace View
