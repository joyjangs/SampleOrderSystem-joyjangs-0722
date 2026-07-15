#include "View/Order/OrderApprovalView.h"

#include <iomanip>
#include <iostream>

#include "Common/ConsoleInput.h"
#include "Model/Order/OrderStatus.h"

namespace View {

void OrderApprovalView::ShowReservedOrders(const std::vector<Model::Order>& orders) const {
    std::cout << "----- 접수된 주문(RESERVED) -----\n";
    for (std::size_t i = 0; i < orders.size(); ++i) {
        const Model::Order& order = orders[i];
        std::cout << (i + 1) << ". " << order.GetOrderId() << " | " << order.GetSampleId() << " | "
                  << order.GetCustomerName() << " | " << order.GetQuantity() << "개 | " << order.GetCreatedAt()
                  << "\n";
    }
}

void OrderApprovalView::ShowNoReservedOrders() const { std::cout << "접수된(RESERVED) 주문이 없습니다.\n"; }

void OrderApprovalView::ShowNextToProcess(const Model::Order& order) const {
    std::cout << "처리 대상(접수 순서상 가장 오래된 주문): " << order.GetOrderId() << "\n";
}

int OrderApprovalView::PromptApproveOrReject() const {
    std::cout << "1. 승인  2. 거절\n선택: ";
    return Common::ReadInt();
}

void OrderApprovalView::ShowConfirmedImmediately(const std::string& orderId) const {
    std::cout << orderId << " 승인 완료: 재고가 충분해 즉시 출고 대기(CONFIRMED) 상태가 되었습니다.\n";
}

void OrderApprovalView::ShowQueuedForProduction(const std::string& orderId, int shortage, int actualQuantity,
                                                 double estimatedTime) const {
    std::cout << orderId << " 승인 완료: 재고 부족분 " << shortage << "개, 실 생산량 " << actualQuantity
              << "개로 생산 큐에 등록되었습니다(예상 생산시간: " << std::fixed << std::setprecision(1)
              << estimatedTime << "분).\n";
}

void OrderApprovalView::ShowRejected(const std::string& orderId) const {
    std::cout << orderId << " 주문이 거절 처리되었습니다.\n";
}

void OrderApprovalView::ShowInvalidSelection() const { std::cout << "잘못된 선택입니다.\n"; }

}  // namespace View
