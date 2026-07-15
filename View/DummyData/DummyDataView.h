#pragma once

#include "Model/DummyData/DummyDataGenerator.h"
#include "Model/DummyData/DummyDataOptions.h"

namespace View {

// Console output/input only — no domain logic. virtual so GMock-based
// Controller tests can mock this class (same convention as other Views).
class DummyDataView {
public:
    virtual ~DummyDataView() = default;

    // Reads sampleCount/orderCount/seed into options, overwriting their
    // DummyDataOptions defaults. A non-numeric or blank entry is read by
    // Common::ReadInt() as 0, so this cannot distinguish "user typed 0" from
    // "left it blank" — both are treated as an explicit 0 (see
    // DummyDataOptions.h).
    virtual void PromptOptions(Model::DummyDataOptions& options) const;
    virtual void ShowResult(const Model::DummyDataResult& result) const;
    virtual void ShowInvalidCount() const;
};

}  // namespace View
