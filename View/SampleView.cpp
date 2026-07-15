#include "View/SampleView.h"

#include <iomanip>
#include <iostream>
#include <limits>

namespace View {

void SampleView::ShowMenu() const {
    std::cout << "\n----- 시료 관리 -----\n"
              << "1. 시료 등록\n"
              << "2. 시료 목록 조회\n"
              << "3. 이름으로 검색\n"
              << "0. 뒤로 가기\n"
              << "선택: ";
}

int SampleView::ReadMenuChoice() const {
    int choice = -1;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        choice = -1;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return choice;
}

bool SampleView::ReadRegistrationInput(std::string& id, std::string& name, double& avgProductionTime,
                                        double& yieldRate) const {
    std::cout << "시료 ID: ";
    std::getline(std::cin, id);
    std::cout << "시료명: ";
    std::getline(std::cin, name);

    std::cout << "평균 생산시간(분/개): ";
    if (!(std::cin >> avgProductionTime)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return false;
    }
    std::cout << "수율(0.0 초과 1.0 이하): ";
    if (!(std::cin >> yieldRate)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return false;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return true;
}

void SampleView::ShowRegistrationResult(bool success, const std::string& message) const {
    std::cout << (success ? "등록 성공: " : "등록 실패: ") << message << "\n";
}

namespace {

void PrintSampleTable(const std::vector<Model::Sample>& samples) {
    std::cout << std::left << std::setw(10) << "ID" << std::setw(16) << "시료명" << std::setw(14)
              << "평균 생산시간" << std::setw(10) << "수율" << "재고" << "\n";
    for (const Model::Sample& sample : samples) {
        std::cout << std::left << std::setw(10) << sample.GetId() << std::setw(16) << sample.GetName()
                  << std::setw(14) << sample.GetAvgProductionTime() << std::setw(10) << sample.GetYieldRate()
                  << sample.GetStock() << "\n";
    }
}

}  // namespace

void SampleView::ShowSampleList(const std::vector<Model::Sample>& samples) const {
    if (samples.empty()) {
        std::cout << "등록된 시료가 없습니다.\n";
        return;
    }
    PrintSampleTable(samples);
}

std::string SampleView::ReadSearchKeyword() const {
    std::cout << "검색어: ";
    std::string keyword;
    std::getline(std::cin, keyword);
    return keyword;
}

void SampleView::ShowSearchResult(const std::vector<Model::Sample>& results,
                                   const std::string& keyword) const {
    std::cout << "\"" << keyword << "\" 검색 결과:\n";
    if (results.empty()) {
        std::cout << "일치하는 시료가 없습니다.\n";
        return;
    }
    PrintSampleTable(results);
}

}  // namespace View
