#pragma once

#include <string>
#include <vector>

#include "Model/Order.h"

namespace View {

// Console output/input only — no domain logic, no calls into Model.
// Methods are virtual (with a virtual destructor) so GMock-based
// OrderApprovalController tests can mock this class.
class OrderApprovalView {
public:
    virtual ~OrderApprovalView() = default;

    virtual void ShowReservedOrders(const std::vector<Model::Order>& orders) const;
    virtual void ShowNoReservedOrders() const;

    // 1-based index into the list shown by ShowReservedOrders; returns an
    // out-of-range value (e.g. 0) if the input doesn't parse as a number.
    virtual int PromptOrderSelection(int count) const;
    // Returns 1 (approve), 2 (reject), or any other value for "invalid".
    virtual int PromptApproveOrReject() const;

    virtual void ShowConfirmedImmediately(const std::string& orderId) const;
    virtual void ShowQueuedForProduction(const std::string& orderId, int shortage, int actualQuantity,
                                        double estimatedTime) const;
    virtual void ShowRejected(const std::string& orderId) const;
    virtual void ShowInvalidSelection() const;
};

}  // namespace View
