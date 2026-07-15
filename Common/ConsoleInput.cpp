#include "Common/ConsoleInput.h"

#include <iostream>
#include <limits>

namespace Common {

int ReadInt() {
    int value = 0;
    if (!(std::cin >> value)) {
        std::cin.clear();
        value = 0;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return value;
}

}  // namespace Common
