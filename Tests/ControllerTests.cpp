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
    Controller::MainMenuController controller(view, sampleController);

    EXPECT_FALSE(controller.IsExitRequested());
    controller.HandleInput("0");
    EXPECT_TRUE(controller.IsExitRequested());
    EXPECT_EQ(sampleController.runCount, 0);
}

TEST(MainMenuControllerTest, HandleInputWithOneDelegatesToSampleController) {
    RunCountingSubMenuController sampleController;
    View::MainMenuView view;
    Controller::MainMenuController controller(view, sampleController);

    controller.HandleInput("1");

    EXPECT_FALSE(controller.IsExitRequested());
    EXPECT_EQ(sampleController.runCount, 1);
}

TEST(MainMenuControllerTest, HandleInputWithOtherKnownMenuNumberDoesNotDelegate) {
    RunCountingSubMenuController sampleController;
    View::MainMenuView view;
    Controller::MainMenuController controller(view, sampleController);

    controller.HandleInput("2");

    EXPECT_FALSE(controller.IsExitRequested());
    EXPECT_EQ(sampleController.runCount, 0);
}
