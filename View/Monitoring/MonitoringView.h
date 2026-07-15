#pragma once

#include <vector>

#include "Model/Monitoring/MonitoringTypes.h"

namespace View {

// Console output only — no domain logic. virtual so GMock-based Controller
// tests can mock this class (same convention as ProductionLineView).
class MonitoringView {
public:
    virtual ~MonitoringView() = default;

    virtual void PrintOrderStatusSummary(const Model::OrderStatusSummary& summary) const;
    virtual void PrintSampleStockStatus(const std::vector<Model::SampleStockView>& views) const;
};

}  // namespace View
