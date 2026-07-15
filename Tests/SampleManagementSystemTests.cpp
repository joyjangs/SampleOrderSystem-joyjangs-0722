#include <gtest/gtest.h>

#include "Controller/Sample/SampleController.h"
#include "Model/Sample/SampleRepository.h"
#include "Tests/TestSupport/ConsoleRedirectGuard.h"
#include "Tests/TestSupport/TempFileFixture.h"
#include "View/Sample/SampleView.h"

// Phase 1 §6.5: end-to-end tests wiring the real SampleRepository (backed by
// a temp JSON file), the real SampleController and the real SampleView
// together — no mocks. SampleView reads from std::cin/writes to std::cout
// directly, so these tests redirect std::cin/std::cout for the duration of
// a scripted SampleController::Run() session (RAII guards below), matching
// how a real console session would be driven from the keyboard.
namespace {

using Tests::CinRedirectGuard;
using Tests::CoutRedirectGuard;

class SampleManagementSystemTest : public Tests::TempFileFixture {
protected:
    SampleManagementSystemTest() : TempFileFixture("data/test_system_samples.json") {}
};

TEST_F(SampleManagementSystemTest, RegisterThenListThenSearchEndToEndThroughRealConsoleFlow) {
    Model::SampleRepository repository(path);
    View::SampleView view;
    Controller::SampleController controller(repository, view);

    // Scripted console session:
    //   "1" register -> id / name / avgProductionTime / yieldRate
    //   "2" list all
    //   "3" search by name -> keyword
    //   "0" exit
    const std::string scriptedInput = "1\nS-001\nWafer-A\n10.0\n0.9\n2\n3\nwafer\n0\n";
    CinRedirectGuard cinGuard(scriptedInput);
    CoutRedirectGuard coutGuard;

    controller.Run();

    const std::string output = coutGuard.Captured();
    EXPECT_NE(output.find("등록 성공"), std::string::npos);
    EXPECT_NE(output.find("S-001"), std::string::npos);
    EXPECT_NE(output.find("Wafer-A"), std::string::npos);

    // The registration actually landed in the repository (not just console text).
    auto found = repository.FindById("S-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->GetName(), "Wafer-A");
    EXPECT_DOUBLE_EQ(found->GetAvgProductionTime(), 10.0);
    EXPECT_DOUBLE_EQ(found->GetYieldRate(), 0.9);
    EXPECT_EQ(found->GetStock(), 0);
}

TEST_F(SampleManagementSystemTest, ControllerRejectsDuplicateRegistrationThroughRealConsoleFlow) {
    Model::SampleRepository repository(path);
    repository.Add(Model::Sample("S-001", "Wafer-A", 10.0, 0.9, 0));

    View::SampleView view;
    Controller::SampleController controller(repository, view);

    // Attempt to register the same id again, then exit.
    const std::string scriptedInput = "1\nS-001\nWafer-A-Duplicate\n5.0\n0.5\n0\n";
    CinRedirectGuard cinGuard(scriptedInput);
    CoutRedirectGuard coutGuard;

    controller.Run();

    const std::string output = coutGuard.Captured();
    EXPECT_NE(output.find("등록 실패"), std::string::npos);

    // Still only the original sample — the duplicate attempt did not overwrite it.
    EXPECT_EQ(repository.FindAll().size(), 1u);
    auto found = repository.FindById("S-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->GetName(), "Wafer-A");
}

TEST_F(SampleManagementSystemTest, RegisteredSampleSurvivesApplicationRestart) {
    {
        Model::SampleRepository repository(path);
        View::SampleView view;
        Controller::SampleController controller(repository, view);

        const std::string scriptedInput = "1\nS-001\nWafer-A\n10.0\n0.9\n0\n";
        CinRedirectGuard cinGuard(scriptedInput);
        CoutRedirectGuard coutGuard;

        controller.Run();
    }  // repository/controller go out of scope, simulating process exit

    // Simulate application restart: brand new repository instance, same file.
    Model::SampleRepository restarted(path);
    restarted.Load();

    auto found = restarted.FindById("S-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->GetName(), "Wafer-A");
    EXPECT_DOUBLE_EQ(found->GetAvgProductionTime(), 10.0);
    EXPECT_DOUBLE_EQ(found->GetYieldRate(), 0.9);
    EXPECT_EQ(found->GetStock(), 0);

    auto searchResults = restarted.SearchByName("wafer");
    ASSERT_EQ(searchResults.size(), 1u);
    EXPECT_EQ(searchResults[0].GetId(), "S-001");
}

}  // namespace
