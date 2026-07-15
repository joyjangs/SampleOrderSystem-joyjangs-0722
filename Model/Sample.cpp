#include "Model/Sample.h"

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

}  // namespace Model
