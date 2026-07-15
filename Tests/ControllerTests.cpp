#include <gtest/gtest.h>

#include "Controller/MainMenuController.h"
#include "View/MainMenuView.h"

TEST(MainMenuControllerTest, HandleInputWithZeroRequestsExit) {
    View::MainMenuView view;
    Controller::MainMenuController controller(view);

    EXPECT_FALSE(controller.IsExitRequested());
    controller.HandleInput("0");
    EXPECT_TRUE(controller.IsExitRequested());
}

TEST(MainMenuControllerTest, HandleInputWithKnownMenuNumberDoesNotExit) {
    View::MainMenuView view;
    Controller::MainMenuController controller(view);

    controller.HandleInput("1");

    EXPECT_FALSE(controller.IsExitRequested());
}
