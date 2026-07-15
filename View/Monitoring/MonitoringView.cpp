#include "View/Monitoring/MonitoringView.h"

#include <iostream>

namespace View {

void MonitoringView::PrintOrderStatusSummary(const Model::OrderStatusSummary& summary) const {
    std::cout << "\n--- 상태별 주문 건수 ---\n"
              << "RESERVED (접수): " << summary.reservedCount << "건\n"
              << "CONFIRMED (출고 대기): " << summary.confirmedCount << "건\n"
              << "PRODUCING (생산 중): " << summary.producingCount << "건\n"
              << "RELEASE (출고 완료): " << summary.releaseCount << "건\n";
}

void MonitoringView::PrintSampleStockStatus(const std::vector<Model::SampleStockView>& views) const {
    std::cout << "\n--- 시료별 재고 현황 ---\n";
    if (views.empty()) {
        std::cout << "등록된 시료가 없습니다.\n";
        return;
    }
    for (const Model::SampleStockView& view : views) {
        std::cout << view.sampleId << " | " << view.sampleName << " | 재고: " << view.stock
                  << " | 상태: " << Model::ToString(view.status) << "\n";
    }
}

}  // namespace View
