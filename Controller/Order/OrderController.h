#pragma once

#include "Controller/ISubMenuController.h"
#include "Model/Order/OrderService.h"
#include "View/Order/OrderView.h"

namespace Controller {

// PRD 7.3 order placement — a single action (not a submenu loop like
// SampleController): selecting "2" prompts for the order once, then
// returns to the main menu.
class OrderController : public ISubMenuController {
public:
    OrderController(Model::OrderService& orderService, View::OrderView& view);

    void Run() override;

private:
    void HandlePlaceOrder();

    Model::OrderService& orderService_;
    View::OrderView& view_;
};

}  // namespace Controller
