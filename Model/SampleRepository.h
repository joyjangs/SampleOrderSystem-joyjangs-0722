#pragma once

#include <string>
#include <vector>

#include "Model/JsonFileRepository.h"
#include "Model/Sample.h"

namespace Model {

class SampleRepository : public JsonFileRepository<Sample> {
public:
    explicit SampleRepository(std::string filePath = "data/samples.json");

    // Case-insensitive substring match against Sample::GetName(). virtual so
    // GMock-based Controller tests can override it on a mock subclass.
    virtual std::vector<Sample> SearchByName(const std::string& keyword) const;
};

}  // namespace Model
