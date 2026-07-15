#include <gtest/gtest.h>

#include "Controller/ISubMenuController.h"
#include "Controller/MainMenuController.h"
#include "Model/Monitoring/MonitoringService.h"
#include "Model/Order/OrderRepository.h"
#include "Model/Production/ProductionJobRepository.h"
#include "Model/Sample/SampleRepository.h"
#include "View/MainMenuView.h"

namespace {

class RunCountingSubMenuController : public Controller::ISubMenuController {
public:
    void Run() override { ++runCount; }

    int runCount = 0;
};

}  // namespace

// HandleInput() itself never touches MonitoringService (only Run()'s
// PrintSummary() call does), so these tests just need a valid reference to
// construct MainMenuController — empty, never-loaded repositories are enough.
class MainMenuControllerTest : public ::testing::Test {
protected:
    Model::SampleRepository sampleRepository{"data/test_mock_mainmenu_ctrl_samples_never_written.json"};
    Model::OrderRepository orderRepository{"data/test_mock_mainmenu_ctrl_orders_never_written.json"};
    Model::ProductionJobRepository productionJobRepository{"data/test_mock_mainmenu_ctrl_jobs_never_written.json"};
    Model::MonitoringService monitoringService{sampleRepository, orderRepository, productionJobRepository};
    View::MainMenuView view;
};

TEST_F(MainMenuControllerTest, HandleInputWithZeroRequestsExit) {
    RunCountingSubMenuController sampleController;
    Controller::MainMenuController controller(view, monitoringService, {{"1", sampleController}});

    EXPECT_FALSE(controller.IsExitRequested());
    controller.HandleInput("0");
    EXPECT_TRUE(controller.IsExitRequested());
    EXPECT_EQ(sampleController.runCount, 0);
}

TEST_F(MainMenuControllerTest, HandleInputDelegatesToTheRegisteredController) {
    RunCountingSubMenuController sampleController;
    RunCountingSubMenuController orderController;
    Controller::MainMenuController controller(
        view, monitoringService, {{"1", sampleController}, {"2", orderController}});

    controller.HandleInput("1");
    controller.HandleInput("2");
    controller.HandleInput("2");

    EXPECT_FALSE(controller.IsExitRequested());
    EXPECT_EQ(sampleController.runCount, 1);
    EXPECT_EQ(orderController.runCount, 2);
}

TEST_F(MainMenuControllerTest, HandleInputWithUnregisteredKnownMenuNumberDoesNotDelegate) {
    RunCountingSubMenuController sampleController;
    Controller::MainMenuController controller(view, monitoringService, {{"1", sampleController}});

    controller.HandleInput("3");

    EXPECT_FALSE(controller.IsExitRequested());
    EXPECT_EQ(sampleController.runCount, 0);
}
