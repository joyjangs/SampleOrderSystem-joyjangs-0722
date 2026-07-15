#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _DEBUG
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#endif

#include <map>
#include <string>

#include "Controller/DummyData/DummyDataController.h"
#include "Controller/MainMenuController.h"
#include "Controller/Monitoring/MonitoringController.h"
#include "Controller/Order/OrderApprovalController.h"
#include "Controller/Order/OrderController.h"
#include "Controller/Production/ProductionLineController.h"
#include "Controller/Order/ReleaseController.h"
#include "Controller/Sample/SampleController.h"
#include "Model/DummyData/DummyDataGenerator.h"
#include "Model/Monitoring/MonitoringService.h"
#include "Model/Order/OrderApprovalService.h"
#include "Model/Order/OrderReleaseService.h"
#include "Model/Order/OrderRepository.h"
#include "Model/Order/OrderService.h"
#include "Model/Production/ProductionJobRepository.h"
#include "Model/Production/ProductionLine.h"
#include "Model/Sample/SampleRepository.h"
#include "View/DummyData/DummyDataView.h"
#include "View/MainMenuView.h"
#include "View/Monitoring/MonitoringView.h"
#include "View/Order/OrderApprovalView.h"
#include "View/Order/OrderView.h"
#include "View/Production/ProductionLineView.h"
#include "View/Order/ReleaseView.h"
#include "View/Sample/SampleView.h"

namespace {

void RunApp() {
    Model::SampleRepository sampleRepository;
    Model::OrderRepository orderRepository;
    Model::ProductionJobRepository productionJobRepository;

    sampleRepository.Load();
    orderRepository.Load();
    productionJobRepository.Load();

    View::SampleView sampleView;
    Controller::SampleController sampleController(sampleRepository, sampleView);

    Model::OrderService orderService(sampleRepository, orderRepository);
    View::OrderView orderView;
    Controller::OrderController orderController(orderService, orderView);

    Model::ProductionLine productionLine(productionJobRepository);

    Model::OrderApprovalService orderApprovalService(sampleRepository, orderRepository);
    View::OrderApprovalView orderApprovalView;
    Controller::OrderApprovalController orderApprovalController(orderApprovalService, orderRepository,
                                                                 productionLine, orderApprovalView);

    View::ProductionLineView productionLineView;
    Controller::ProductionLineController productionLineController(productionLine, sampleRepository,
                                                                     orderRepository, productionLineView);

    Model::OrderReleaseService orderReleaseService(sampleRepository, orderRepository);
    View::ReleaseView releaseView;
    Controller::ReleaseController releaseController(orderReleaseService, orderRepository, releaseView);

    Model::MonitoringService monitoringService(sampleRepository, orderRepository, productionJobRepository);
    View::MonitoringView monitoringView;
    Controller::MonitoringController monitoringController(monitoringService, monitoringView);

    Model::DummyDataGenerator dummyDataGenerator(sampleRepository, orderService);
    View::DummyDataView dummyDataView;
    Controller::DummyDataController dummyDataController(dummyDataGenerator, dummyDataView);

    View::MainMenuView mainMenuView;
    Controller::MainMenuController mainMenuController(
        mainMenuView, monitoringService,
        {{Controller::ToMenuOptionKey(Controller::MainMenuOption::SampleManagement), sampleController},
         {Controller::ToMenuOptionKey(Controller::MainMenuOption::OrderPlacement), orderController},
         {Controller::ToMenuOptionKey(Controller::MainMenuOption::OrderApproval), orderApprovalController},
         {Controller::ToMenuOptionKey(Controller::MainMenuOption::Monitoring), monitoringController},
         {Controller::ToMenuOptionKey(Controller::MainMenuOption::ProductionLine), productionLineController},
         {Controller::ToMenuOptionKey(Controller::MainMenuOption::Release), releaseController},
         {Controller::ToMenuOptionKey(Controller::MainMenuOption::DummyDataGeneration), dummyDataController}});
    mainMenuController.Run();
}

}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

#ifdef _DEBUG
    // Debug-only test entry point: `SampleOrderSystem.exe --test` runs the
    // GoogleTest/GMock suite instead of the console app. Release builds
    // don't compile the test sources or link gtest/gmock at all (see
    // SampleOrderSystem.vcxproj's Debug-only ItemGroup/Import).
    if (argc > 1 && std::string(argv[1]) == "--test") {
        ::testing::InitGoogleMock(&argc, argv);
        return RUN_ALL_TESTS();
    }
#endif

    RunApp();
    return 0;
}
