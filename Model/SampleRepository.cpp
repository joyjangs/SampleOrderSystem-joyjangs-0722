#include "Model/SampleRepository.h"

namespace Model {

SampleRepository::SampleRepository(std::string filePath) : JsonFileRepository<Sample>(std::move(filePath)) {}

}  // namespace Model
