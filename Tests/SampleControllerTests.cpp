#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include "Controller/SampleController.h"
#include "Model/Sample.h"
#include "Model/SampleRepository.h"
#include "View/SampleView.h"

// Phase 1 §6.3: SampleController is tested against GMock doubles for both of
// its collaborators (Model::SampleRepository, View::SampleView) so we can
// assert on the exact calls it makes without touching a real JSON file or a
// real std::cin/std::cout loop.
namespace {

class MockSampleRepository : public Model::SampleRepository {
public:
    // The base SampleRepository ctor takes a file path but MockSampleRepository
    // never calls the real Add/FindAll/etc (they're all mocked out below), so
    // this path is never actually read from or written to.
    MockSampleRepository() : Model::SampleRepository("data/test_mock_samples_never_written.json") {}

    MOCK_METHOD(std::optional<Model::Sample>, FindById, (const std::string& id), (const, override));
    MOCK_METHOD(std::vector<Model::Sample>, FindAll, (), (const, override));
    MOCK_METHOD(void, Add, (const Model::Sample& sample), (override));
    MOCK_METHOD(std::vector<Model::Sample>, SearchByName, (const std::string& keyword), (const, override));
};

class MockSampleView : public View::SampleView {
public:
    MOCK_METHOD(void, ShowMenu, (), (const, override));
    MOCK_METHOD(int, ReadMenuChoice, (), (const, override));
    MOCK_METHOD(bool, ReadRegistrationInput,
                (std::string & id, std::string& name, double& avgProductionTime, double& yieldRate),
                (const, override));
    MOCK_METHOD(void, ShowRegistrationResult, (bool success, const std::string& message), (const, override));
    MOCK_METHOD(void, ShowError, (const std::string& message), (const, override));
    MOCK_METHOD(void, ShowSampleList, (const std::vector<Model::Sample>& samples), (const, override));
    MOCK_METHOD(std::string, ReadSearchKeyword, (), (const, override));
    MOCK_METHOD(void, ShowSearchResult, (const std::vector<Model::Sample>& results, const std::string& keyword),
                (const, override));
};

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;

// Drives one HandleRegister/HandleListAll/HandleSearchByName iteration then
// exits the Run() loop, matching how MainMenuController drives SampleController
// interactively (ShowMenu -> ReadMenuChoice -> dispatch -> loop -> "0" exits).
void RunOneMenuChoiceThenExit(MockSampleView& view, int choice) {
    EXPECT_CALL(view, ShowMenu()).Times(2);
    EXPECT_CALL(view, ReadMenuChoice()).WillOnce(Return(choice)).WillOnce(Return(0));
}

class SampleControllerTest : public ::testing::Test {
protected:
    MockSampleRepository repository;
    MockSampleView view;
    Controller::SampleController controller{repository, view};
};

TEST_F(SampleControllerTest, RegisterWithValidInputAndNoDuplicateAddsSampleAndReportsSuccess) {
    RunOneMenuChoiceThenExit(view, 1);
    EXPECT_CALL(view, ReadRegistrationInput(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<0>("S-001"), SetArgReferee<1>("Wafer-A"), SetArgReferee<2>(10.0),
                        SetArgReferee<3>(0.9), Return(true)));
    EXPECT_CALL(repository, FindById("S-001")).WillOnce(Return(std::nullopt));
    EXPECT_CALL(repository, Add(::testing::AllOf(
                                 ::testing::Property(&Model::Sample::GetId, "S-001"),
                                 ::testing::Property(&Model::Sample::GetName, "Wafer-A"),
                                 ::testing::Property(&Model::Sample::GetAvgProductionTime, 10.0),
                                 ::testing::Property(&Model::Sample::GetYieldRate, 0.9),
                                 ::testing::Property(&Model::Sample::GetStock, 0))))
        .Times(1);
    EXPECT_CALL(view, ShowRegistrationResult(true, _)).Times(1);

    controller.Run();
}

TEST_F(SampleControllerTest, RegisterWithInvalidYieldRateDoesNotAddAndReportsFailure) {
    RunOneMenuChoiceThenExit(view, 1);
    EXPECT_CALL(view, ReadRegistrationInput(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<0>("S-001"), SetArgReferee<1>("Wafer-A"), SetArgReferee<2>(10.0),
                        SetArgReferee<3>(0.0), Return(true)));
    EXPECT_CALL(repository, FindById("S-001")).WillOnce(Return(std::nullopt));
    EXPECT_CALL(repository, Add(_)).Times(0);
    EXPECT_CALL(view, ShowRegistrationResult(false, _)).Times(1);

    controller.Run();
}

TEST_F(SampleControllerTest, RegisterWithDuplicateIdDoesNotAddRegardlessOfValidity) {
    RunOneMenuChoiceThenExit(view, 1);
    EXPECT_CALL(view, ReadRegistrationInput(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<0>("S-001"), SetArgReferee<1>("Wafer-A"), SetArgReferee<2>(10.0),
                        SetArgReferee<3>(0.9), Return(true)));
    EXPECT_CALL(repository, FindById("S-001"))
        .WillOnce(Return(Model::Sample("S-001", "Existing", 5.0, 0.5, 20)));
    EXPECT_CALL(repository, Add(_)).Times(0);
    EXPECT_CALL(view, ShowRegistrationResult(false, ::testing::HasSubstr("S-001"))).Times(1);

    controller.Run();
}

TEST_F(SampleControllerTest, ListAllPassesRepositoryResultStraightThroughToView) {
    RunOneMenuChoiceThenExit(view, 2);
    std::vector<Model::Sample> samples = {Model::Sample("S-001", "Wafer-A", 10.0, 0.9, 5)};
    EXPECT_CALL(repository, FindAll()).WillOnce(Return(samples));
    EXPECT_CALL(view, ShowSampleList(::testing::SizeIs(1))).Times(1);

    controller.Run();
}

TEST_F(SampleControllerTest, SearchByNameForwardsKeywordFromViewToRepositoryAndResultsBack) {
    RunOneMenuChoiceThenExit(view, 3);
    EXPECT_CALL(view, ReadSearchKeyword()).WillOnce(Return("wafer"));
    std::vector<Model::Sample> results = {Model::Sample("S-001", "Wafer-A", 10.0, 0.9, 5)};
    EXPECT_CALL(repository, SearchByName("wafer")).WillOnce(Return(results));
    EXPECT_CALL(view, ShowSearchResult(::testing::SizeIs(1), "wafer")).Times(1);

    controller.Run();
}

}  // namespace
