#pragma once

#include <string>

#include "Model/ProductionJob.h"

namespace Model {

// progress is never stored (see ProductionJob) — it's derived here at query
// time from startedAt/estimatedTime. Returns 0.0 for a job still waiting in
// the queue (GetStartedAt() == nullopt). estimatedTime/avgProductionTime are
// in minutes, so elapsed time is converted from seconds to minutes before
// dividing. Pure function: never mutates job, and calling it twice with
// different `now` values recomputes independently each time.
double ComputeProgress(const ProductionJob& job, const std::string& nowIso8601);

// Convenience wrapper: ComputeProgress(...) >= 1.0.
bool IsProductionComplete(const ProductionJob& job, const std::string& nowIso8601);

}  // namespace Model
