#include "Model/MonitoringTypes.h"

namespace Model {

std::string ToString(StockStatus status) {
    switch (status) {
        case StockStatus::Sufficient:
            return "여유";
        case StockStatus::Shortage:
            return "부족";
        case StockStatus::Depleted:
            return "고갈";
    }
    return "알수없음";
}

}  // namespace Model
