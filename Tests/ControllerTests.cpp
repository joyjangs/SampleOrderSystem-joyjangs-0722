#include <gtest/gtest.h>

#include "Controller/MainMenuController.h"
#include "Controller/SampleController.h"
#include "Model/SampleRepository.h"
#include "View/MainMenuView.h"
#include "View/SampleView.h"

namespace {

// Avoids driving SampleController's real interactive std::cin loop from a
// unit test; just records whether/how many times Run() was delegated to.
class RunCountingSampleController : public Controller::SampleController {
public:
    RunCountingSampleController(Model::SampleRepository& repository, View::SampleView& view)
        : Controller::SampleController(repository, view) {}

    void Run() override { ++runCount; }

    int runCount = 0;
};

}  // namespace

TEST(MainMenuControllerTest, HandleInputWithZeroRequestsExit) {
    Model::SampleRepository repository("data/test_main_menu_samples.json");
    View::SampleView sampleView;
    RunCountingSampleController sampleController(repository, sampleView);
    View::MainMenuView view;
    Controller::MainMenuController controller(view, sampleController);

    EXPECT_FALSE(controller.IsExitRequested());
    controller.HandleInput("0");
    EXPECT_TRUE(controller.IsExitRequested());
    EXPECT_EQ(sampleController.runCount, 0);
}

TEST(MainMenuControllerTest, HandleInputWithOneDelegatesToSampleController) {
    Model::SampleRepository repository("data/test_main_menu_samples.json");
    View::SampleView sampleView;
    RunCountingSampleController sampleController(repository, sampleView);
    View::MainMenuView view;
    Controller::MainMenuController controller(view, sampleController);

    controller.HandleInput("1");

    EXPECT_FALSE(controller.IsExitRequested());
    EXPECT_EQ(sampleController.runCount, 1);
}

TEST(MainMenuControllerTest, HandleInputWithOtherKnownMenuNumberDoesNotDelegate) {
    Model::SampleRepository repository("data/test_main_menu_samples.json");
    View::SampleView sampleView;
    RunCountingSampleController sampleController(repository, sampleView);
    View::MainMenuView view;
    Controller::MainMenuController controller(view, sampleController);

    controller.HandleInput("2");

    EXPECT_FALSE(controller.IsExitRequested());
    EXPECT_EQ(sampleController.runCount, 0);
}
