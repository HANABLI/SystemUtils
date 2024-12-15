/**
 * @file CryptoRandomTests.cpp 
 * 
 * This module contains tests for SystemUtils::CryptoRandom class
 * 
 * © 2024 by Hatem Nabli
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <stdlib.h>
#include <vector>
#include <stdint.h>
#include <SystemUtils/CryptoRandom.hpp>

TEST(CryptoRandomTests, CryptoRandomTests_GenerateRandom_Test) {
    SystemUtils::CryptoRandom generator;
    std::vector< int > counts(256);
    constexpr size_t iterations = 100000000;
    for (size_t i = 0; i < iterations; i++) {
        uint8_t buffer;
        generator.Generate(&buffer, 1);
        ++counts[buffer];
    }
    int sum = 0;
    for (auto count: counts) {
        sum += count; 
    }
    const int average = sum / 256;
    EXPECT_NEAR(iterations / 256, average, 10);
    int largestDifference = 0;
    for (auto count: counts) {
        const auto difference = abs(count - average);
        largestDifference = std::max(largestDifference, difference);
        EXPECT_LE(difference, average / 20);
    }
}