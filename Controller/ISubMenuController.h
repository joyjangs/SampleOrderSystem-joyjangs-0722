#pragma once

namespace Controller {

// Common interface for main-menu submenus (SampleController, and later
// Phase 2~6's order/production/monitoring/release controllers), so
// MainMenuController depends on an abstraction rather than each concrete
// controller (DIP) and doesn't grow a new constructor parameter/include per
// Phase.
class ISubMenuController {
public:
    virtual ~ISubMenuController() = default;
    virtual void Run() = 0;
};

}  // namespace Controller
