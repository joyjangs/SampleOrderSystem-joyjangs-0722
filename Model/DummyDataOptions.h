#pragma once

namespace Model {

// Test/dev convenience input for DummyDataGenerator::Generate. A count of 0
// means "generate none of that kind" (not "use a different default") — see
// DummyDataGenerator.h for why 0 can't also mean "keep default".
struct DummyDataOptions {
    int sampleCount = 5;
    int orderCount = 10;
    unsigned int seed = 0;  // 0 => seed from std::random_device (non-reproducible)

    double minYieldRate = 0.5;  // Sample::IsValidYieldRate requires 0.0 < x <= 1.0
    double maxYieldRate = 1.0;

    double minAvgProductionTime = 1.0;  // Sample::IsValidAvgProductionTime requires > 0
    double maxAvgProductionTime = 120.0;

    int minInitialStock = 0;
    int maxInitialStock = 100;

    int minOrderQuantity = 1;  // Order::IsValidQuantity requires >= 1
    int maxOrderQuantity = 50;
};

}  // namespace Model
