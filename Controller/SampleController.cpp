#include "Controller/SampleController.h"

#include <optional>
#include <string>

namespace Controller {

SampleController::SampleController(Model::SampleRepository& repository, View::SampleView& view)
    : repository_(repository), view_(view) {}

void SampleController::Run() {
    while (true) {
        view_.ShowMenu();
        int choice = view_.ReadMenuChoice();
        if (choice == 0) {
            return;
        }
        switch (choice) {
            case 1: HandleRegister(); break;
            case 2: HandleListAll(); break;
            case 3: HandleSearchByName(); break;
            default: view_.ShowError("잘못된 선택입니다."); break;
        }
    }
}

void SampleController::HandleRegister() {
    std::string id;
    std::string name;
    double avgProductionTime = 0.0;
    double yieldRate = 0.0;
    if (!view_.ReadRegistrationInput(id, name, avgProductionTime, yieldRate)) {
        view_.ShowRegistrationResult(false, "입력 형식이 올바르지 않습니다.");
        return;
    }

    if (repository_.FindById(id).has_value()) {
        view_.ShowRegistrationResult(false, "이미 존재하는 시료 ID입니다: " + id);
        return;
    }

    std::optional<Model::Sample> sample = Model::Sample::TryCreate(id, name, avgProductionTime, yieldRate);
    if (!sample.has_value()) {
        view_.ShowRegistrationResult(false, "유효하지 않은 값입니다 (수율은 0 초과 1 이하, 생산시간은 0 초과, "
                                             "ID/이름은 비어있지 않아야 합니다).");
        return;
    }

    repository_.Add(*sample);
    view_.ShowRegistrationResult(true, id + " 등록 완료");
}

void SampleController::HandleListAll() { view_.ShowSampleList(repository_.FindAll()); }

void SampleController::HandleSearchByName() {
    std::string keyword = view_.ReadSearchKeyword();
    view_.ShowSearchResult(repository_.SearchByName(keyword), keyword);
}

}  // namespace Controller
