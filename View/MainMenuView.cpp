#include "View/MainMenuView.h"

#include <iostream>

namespace View {

void MainMenuView::PrintSummary(const Model::MainMenuSummary& summary) const {
    std::cout << "\n[등록 시료 " << summary.registeredSampleCount << "종 | 총 재고 " << summary.totalStock
              << " | 전체 주문 " << summary.totalOrderCount << "건 | 생산 대기 " << summary.productionQueueCount
              << "건]\n";
}

void MainMenuView::ShowMainMenu() const {
    std::cout << "\n===== S-Semi 시료 생산주문관리 시스템 =====\n"
              << "1. 시료 관리\n"
              << "2. 시료 주문\n"
              << "3. 주문 승인/거절\n"
              << "4. 모니터링\n"
              << "5. 생산 라인 조회\n"
              << "6. 출고 처리\n"
              << "7. 더미 데이터 생성 (테스트용)\n"
              << "0. 종료\n"
              << "선택: ";
}

void MainMenuView::ShowNotImplemented() const {
    std::cout << "아직 구현되지 않은 기능입니다.\n";
}

void MainMenuView::ShowInvalidSelection() const {
    std::cout << "잘못된 선택입니다. 다시 입력해주세요.\n";
}

void MainMenuView::ShowExitMessage() const {
    std::cout << "프로그램을 종료합니다.\n";
}

}  // namespace View
