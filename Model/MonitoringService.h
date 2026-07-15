#pragma once

#include <vector>

#include "Model/MonitoringTypes.h"
#include "Model/OrderRepository.h"
#include "Model/ProductionJobRepository.h"
#include "Model/SampleRepository.h"

namespace Model {

// Read-only aggregation over Sample/Order/ProductionJob repositories — never
// mutates state, never caches (every call re-reads the repositories), and
// never uses the Phase 3 approval-time available-stock calculation (see
// CLAUDE.md: monitoring must always show the physical Sample::GetStock()).
class MonitoringService {
public:
    MonitoringService(const SampleRepository& sampleRepository, const OrderRepository& orderRepository,
                       const ProductionJobRepository& productionJobRepository);

    OrderStatusSummary GetOrderStatusSummary() const;
    std::vector<SampleStockView> GetSampleStockViews() const;
    MainMenuSummary GetMainMenuSummary() const;

private:
    StockStatus JudgeStockStatus(int stock, int pendingOrderQuantity) const;

    const SampleRepository& sampleRepository_;
    const OrderRepository& orderRepository_;
    const ProductionJobRepository& productionJobRepository_;
};

}  // namespace Model
