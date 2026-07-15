#pragma once

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

namespace Tests {

// Base fixture for tests that persist to temp JSON file(s) under data/.
// Every path passed to the constructor is removed in TearDown, so test runs
// never leak state into each other or subsequent runs.
class TempFileFixture : public ::testing::Test {
protected:
    explicit TempFileFixture(std::string singlePath) : path(std::move(singlePath)), paths_{path} {}
    explicit TempFileFixture(std::vector<std::string> multiplePaths) : paths_(std::move(multiplePaths)) {}

    void TearDown() override {
        for (const auto& path : paths_) {
            std::filesystem::remove(path);
        }
    }

    // Convenience member for the common single-file case.
    std::string path;

private:
    std::vector<std::string> paths_;
};

}  // namespace Tests
