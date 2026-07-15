#include "Common/Clock.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Common {

namespace {

std::tm LocalTimeNow() {
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif
    return localTime;
}

}  // namespace

std::string CurrentTimestampIso8601() {
    std::tm localTime = LocalTimeNow();
    std::ostringstream out;
    out << std::put_time(&localTime, "%Y-%m-%dT%H:%M:%S");
    return out.str();
}

std::string CurrentDateYyyymmdd() {
    std::tm localTime = LocalTimeNow();
    std::ostringstream out;
    out << std::put_time(&localTime, "%Y%m%d");
    return out.str();
}

}  // namespace Common
