#include "View/DummyData/DummyDataView.h"

#include <iostream>

#include "Common/ConsoleInput.h"

namespace View {

void DummyDataView::PromptOptions(Model::DummyDataOptions& options) const {
    std::cout << "생성할 시료 개수: ";
    options.sampleCount = Common::ReadInt();

    std::cout << "생성할 주문 개수: ";
    options.orderCount = Common::ReadInt();

    std::cout << "난수 시드 (0 = 자동): ";
    int seed = Common::ReadInt();
    options.seed = seed > 0 ? static_cast<unsigned int>(seed) : 0;
}

void DummyDataView::ShowResult(const Model::DummyDataResult& result) const {
    std::cout << "\n더미 시료 " << result.createdSampleIds.size() << "개, 더미 주문 "
              << result.createdOrderIds.size() << "건 생성 완료.\n";
    if (!result.failureReasons.empty()) {
        std::cout << "실패 사유:\n";
        for (const std::string& reason : result.failureReasons) {
            std::cout << "- " << reason << "\n";
        }
    }
}

void DummyDataView::ShowInvalidCount() const {
    std::cout << "개수는 0 이상이어야 합니다.\n";
}

}  // namespace View
