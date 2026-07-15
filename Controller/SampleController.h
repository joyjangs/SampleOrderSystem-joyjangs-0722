#pragma once

#include "Controller/ISubMenuController.h"
#include "Model/SampleRepository.h"
#include "View/SampleView.h"

namespace Controller {

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
