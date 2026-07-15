#pragma once

namespace Common {

// Reads one int from std::cin. Returns 0 (and clears the failure state) if
// the input doesn't parse as a number, so callers can treat 0 as "invalid".
int ReadInt();

}  // namespace Common
