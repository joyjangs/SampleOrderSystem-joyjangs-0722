#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Model/ProductionJob.h"

namespace View {

// Console output/input only — no domain logic, no calls into Model.
// Methods are virtual (with a virtual destructor) so GMock-based
// ProductionLineController tests can mock this class.
class ProductionLineView {
public:
    virtual ~ProductionLineView() = default;

    virtual void ShowMenu() const;
    virtual int PromptMenuChoice() const;

    // progress is passed in already-computed (Controller calls
    // ProductionLine::PeekProgress) — View never calculates it itself.
    virtual void ShowCurrentJob(const std::optional<Model::ProductionJob>& currentJob, double progress) const;
    virtual void ShowQueue(const std::vector<Model::ProductionJob>& queue) const;

    virtual void ShowNoJobToComplete() const;
    virtual void ShowJobCompleted(const std::string& orderId, int actualQuantity) const;
    virtual void ShowInvalidSelection() const;
};

}  // namespace View
