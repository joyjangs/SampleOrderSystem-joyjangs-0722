#ifdef _WIN32
#include <windows.h>
#endif

#include "Controller/MainMenuController.h"
#include "Model/OrderRepository.h"
#include "Model/ProductionJobRepository.h"
#include "Model/SampleRepository.h"
#include "View/MainMenuView.h"

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    Model::SampleRepository sampleRepository;
    Model::OrderRepository orderRepository;
    Model::ProductionJobRepository productionJobRepository;

    sampleRepository.Load();
    orderRepository.Load();
    productionJobRepository.Load();

    View::MainMenuView mainMenuView;
    Controller::MainMenuController mainMenuController(mainMenuView);
    mainMenuController.Run();

    return 0;
}
