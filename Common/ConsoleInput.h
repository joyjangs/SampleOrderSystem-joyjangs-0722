#pragma once

namespace Common {

// Reads one int from std::cin. Returns 0 (and clears the failure state) if
// the input doesn't parse as a number, so callers can treat 0 as "invalid".
// This 0-on-failure contract is relied on by every View that uses ReadInt
// for a 1-based selection prompt (0 is never a valid selection index).
int ReadInt();

}  // namespace Common
