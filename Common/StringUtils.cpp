#include "Common/StringUtils.h"

#include <cctype>

namespace Common {

bool IsBlank(const std::string& text) {
    for (char c : text) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

}  // namespace Common
