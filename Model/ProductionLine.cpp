#include "Model/ProductionLine.h"

#include "Common/Clock.h"
#include "Model/ProductionProgress.h"

namespace Model {

ProductionLine::ProductionLine(ProductionJobRepository& repository) : repository_(repository) {}

void ProductionLine::Enqueue(ProductionJob job) {
    if (repository_.FindAll().empty()) {
        job.SetStartedAt(Common::CurrentTimestampIso8601());
    }
    repository_.Add(job);
}

std::optional<ProductionJob> ProductionLine::Peek() const {
    std::vector<ProductionJob> queue = repository_.FindAll();
    if (queue.empty()) {
        return std::nullopt;
    }
    return queue.front();
}

std::vector<ProductionJob> ProductionLine::ListQueue() const { return repository_.FindAll(); }

double ProductionLine::PeekProgress(const std::string& nowIso8601) const {
    std::optional<ProductionJob> front = Peek();
    if (!front.has_value()) {
        return 0.0;
    }
    return ComputeProgress(*front, nowIso8601);
}

std::optional<ProductionJob> ProductionLine::CompleteFrontJob() {
    std::vector<ProductionJob> queue = repository_.FindAll();
    if (queue.empty()) {
        return std::nullopt;
    }

    ProductionJob completed = queue.front();
    repository_.Remove(completed.GetOrderId());

    std::vector<ProductionJob> remaining = repository_.FindAll();
    if (!remaining.empty()) {
        ProductionJob next = remaining.front();
        next.SetStartedAt(Common::CurrentTimestampIso8601());
        repository_.Update(next);
    }

    return completed;
}

}  // namespace Model
