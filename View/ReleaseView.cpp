#include "View/ReleaseView.h"

#include <iostream>

#include "Common/ConsoleInput.h"

namespace View {

void ReleaseView::ShowConfirmedOrders(const std::vector<Model::Order>& orders) const {
    std::cout << "----- 출고 대기 주문(CONFIRMED) -----\n";
    for (std::size_t i = 0; i < orders.size(); ++i) {
        const Model::Order& order = orders[i];
        std::cout << (i + 1) << ". " << order.GetOrderId() << " | " << order.GetSampleId() << " | "
                  << order.GetCustomerName() << " | " << order.GetQuantity() << "개\n";
    }
}

void ReleaseView::ShowNoConfirmedOrders() const { std::cout << "출고 대기(CONFIRMED) 주문이 없습니다.\n"; }

int ReleaseView::PromptOrderSelection() const {
    std::cout << "출고 처리할 주문 번호를 선택하세요: ";
    return Common::ReadInt();
}

void ReleaseView::ShowReleaseSuccess(const std::string& orderId) const {
    std::cout << orderId << " 출고 처리 완료(RELEASE).\n";
}

void ReleaseView::ShowReleaseFailure(const std::string& reason) const {
    std::cout << "출고 처리 실패: " << reason << "\n";
}

void ReleaseView::ShowInvalidSelection() const { std::cout << "잘못된 선택입니다.\n"; }

}  // namespace View
