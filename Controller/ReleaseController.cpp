#include "Controller/ReleaseController.h"

#include <algorithm>
#include <stdexcept>

#include "Model/OrderStatus.h"

namespace Controller {

namespace {

std::vector<Model::Order> ConfirmedOrdersSortedByOrderId(const std::vector<Model::Order>& allOrders) {
    std::vector<Model::Order> confirmed;
    for (const Model::Order& order : allOrders) {
        if (order.GetStatus() == Model::OrderStatus::Confirmed) {
            confirmed.push_back(order);
        }
    }
    std::sort(confirmed.begin(), confirmed.end(),
              [](const Model::Order& a, const Model::Order& b) { return a.GetOrderId() < b.GetOrderId(); });
    return confirmed;
}

}  // namespace

ReleaseController::ReleaseController(Model::OrderReleaseService& releaseService,
                                      Model::OrderRepository& orderRepository, View::ReleaseView& view)
    : releaseService_(releaseService), orderRepository_(orderRepository), view_(view) {}

void ReleaseController::Run() { HandleRelease(); }

void ReleaseController::HandleRelease() {
    std::vector<Model::Order> confirmed = ConfirmedOrdersSortedByOrderId(orderRepository_.FindAll());
    if (confirmed.empty()) {
        view_.ShowNoConfirmedOrders();
        return;
    }
    view_.ShowConfirmedOrders(confirmed);

    int selection = view_.PromptOrderSelection();
    if (selection < 1 || selection > static_cast<int>(confirmed.size())) {
        view_.ShowInvalidSelection();
        return;
    }
    const Model::Order& selected = confirmed[static_cast<std::size_t>(selection - 1)];

    try {
        releaseService_.Release(selected);
        view_.ShowReleaseSuccess(selected.GetOrderId());
    } catch (const std::exception& e) {
        view_.ShowReleaseFailure(e.what());
    }
}

}  // namespace Controller
