#include "View/SampleView.h"

#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

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

namespace {

// Re-prompts on a parse failure until it succeeds or the stream hits EOF.
// Returns false only on EOF (an interactive typo just re-prompts).
bool ReadDoubleWithRetry(const std::string& prompt, double& value) {
    while (true) {
        std::cout << prompt;
        if (std::cin >> value) {
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

}  // namespace

bool SampleView::ReadRegistrationInput(std::string& id, std::string& name, double& avgProductionTime,
                                        double& yieldRate) const {
    std::cout << "시료 ID: ";
    std::getline(std::cin, id);
    std::cout << "시료명: ";
    std::getline(std::cin, name);

    if (!ReadDoubleWithRetry("평균 생산시간(분/개): ", avgProductionTime)) {
        return false;
    }
    if (!ReadDoubleWithRetry("수율(0.0 초과 1.0 이하): ", yieldRate)) {
        return false;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return true;
}

void SampleView::ShowRegistrationResult(bool success, const std::string& message) const {
    std::cout << (success ? "등록 성공: " : "등록 실패: ") << message << "\n";
}

void SampleView::ShowError(const std::string& message) const { std::cout << message << "\n"; }

namespace {

// Formats a double with a fixed decimal precision for display (e.g. avg
// production time to 1 decimal place, yield rate to 2) — Sample itself
// stores the raw double; this is purely a console-rendering concern.
std::string FormatFixed(double value, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

void PrintSampleTable(const std::vector<Model::Sample>& samples) {
    std::cout << std::left << std::setw(10) << "ID" << std::setw(16) << "시료명" << std::setw(14)
              << "평균 생산시간" << std::setw(10) << "수율" << "재고" << "\n";
    for (const Model::Sample& sample : samples) {
        std::cout << std::left << std::setw(10) << sample.GetId() << std::setw(16) << sample.GetName()
                  << std::setw(14) << FormatFixed(sample.GetAvgProductionTime(), 1) << std::setw(10)
                  << FormatFixed(sample.GetYieldRate(), 2) << sample.GetStock() << "\n";
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
