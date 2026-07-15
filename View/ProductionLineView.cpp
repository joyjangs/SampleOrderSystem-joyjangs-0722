#include "View/ProductionLineView.h"

#include <iomanip>
#include <iostream>
#include <limits>

namespace View {

namespace {

int ReadInt() {
    int value = 0;
    if (!(std::cin >> value)) {
        std::cin.clear();
        value = 0;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return value;
}

}  // namespace

void ProductionLineView::ShowMenu() const {
    std::cout << "\n----- 생산 라인 -----\n"
              << "1. 새로고침\n"
              << "2. 생산 완료 처리\n"
              << "0. 뒤로 가기\n"
              << "선택: ";
}

int ProductionLineView::PromptMenuChoice() const { return ReadInt(); }

void ProductionLineView::ShowCurrentJob(const std::optional<Model::ProductionJob>& currentJob,
                                         double progress) const {
    if (!currentJob.has_value()) {
        std::cout << "현재 생산 중인 작업이 없습니다.\n";
        return;
    }
    std::cout << "생산 중: " << currentJob->GetOrderId() << " (" << currentJob->GetSampleId() << ") "
              << currentJob->GetActualQuantity() << "개 — 진행률 " << std::fixed << std::setprecision(1)
              << (progress * 100.0) << "%\n";
}

void ProductionLineView::ShowQueue(const std::vector<Model::ProductionJob>& queue) const {
    std::cout << "대기열(" << queue.size() << "건):\n";
    for (std::size_t i = 0; i < queue.size(); ++i) {
        const Model::ProductionJob& job = queue[i];
        std::cout << "  " << (i + 1) << ". " << job.GetOrderId() << " (" << job.GetSampleId() << ") "
                  << job.GetActualQuantity() << "개"
                  << (job.GetStartedAt().has_value() ? " [진행 중]" : " [대기 중]") << "\n";
    }
}

void ProductionLineView::ShowNoJobToComplete() const { std::cout << "완료 처리할 생산 작업이 없습니다.\n"; }

void ProductionLineView::ShowJobCompleted(const std::string& orderId, int actualQuantity) const {
    std::cout << orderId << " 생산 완료 처리됨 (" << actualQuantity << "개 재고 반영, 주문 상태 CONFIRMED).\n";
}

void ProductionLineView::ShowInvalidSelection() const { std::cout << "잘못된 선택입니다.\n"; }

}  // namespace View
