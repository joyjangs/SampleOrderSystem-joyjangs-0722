#pragma once

#include <string>
#include <vector>

#include "Model/Sample.h"

namespace View {

// Console output/input only — no domain logic, no calls into Model.
class SampleView {
public:
    void ShowMenu() const;
    int ReadMenuChoice() const;

    // Re-prompts on a format error (e.g. non-numeric input) until it parses
    // or the input stream is exhausted (EOF), in which case it returns false.
    // Format errors are distinct from semantic validity, which Model owns.
    bool ReadRegistrationInput(std::string& id, std::string& name, double& avgProductionTime,
                               double& yieldRate) const;
    void ShowRegistrationResult(bool success, const std::string& message) const;
    void ShowError(const std::string& message) const;

    void ShowSampleList(const std::vector<Model::Sample>& samples) const;

    std::string ReadSearchKeyword() const;
    void ShowSearchResult(const std::vector<Model::Sample>& results, const std::string& keyword) const;
};

}  // namespace View
