#include "Model/SampleRepository.h"

#include <algorithm>
#include <cctype>

namespace Model {

namespace {

std::string ToLower(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

}  // namespace

SampleRepository::SampleRepository(std::string filePath) : JsonFileRepository<Sample>(std::move(filePath)) {}

std::vector<Sample> SampleRepository::SearchByName(const std::string& keyword) const {
    const std::string lowerKeyword = ToLower(keyword);
    std::vector<Sample> results;
    for (const Sample& sample : FindAll()) {
        if (ToLower(sample.GetName()).find(lowerKeyword) != std::string::npos) {
            results.push_back(sample);
        }
    }
    return results;
}

}  // namespace Model
