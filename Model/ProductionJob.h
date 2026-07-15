#pragma once

#include <optional>
#include <string>

#include "Common/Json.h"

namespace Model {

// ProductionJob holds the values fixed at approval time (Phase 3's
// "승인 시점 생산량 확정 규칙"). progress is intentionally not a field here —
// it is derived from startedAt/estimatedTime at query time (see Phase 3).
class ProductionJob {
public:
    ProductionJob() = default;
    ProductionJob(std::string orderId, std::string sampleId, int shortage, int actualQuantity,
                  double estimatedTime, std::optional<std::string> startedAt = std::nullopt);

    // GetId() is the Repository-facing key accessor (see JsonFileRepository);
    // GetOrderId() is the domain-facing name for the same field.
    const std::string& GetId() const { return orderId_; }
    const std::string& GetOrderId() const { return orderId_; }
    const std::string& GetSampleId() const { return sampleId_; }
    int GetShortage() const { return shortage_; }
    int GetActualQuantity() const { return actualQuantity_; }
    double GetEstimatedTime() const { return estimatedTime_; }
    const std::optional<std::string>& GetStartedAt() const { return startedAt_; }

    void SetStartedAt(std::optional<std::string> startedAt) { startedAt_ = std::move(startedAt); }

    Common::JsonValue ToJson() const;
    static ProductionJob FromJson(const Common::JsonValue& json);

private:
    std::string orderId_;
    std::string sampleId_;
    int shortage_ = 0;
    int actualQuantity_ = 0;
    double estimatedTime_ = 0.0;
    std::optional<std::string> startedAt_;
};

}  // namespace Model
