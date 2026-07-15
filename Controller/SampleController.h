#pragma once

#include "Model/SampleRepository.h"
#include "View/SampleView.h"

namespace Controller {

// Sample management submenu: register / list / search. Delegates all
// domain validation to Model::Sample and all I/O to View::SampleView.
class SampleController {
public:
    SampleController(Model::SampleRepository& repository, View::SampleView& view);
    virtual ~SampleController() = default;

    // virtual so MainMenuController tests can stub it out (avoids driving a
    // real interactive std::cin loop from a unit test).
    virtual void Run();

private:
    void HandleRegister();
    void HandleListAll();
    void HandleSearchByName();

    Model::SampleRepository& repository_;
    View::SampleView& view_;
};

}  // namespace Controller
