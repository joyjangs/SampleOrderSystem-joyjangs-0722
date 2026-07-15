#pragma once

#include <string>
#include <vector>

#include "Model/Order.h"

namespace View {

// Console output/input only — no domain logic, no calls into Model.
// Methods are virtual (with a virtual destructor) so GMock-based
// ReleaseController tests can mock this class.
class ReleaseView {
public:
    virtual ~ReleaseView() = default;

    virtual void ShowConfirmedOrders(const std::vector<Model::Order>& orders) const;
    virtual void ShowNoConfirmedOrders() const;
    // 1-based index into the list shown by ShowConfirmedOrders; returns an
    // out-of-range value (e.g. 0) if the input doesn't parse as a number.
    virtual int PromptOrderSelection() const;

    virtual void ShowReleaseSuccess(const std::string& orderId) const;
    virtual void ShowReleaseFailure(const std::string& reason) const;
    virtual void ShowInvalidSelection() const;
};

}  // namespace View
