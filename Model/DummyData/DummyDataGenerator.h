#pragma once

#include <optional>
#include <random>
#include <string>
#include <vector>

#include "Model/DummyData/DummyDataOptions.h"
#include "Model/Order/OrderService.h"
#include "Model/Sample/SampleRepository.h"

namespace Model {

struct DummyDataResult {
    std::vector<std::string> createdSampleIds;
    std::vector<std::string> createdOrderIds;
    std::vector<std::string> failureReasons;
};

// Test/dev convenience use-case: generates dummy Sample/Order data through
// the existing self-validating entry points (Sample::TryCreate,
// OrderService::PlaceOrder) instead of duplicating their validation/ID
// generation. Injecting OrderService (rather than OrderRepository directly)
// follows the same "inject the collaborator use-case" convention as
// OrderApprovalController taking a ProductionLine&.
class DummyDataGenerator {
public:
    DummyDataGenerator(SampleRepository& sampleRepository, OrderService& orderService);

    DummyDataResult Generate(const DummyDataOptions& options);

private:
    void GenerateSamples(const DummyDataOptions& options, std::mt19937& rng, DummyDataResult& result);
    void GenerateOrders(const DummyDataOptions& options, std::mt19937& rng, DummyDataResult& result);

    // Sample has no auto ID generator (see Model/Sample.h), so this scans
    // "S-{n}" candidates from nextCandidate upward, skipping IDs already in
    // use. attemptsRemaining bounds the whole scan (shared across every
    // sample generated in one Generate() call) so an already-saturated ID
    // space fails fast instead of looping forever.
    std::optional<std::string> FindNextAvailableSampleId(int& nextCandidate, int& attemptsRemaining) const;

    SampleRepository& sampleRepository_;
    OrderService& orderService_;
};

}  // namespace Model
