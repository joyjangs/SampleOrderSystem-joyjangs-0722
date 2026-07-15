#include "Model/Sample.h"

#include "Common/StringUtils.h"

namespace Model {

Sample::Sample(std::string id, std::string name, double avgProductionTime, double yieldRate, int stock)
    : id_(std::move(id)),
      name_(std::move(name)),
      avgProductionTime_(avgProductionTime),
      yieldRate_(yieldRate),
      stock_(stock) {}

Common::JsonValue Sample::ToJson() const {
    Common::JsonValue json = Common::JsonValue::MakeObject();
    json["id"] = id_;
    json["name"] = name_;
    json["avgProductionTime"] = avgProductionTime_;
    json["yieldRate"] = yieldRate_;
    json["stock"] = stock_;
    return json;
}

Sample Sample::FromJson(const Common::JsonValue& json) {
    return Sample(json.At("id").AsString(), json.At("name").AsString(),
                  json.At("avgProductionTime").AsDouble(), json.At("yieldRate").AsDouble(),
                  json.At("stock").AsInt());
}

bool Sample::IsValidYieldRate(double yieldRate) { return yieldRate > 0.0 && yieldRate <= 1.0; }

bool Sample::IsValidAvgProductionTime(double avgProductionTime) { return avgProductionTime > 0.0; }

bool Sample::IsValidId(const std::string& id) { return !Common::IsBlank(id); }

bool Sample::IsValidName(const std::string& name) { return !Common::IsBlank(name); }

std::optional<Sample> Sample::TryCreate(std::string id, std::string name, double avgProductionTime,
                                        double yieldRate, int stock) {
    if (!IsValidId(id) || !IsValidName(name) || !IsValidAvgProductionTime(avgProductionTime) ||
        !IsValidYieldRate(yieldRate)) {
        return std::nullopt;
    }
    return Sample(std::move(id), std::move(name), avgProductionTime, yieldRate, stock);
}

}  // namespace Model
