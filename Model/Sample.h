#pragma once

#include <optional>
#include <string>

#include "Common/Json.h"

namespace Model {

// Sample is a plain data container — the constructor stores values as given,
// with no validation. Registration validation lives in TryCreate() instead,
// so callers that need to trust the invariants (Controller registration)
// go through TryCreate while internal round-trips (FromJson) keep using the
// constructor directly.
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

    // yieldRate = 0 would make Phase 3's ceil(shortage / yieldRate) divide by zero,
    // so 0 is rejected here at registration time.
    static bool IsValidYieldRate(double yieldRate);
    static bool IsValidAvgProductionTime(double avgProductionTime);
    static bool IsValidId(const std::string& id);
    static bool IsValidName(const std::string& name);

    // Returns std::nullopt (never throws) if any field fails validation.
    // stock always starts at 0 — Phase 1 registration has no initial-stock
    // input; stock only changes on production completion/release (Phase 3/4).
    static std::optional<Sample> TryCreate(std::string id, std::string name, double avgProductionTime,
                                           double yieldRate, int stock = 0);

private:
    std::string id_;
    std::string name_;
    double avgProductionTime_ = 0.0;
    double yieldRate_ = 0.0;
    int stock_ = 0;
};

}  // namespace Model
