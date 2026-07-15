#include "Model/ProductionJobRepository.h"

namespace Model {

ProductionJobRepository::ProductionJobRepository(std::string filePath)
    : JsonFileRepository<ProductionJob>(std::move(filePath)) {}

}  // namespace Model
