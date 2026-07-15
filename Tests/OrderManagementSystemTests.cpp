#include <gtest/gtest.h>

#include <regex>

#include "Controller/Order/OrderController.h"
#include "Model/Order/OrderRepository.h"
#include "Model/Order/OrderService.h"
#include "Model/Sample/Sample.h"
#include "Model/Sample/SampleRepository.h"
#include "Tests/TestSupport/ConsoleRedirectGuard.h"
#include "Tests/TestSupport/TempFileFixture.h"
#include "View/Order/OrderView.h"

// Phase 2 §7.2: end-to-end tests wiring the real SampleRepository/
// OrderRepository (backed by temp JSON files), the real OrderService/
// OrderController and the real OrderView together — no mocks. OrderView
// reads from std::cin/writes to std::cout directly, so these tests redirect
// std::cin/std::cout for the duration of a scripted OrderController::Run()
// call (RAII guards from Tests/TestSupport/ConsoleRedirectGuard.h, shared
// with Tests/SampleManagementSystemTests.cpp).
namespace {

using Tests::CinRedirectGuard;
using Tests::CoutRedirectGuard;

class OrderManagementSystemTest : public Tests::TempFileFixture {
protected:
    OrderManagementSystemTest()
        : TempFileFixture(std::vector<std::string>{"data/test_system_order_samples.json", "data/test_system_orders.json"}),
          samplesPath("data/test_system_order_samples.json"),
          ordersPath("data/test_system_orders.json") {}

    void SetUp() override {
        Model::SampleRepository samples(samplesPath);
        samples.Add(Model::Sample("S-001", "Wafer-A", 10.0, 0.9, 20));
    }

    std::string samplesPath;
    std::string ordersPath;
};

TEST_F(OrderManagementSystemTest, PlaceOrderEndToEndThroughRealConsoleFlowPersistsReservedOrder) {
    Model::SampleRepository sampleRepository(samplesPath);
    sampleRepository.Load();
    Model::OrderRepository orderRepository(ordersPath);
    Model::OrderService orderService(sampleRepository, orderRepository);
    View::OrderView view;
    Controller::OrderController controller(orderService, view);

    // Scripted console session: sample ID / customer name / quantity.
    const std::string scriptedInput = "S-001\nAcme\n50\n";
    CinRedirectGuard cinGuard(scriptedInput);
    CoutRedirectGuard coutGuard;

    controller.Run();

    const std::string output = coutGuard.Captured();
    EXPECT_NE(output.find("주문 접수 완료"), std::string::npos);
    EXPECT_NE(output.find("RESERVED"), std::string::npos);

    auto allOrders = orderRepository.FindAll();
    ASSERT_EQ(allOrders.size(), 1u);
    EXPECT_EQ(allOrders[0].GetSampleId(), "S-001");
    EXPECT_EQ(allOrders[0].GetCustomerName(), "Acme");
    EXPECT_EQ(allOrders[0].GetQuantity(), 50);
    EXPECT_EQ(allOrders[0].GetStatus(), Model::OrderStatus::Reserved);
    EXPECT_TRUE(std::regex_match(allOrders[0].GetOrderId(), std::regex("ORD-[0-9]{8}-[0-9]{4}")));

    // Order placement must never touch Sample.stock (Phase 3's job, PRD 7.4).
    auto sample = sampleRepository.FindById("S-001");
    ASSERT_TRUE(sample.has_value());
    EXPECT_EQ(sample->GetStock(), 20);
}

TEST_F(OrderManagementSystemTest, UnregisteredSampleIdEndToEndShowsFailureAndCreatesNoOrder) {
    Model::SampleRepository sampleRepository(samplesPath);
    sampleRepository.Load();
    Model::OrderRepository orderRepository(ordersPath);
    Model::OrderService orderService(sampleRepository, orderRepository);
    View::OrderView view;
    Controller::OrderController controller(orderService, view);

    const std::string scriptedInput = "S-999\nAcme\n50\n";
    CinRedirectGuard cinGuard(scriptedInput);
    CoutRedirectGuard coutGuard;

    controller.Run();

    const std::string output = coutGuard.Captured();
    EXPECT_NE(output.find("주문 접수 실패"), std::string::npos);
    EXPECT_TRUE(orderRepository.FindAll().empty());
}

TEST_F(OrderManagementSystemTest, ReservedOrderSurvivesApplicationRestart) {
    {
        Model::SampleRepository sampleRepository(samplesPath);
        sampleRepository.Load();
        Model::OrderRepository orderRepository(ordersPath);
        Model::OrderService orderService(sampleRepository, orderRepository);
        View::OrderView view;
        Controller::OrderController controller(orderService, view);

        const std::string scriptedInput = "S-001\nAcme\n50\n";
        CinRedirectGuard cinGuard(scriptedInput);
        CoutRedirectGuard coutGuard;

        controller.Run();
    }  // repository/controller go out of scope, simulating process exit

    // Simulate application restart: brand new repository instance, same file.
    Model::OrderRepository restarted(ordersPath);
    restarted.Load();

    auto all = restarted.FindAll();
    ASSERT_EQ(all.size(), 1u);
    EXPECT_EQ(all[0].GetSampleId(), "S-001");
    EXPECT_EQ(all[0].GetCustomerName(), "Acme");
    EXPECT_EQ(all[0].GetQuantity(), 50);
    EXPECT_EQ(all[0].GetStatus(), Model::OrderStatus::Reserved);
}

TEST_F(OrderManagementSystemTest, SecondOrderOnSameDayGetsTheNextSequentialOrderNumber) {
    Model::SampleRepository sampleRepository(samplesPath);
    sampleRepository.Load();
    Model::OrderRepository orderRepository(ordersPath);
    Model::OrderService orderService(sampleRepository, orderRepository);
    View::OrderView view;

    {
        Controller::OrderController controller(orderService, view);
        const std::string scriptedInput = "S-001\nAcme\n50\n";
        CinRedirectGuard cinGuard(scriptedInput);
        CoutRedirectGuard coutGuard;
        controller.Run();
    }
    {
        Controller::OrderController controller(orderService, view);
        const std::string scriptedInput = "S-001\nGlobex\n10\n";
        CinRedirectGuard cinGuard(scriptedInput);
        CoutRedirectGuard coutGuard;
        controller.Run();
    }

    auto all = orderRepository.FindAll();
    ASSERT_EQ(all.size(), 2u);
    // Both orders are issued "today" in this test run, so the second order's
    // sequence number must be exactly one greater than the first's (same
    // ORD-YYYYMMDD- prefix, per OrderIdGenerator.cpp).
    EXPECT_EQ(all[0].GetOrderId().substr(0, all[0].GetOrderId().size() - 4),
              all[1].GetOrderId().substr(0, all[1].GetOrderId().size() - 4));
    int firstSeq = std::stoi(all[0].GetOrderId().substr(all[0].GetOrderId().size() - 4));
    int secondSeq = std::stoi(all[1].GetOrderId().substr(all[1].GetOrderId().size() - 4));
    EXPECT_EQ(secondSeq, firstSeq + 1);
}

}  // namespace
