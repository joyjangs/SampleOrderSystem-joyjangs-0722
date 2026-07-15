#pragma once

#include <iostream>
#include <sstream>
#include <string>

namespace Tests {

// RAII guard that redirects std::cin to a scripted input string for the
// guard's lifetime, restoring the original streambuf on destruction.
class CinRedirectGuard {
public:
    explicit CinRedirectGuard(const std::string& scriptedInput) : input_(scriptedInput) {
        previousBuffer_ = std::cin.rdbuf(input_.rdbuf());
    }
    ~CinRedirectGuard() { std::cin.rdbuf(previousBuffer_); }

private:
    std::istringstream input_;
    std::streambuf* previousBuffer_;
};

// RAII guard that captures everything written to std::cout for the guard's
// lifetime, restoring the original streambuf on destruction.
class CoutRedirectGuard {
public:
    CoutRedirectGuard() { previousBuffer_ = std::cout.rdbuf(captured_.rdbuf()); }
    ~CoutRedirectGuard() { std::cout.rdbuf(previousBuffer_); }
    std::string Captured() const { return captured_.str(); }

private:
    std::ostringstream captured_;
    std::streambuf* previousBuffer_;
};

}  // namespace Tests
