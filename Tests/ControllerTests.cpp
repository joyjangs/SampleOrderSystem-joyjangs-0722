#include <gtest/gtest.h>

#include "Controller/ISubMenuController.h"
#include "Controller/MainMenuController.h"
#include "View/MainMenuView.h"

namespace {

class RunCountingSubMenuController : public Controller::ISubMenuController {
public:
    void Run() override { ++runCount; }

    int runCount = 0;
};

}  // namespace

TEST(MainMenuControllerTest, HandleInputWithZeroRequestsExit) {
    RunCountingSubMenuController sampleController;
    View::MainMenuView view;
    Controller::MainMenuController controller(view, {{"1", sampleController}});

    EXPECT_FALSE(controller.IsExitRequested());
    controller.HandleInput("0");
    EXPECT_TRUE(controller.IsExitRequested());
    EXPECT_EQ(sampleController.runCount, 0);
}

TEST(MainMenuControllerTest, HandleInputDelegatesToTheRegisteredController) {
    RunCountingSubMenuController sampleController;
    RunCountingSubMenuController orderController;
    View::MainMenuView view;
    Controller::MainMenuController controller(
        view, {{"1", sampleController}, {"2", orderController}});

    controller.HandleInput("1");
    controller.HandleInput("2");
    controller.HandleInput("2");

    EXPECT_FALSE(controller.IsExitRequested());
    EXPECT_EQ(sampleController.runCount, 1);
    EXPECT_EQ(orderController.runCount, 2);
}

TEST(MainMenuControllerTest, HandleInputWithUnregisteredKnownMenuNumberDoesNotDelegate) {
    RunCountingSubMenuController sampleController;
    View::MainMenuView view;
    Controller::MainMenuController controller(view, {{"1", sampleController}});

    controller.HandleInput("3");

    EXPECT_FALSE(controller.IsExitRequested());
    EXPECT_EQ(sampleController.runCount, 0);
}
