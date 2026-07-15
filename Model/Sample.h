#pragma once

#include <string>

#include "Common/Json.h"

namespace Model {

// Sample is a plain data container in Phase 0 — registration validation and
// stock mutation rules belong to later phases (Phase 1, Phase 3/4).
class Sample {
public:
    Sample() = default;
    Sample(std::string id, std::string name, double avgProductionTime, double yieldRate, int stock);

    const std::string& GetId() const { return id_; }
    const std::string& GetName() const { return name_; }
    double GetAvgProductionTime() const { return avgProductionTime_; }
    double GetYieldRate() const { return yieldRate_; }
    int GetStock() const { return stock_; }

    void SetName(std::string name) { name_ = std::move(name); }
    void SetAvgProductionTime(double value) { avgProductionTime_ = value; }
    void SetYieldRate(double value) { yieldRate_ = value; }
    void SetStock(int value) { stock_ = value; }

    Common::JsonValue ToJson() const;
    static Sample FromJson(const Common::JsonValue& json);

private:
    std::string id_;
    std::string name_;
    double avgProductionTime_ = 0.0;
    double yieldRate_ = 0.0;
    int stock_ = 0;
};

}  // namespace Model
