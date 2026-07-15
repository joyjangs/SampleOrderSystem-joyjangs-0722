#include "View/Order/OrderView.h"

#include <iostream>
#include <limits>

namespace View {

std::string OrderView::PromptSampleId() const {
    std::cout << "시료 ID: ";
    std::string sampleId;
    std::getline(std::cin, sampleId);
    return sampleId;
}

std::string OrderView::PromptCustomerName() const {
    std::cout << "고객명: ";
    std::string customerName;
    std::getline(std::cin, customerName);
    return customerName;
}

bool OrderView::PromptQuantity(int& quantity) const {
    while (true) {
        std::cout << "주문 수량: ";
        if (std::cin >> quantity) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return true;
        }
        if (std::cin.eof()) {
            std::cin.clear();
            return false;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "숫자를 입력해주세요.\n";
    }
}

void OrderView::ShowOrderPlaced(const std::string& orderId, Model::OrderStatus status) const {
    std::cout << "주문 접수 완료: " << orderId << " (상태: " << Model::ToString(status) << ")\n";
}

void OrderView::ShowOrderRejectedInput(const std::string& reason) const {
    std::cout << "주문 접수 실패: " << reason << "\n";
}

}  // namespace View
