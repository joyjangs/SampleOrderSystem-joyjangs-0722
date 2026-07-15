#pragma once

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace Tests {

// Base fixture for tests that persist to a temp JSON file under data/.
// The file is removed in TearDown so test runs never leak state into
// each other or subsequent runs.
class TempFileFixture : public ::testing::Test {
protected:
    explicit TempFileFixture(std::string filePath) : path(std::move(filePath)) {}

    void TearDown() override { std::filesystem::remove(path); }

    std::string path;
};

}  // namespace Tests
