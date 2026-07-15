#pragma once

#include <string>

#include "Model/OrderStatus.h"

namespace View {

// Console output/input only — no domain logic, no calls into Model.
// Methods are virtual (with a virtual destructor) so GMock-based
// OrderController tests can mock this class, same pattern as SampleView.
class OrderView {
public:
    virtual ~OrderView() = default;

    virtual std::string PromptSampleId() const;
    virtual std::string PromptCustomerName() const;

    // Re-prompts on a non-numeric quantity until it parses or the input
    // stream hits EOF, in which case it returns false.
    virtual bool PromptQuantity(int& quantity) const;

    virtual void ShowOrderPlaced(const std::string& orderId, Model::OrderStatus status) const;
    virtual void ShowOrderRejectedInput(const std::string& reason) const;
};

}  // namespace View
