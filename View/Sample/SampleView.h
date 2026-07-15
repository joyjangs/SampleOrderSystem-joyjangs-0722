#pragma once

#include <string>
#include <vector>

#include "Model/Sample/Sample.h"

namespace View {

// Console output/input only — no domain logic, no calls into Model.
// Methods are virtual (and this class has a virtual destructor) solely so
// that Phase 1 §6.3 GMock-based SampleController tests can override them on
// a mock subclass without driving a real std::cin/std::cout loop; production
// code always uses a concrete SampleView by reference/value as before.
class SampleView {
public:
    virtual ~SampleView() = default;

    virtual void ShowMenu() const;
    virtual int ReadMenuChoice() const;

    // Re-prompts on a format error (e.g. non-numeric input) until it parses
    // or the input stream is exhausted (EOF), in which case it returns false.
    // Format errors are distinct from semantic validity, which Model owns.
    virtual bool ReadRegistrationInput(std::string& id, std::string& name, double& avgProductionTime,
                                       double& yieldRate) const;
    virtual void ShowRegistrationResult(bool success, const std::string& message) const;
    virtual void ShowError(const std::string& message) const;

    virtual void ShowSampleList(const std::vector<Model::Sample>& samples) const;

    virtual std::string ReadSearchKeyword() const;
    virtual void ShowSearchResult(const std::vector<Model::Sample>& results, const std::string& keyword) const;
};

}  // namespace View
