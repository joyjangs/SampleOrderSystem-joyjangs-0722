#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _DEBUG
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#endif

#include <map>
#include <string>

#include "Controller/MainMenuController.h"
#include "Controller/OrderApprovalController.h"
#include "Controller/OrderController.h"
#include "Controller/ProductionLineController.h"
#include "Controller/ReleaseController.h"
#include "Controller/SampleController.h"
#include "Model/OrderApprovalService.h"
#include "Model/OrderReleaseService.h"
#include "Model/OrderRepository.h"
#include "Model/OrderService.h"
#include "Model/ProductionJobRepository.h"
#include "Model/ProductionLine.h"
#include "Model/SampleRepository.h"
#include "View/MainMenuView.h"
#include "View/OrderApprovalView.h"
#include "View/OrderView.h"
#include "View/ProductionLineView.h"
#include "View/ReleaseView.h"
#include "View/SampleView.h"

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

    View::MainMenuView mainMenuView;
    Controller::MainMenuController mainMenuController(
        mainMenuView, {{Controller::ToMenuOptionKey(Controller::MainMenuOption::SampleManagement), sampleController},
                       {Controller::ToMenuOptionKey(Controller::MainMenuOption::OrderPlacement), orderController},
                       {Controller::ToMenuOptionKey(Controller::MainMenuOption::OrderApproval), orderApprovalController},
                       {Controller::ToMenuOptionKey(Controller::MainMenuOption::ProductionLine), productionLineController},
                       {Controller::ToMenuOptionKey(Controller::MainMenuOption::Release), releaseController}});
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
