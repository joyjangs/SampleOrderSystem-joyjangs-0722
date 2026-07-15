#pragma once

#include "Controller/ISubMenuController.h"
#include "Model/Sample/SampleRepository.h"
#include "View/Sample/SampleView.h"

namespace Controller {

// The digit the user types at View::SampleView::ReadMenuChoice().
enum class SampleMenuOption {
    Exit = 0,
    Register = 1,
    ListAll = 2,
    Search = 3,
};

// Sample management submenu: register / list / search. Delegates all
// domain validation to Model::Sample and all I/O to View::SampleView.
class SampleController : public ISubMenuController {
public:
    SampleController(Model::SampleRepository& repository, View::SampleView& view);

    // virtual (via ISubMenuController) so MainMenuController tests can stub
    // it out (avoids driving a real interactive std::cin loop from a unit test).
    void Run() override;

private:
    void HandleRegister();
    void HandleListAll();
    void HandleSearchByName();

    Model::SampleRepository& repository_;
    View::SampleView& view_;
};

}  // namespace Controller
