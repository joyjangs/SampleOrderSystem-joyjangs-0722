#include "Model/DummyDataGenerator.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "Model/Sample.h"

namespace Model {

namespace {
const std::string kDummyCustomerNames[] = {"고객A", "고객B", "고객C", "고객D", "고객E"};
}  // namespace

DummyDataGenerator::DummyDataGenerator(SampleRepository& sampleRepository, OrderService& orderService)
    : sampleRepository_(sampleRepository), orderService_(orderService) {}

DummyDataResult DummyDataGenerator::Generate(const DummyDataOptions& options) {
    std::mt19937 rng(options.seed == 0 ? std::random_device{}() : options.seed);

    DummyDataResult result;
    GenerateSamples(options, rng, result);
    GenerateOrders(options, rng, result);
    return result;
}

void DummyDataGenerator::GenerateSamples(const DummyDataOptions& options, std::mt19937& rng,
                                          DummyDataResult& result) {
    std::uniform_real_distribution<double> yieldDist(options.minYieldRate, options.maxYieldRate);
    std::uniform_real_distribution<double> avgTimeDist(options.minAvgProductionTime, options.maxAvgProductionTime);
    std::uniform_int_distribution<int> stockDist(options.minInitialStock, options.maxInitialStock);

    int nextCandidate = 1;
    int attemptsRemaining = std::max(100, options.sampleCount * 10);

    for (int i = 0; i < options.sampleCount; ++i) {
        std::optional<std::string> id = FindNextAvailableSampleId(nextCandidate, attemptsRemaining);
        if (!id.has_value()) {
            result.failureReasons.push_back("더미 시료 ID 채번에 실패했습니다 (시도 횟수 초과).");
            break;  // the ID space is exhausted; further attempts would fail identically.
        }

        std::string name = "더미시료-" + std::to_string(i + 1);
        std::optional<Sample> sample =
            Sample::TryCreate(*id, name, avgTimeDist(rng), yieldDist(rng), stockDist(rng));
        if (!sample.has_value()) {
            result.failureReasons.push_back("더미 시료 생성에 실패했습니다: " + *id);
            continue;
        }

        sampleRepository_.Add(*sample);
        result.createdSampleIds.push_back(*id);
    }
}

void DummyDataGenerator::GenerateOrders(const DummyDataOptions& options, std::mt19937& rng,
                                         DummyDataResult& result) {
    std::vector<Sample> samples = sampleRepository_.FindAll();
    if (samples.empty()) {
        if (options.orderCount > 0) {
            result.failureReasons.push_back("등록된 시료가 없어 더미 주문을 생성할 수 없습니다.");
        }
        return;
    }

    std::uniform_int_distribution<std::size_t> sampleDist(0, samples.size() - 1);
    std::uniform_int_distribution<int> quantityDist(options.minOrderQuantity, options.maxOrderQuantity);
    std::uniform_int_distribution<std::size_t> customerDist(0, std::size(kDummyCustomerNames) - 1);

    for (int i = 0; i < options.orderCount; ++i) {
        const Sample& sample = samples[sampleDist(rng)];
        const std::string& customerName = kDummyCustomerNames[customerDist(rng)];
        int quantity = quantityDist(rng);

        PlaceOrderResult placeResult = orderService_.PlaceOrder(sample.GetId(), customerName, quantity);
        if (!placeResult.success) {
            result.failureReasons.push_back(placeResult.failureReason);
            continue;
        }
        result.createdOrderIds.push_back(placeResult.order->GetOrderId());
    }
}

std::optional<std::string> DummyDataGenerator::FindNextAvailableSampleId(int& nextCandidate,
                                                                          int& attemptsRemaining) const {
    while (attemptsRemaining > 0) {
        --attemptsRemaining;
        std::ostringstream oss;
        oss << "S-" << std::setw(3) << std::setfill('0') << nextCandidate;
        ++nextCandidate;

        std::string candidateId = oss.str();
        if (!sampleRepository_.FindById(candidateId).has_value()) {
            return candidateId;
        }
    }
    return std::nullopt;
}

}  // namespace Model
