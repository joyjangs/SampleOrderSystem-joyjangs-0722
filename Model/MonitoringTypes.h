#pragma once

#include <string>

namespace Model {

enum class StockStatus {
    Sufficient,  // 여유
    Shortage,    // 부족
    Depleted,    // 고갈
};

std::string ToString(StockStatus status);

// REJECTED is intentionally not a field here (see CLAUDE.md/PRD 7.5 — REJECTED
// is excluded from monitoring aggregates), so it can never be summed in by
// accident.
struct OrderStatusSummary {
    int reservedCount = 0;
    int confirmedCount = 0;
    int producingCount = 0;
    int releaseCount = 0;
};

struct SampleStockView {
    std::string sampleId;
    std::string sampleName;
    int stock = 0;                // physical Sample::GetStock(), never the approval-time available stock
    int pendingOrderQuantity = 0;  // sum of PRODUCING orders' quantity for this sample (see Phase5.md 3.4)
    StockStatus status = StockStatus::Sufficient;
};

struct MainMenuSummary {
    int registeredSampleCount = 0;
    int totalStock = 0;
    int totalOrderCount = 0;       // REJECTED excluded
    int productionQueueCount = 0;  // ProductionJob entries still in the queue
};

}  // namespace Model
