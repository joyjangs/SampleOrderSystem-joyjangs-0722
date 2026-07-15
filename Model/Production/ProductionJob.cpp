#include "Model/Production/ProductionJob.h"

namespace Model {

ProductionJob::ProductionJob(std::string orderId, std::string sampleId, int shortage,
                              int actualQuantity, double estimatedTime,
                              std::optional<std::string> startedAt)
    : orderId_(std::move(orderId)),
      sampleId_(std::move(sampleId)),
      shortage_(shortage),
      actualQuantity_(actualQuantity),
      estimatedTime_(estimatedTime),
      startedAt_(std::move(startedAt)) {}

Common::JsonValue ProductionJob::ToJson() const {
    Common::JsonValue json = Common::JsonValue::MakeObject();
    json["orderId"] = orderId_;
    json["sampleId"] = sampleId_;
    json["shortage"] = shortage_;
    json["actualQuantity"] = actualQuantity_;
    json["estimatedTime"] = estimatedTime_;
    json["startedAt"] = startedAt_.has_value() ? Common::JsonValue(*startedAt_) : Common::JsonValue(nullptr);
    return json;
}

ProductionJob ProductionJob::FromJson(const Common::JsonValue& json) {
    return ProductionJob(json.At("orderId").AsString(), json.At("sampleId").AsString(),
                         json.At("shortage").AsInt(), json.At("actualQuantity").AsInt(),
                         json.At("estimatedTime").AsDouble(), json.GetOptionalString("startedAt"));
}

}  // namespace Model
