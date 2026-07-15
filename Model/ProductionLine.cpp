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
    std::optional<ProductionJob> completed = Peek();
    if (!completed.has_value()) {
        return std::nullopt;
    }
    repository_.Remove(completed->GetOrderId());

    std::optional<ProductionJob> next = Peek();
    if (next.has_value()) {
        next->SetStartedAt(Common::CurrentTimestampIso8601());
        repository_.Update(*next);
    }

    return completed;
}

}  // namespace Model
