#pragma once

#include <string>

#include "Model/JsonFileRepository.h"
#include "Model/ProductionJob.h"

namespace Model {

// Keyed by orderId — one order maps to at most one production job.
class ProductionJobRepository : public JsonFileRepository<ProductionJob> {
public:
    explicit ProductionJobRepository(std::string filePath = "data/production_jobs.json");
};

}  // namespace Model
