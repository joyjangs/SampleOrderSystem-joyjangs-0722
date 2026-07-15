#pragma once

#include <string>

namespace Common {

// Local time, ISO 8601 without timezone offset: "YYYY-MM-DDTHH:MM:SS"
// (matches the format already used for Order::createdAt/updatedAt).
std::string CurrentTimestampIso8601();

// Local date only, "YYYYMMDD" — used for order number generation.
std::string CurrentDateYyyymmdd();

}  // namespace Common
