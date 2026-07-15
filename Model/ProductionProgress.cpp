#include "Model/ProductionProgress.h"

#include <algorithm>

#include "Common/Clock.h"

namespace Model {

double ComputeProgress(const ProductionJob& job, const std::string& nowIso8601) {
    if (!job.GetStartedAt().has_value()) {
        return 0.0;
    }
    if (job.GetEstimatedTime() <= 0.0) {
        return 1.0;
    }
    double elapsedMinutes = Common::SecondsBetweenIso8601(*job.GetStartedAt(), nowIso8601) / 60.0;
    return std::clamp(elapsedMinutes / job.GetEstimatedTime(), 0.0, 1.0);
}

bool IsProductionComplete(const ProductionJob& job, const std::string& nowIso8601) {
    return ComputeProgress(job, nowIso8601) >= 1.0;
}

}  // namespace Model
