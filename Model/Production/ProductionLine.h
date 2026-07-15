#pragma once

#include <optional>
#include <vector>

#include "Model/Production/ProductionJob.h"
#include "Model/Production/ProductionJobRepository.h"

namespace Model {

// FIFO production queue. Deliberately holds no queue state of its own —
// ProductionJobRepository (already ordered, JSON-persisted, restart-safe)
// is the single source of truth, so every method reads/writes through it
// rather than duplicating a std::vector that could drift out of sync.
//
// Deviates from docs/Phase3.md's suggested `const ProductionJob* Peek()
// const` (a raw pointer would dangle — FindAll() returns copies, so there's
// no stored object to point to): Peek() returns std::optional<ProductionJob>
// instead.
class ProductionLine {
public:
    explicit ProductionLine(ProductionJobRepository& repository);

    // The first job enqueued into an empty queue starts immediately
    // (SetStartedAt(now)); anything enqueued after that stays queued
    // (startedAt left as nullopt) until CompleteFrontJob() reaches it.
    void Enqueue(ProductionJob job);

    std::optional<ProductionJob> Peek() const;
    std::vector<ProductionJob> ListQueue() const;

    // "생산 라인 조회/새로고침" — recomputes progress for the front job at
    // the given moment; 0.0 if the queue is empty.
    double PeekProgress(const std::string& nowIso8601) const;

    // Completes the front job automatically once its progress (see
    // Model/ProductionProgress.h) reaches 1.0 as of nowIso8601, and starts
    // the next queued job immediately (no idle gap). Returns the completed
    // job, or std::nullopt if there's no job in progress or it isn't done
    // yet. There is no manual "force complete" — completion is always
    // driven by elapsed real time, checked whenever the caller polls (e.g.
    // on production line screen entry/refresh).
    std::optional<ProductionJob> CompleteFrontJobIfDue(const std::string& nowIso8601);

private:
    std::optional<ProductionJob> CompleteFrontJob();

    ProductionJobRepository& repository_;
};

}  // namespace Model
