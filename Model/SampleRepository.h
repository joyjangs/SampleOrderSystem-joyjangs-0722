#pragma once

#include <string>

#include "Model/JsonFileRepository.h"
#include "Model/Sample.h"

namespace Model {

class SampleRepository : public JsonFileRepository<Sample> {
public:
    explicit SampleRepository(std::string filePath = "data/samples.json");
};

}  // namespace Model
