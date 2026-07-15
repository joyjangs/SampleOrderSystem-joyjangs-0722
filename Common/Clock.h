#pragma once

#include <string>

namespace Common {

// Local time, ISO 8601 without timezone offset: "YYYY-MM-DDTHH:MM:SS"
// (matches the format already used for Order::createdAt/updatedAt).
std::string CurrentTimestampIso8601();

// Local date only, "YYYYMMDD" — used for order number generation.
std::string CurrentDateYyyymmdd();

// end - start, in seconds. Both are parsed as local-time ISO 8601 strings in
// the same format CurrentTimestampIso8601() produces. Used by
// Model::ComputeProgress to derive ProductionJob progress at query time.
double SecondsBetweenIso8601(const std::string& start, const std::string& end);

}  // namespace Common
