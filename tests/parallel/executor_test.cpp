/**
 * @file executor_test.cpp
 * @brief Tests for parallel execution utilities
 */

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <algorithm>
#include <atomic>
#include "parallel/executor.h"

namespace suzume {
namespace parallel {
namespace test {

// Test parallel map operation
TEST(ExecutorTest, ParallelMap) {
    // Input data
    std::vector<int> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Mapper function
    auto mapper = [](const int& x) { return x * 2; };

    // Run parallel map
    auto result = ParallelExecutor::parallelMap<int, int>(input, mapper);

    // Check result
    ASSERT_EQ(input.size(), result.size());
    for (size_t i = 0; i < input.size(); ++i) {
        EXPECT_EQ(input[i] * 2, result[i]);
    }
}

// Test parallel map with large input
TEST(ExecutorTest, ParallelMapLargeInput) {
    // Create large input
    std::vector<int> input(1000);
    for (int i = 0; i < 1000; ++i) {
        input[i] = i;
    }

    // Mapper function
    auto mapper = [](const int& x) { return x * 2; };

    // Run parallel map
    auto result = ParallelExecutor::parallelMap<int, int>(input, mapper, 4);

    // Check result
    ASSERT_EQ(input.size(), result.size());
    for (size_t i = 0; i < input.size(); ++i) {
        EXPECT_EQ(input[i] * 2, result[i]);
    }
}

// Test parallel for-each operation
TEST(ExecutorTest, ParallelForEach) {
    // Input data
    std::vector<int> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Result vector
    std::vector<int> result;
    std::mutex resultMutex;

    // Processor function
    auto processor = [&result, &resultMutex](const int& x) {
        std::lock_guard<std::mutex> lock(resultMutex);
        result.push_back(x * 2);
    };

    // Run parallel for-each
    ParallelExecutor::parallelForEach<int>(input, processor);

    // Check result
    ASSERT_EQ(input.size(), result.size());

    // Sort result (order may be different due to parallel execution)
    std::sort(result.begin(), result.end());

    // Check values
    for (size_t i = 0; i < input.size(); ++i) {
        EXPECT_EQ((i + 1) * 2, result[i]);
    }
}

// Test parallel for-each with progress callback
TEST(ExecutorTest, ParallelForEachWithProgress) {
    // Input data
    std::vector<int> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Result vector
    std::vector<int> result;
    std::mutex resultMutex;

    // Progress tracking
    bool callbackCalled = false;
    double lastProgress = 0.0;

    // Processor function
    auto processor = [&result, &resultMutex](const int& x) {
        std::lock_guard<std::mutex> lock(resultMutex);
        result.push_back(x * 2);
    };

    // Progress callback
    auto progressCallback = [&callbackCalled, &lastProgress](double progress) {
        callbackCalled = true;
        lastProgress = progress;
    };

    // Run parallel for-each with progress callback
    ParallelExecutor::parallelForEach<int>(input, processor, 4, progressCallback);

    // Check callback was called
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(1.0, lastProgress);

    // Check result
    ASSERT_EQ(input.size(), result.size());
}

// Test parallel reduce operation
TEST(ExecutorTest, ParallelReduce) {
    // Input data
    std::vector<int> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Reducer function
    auto reducer = [](int acc, const int& x) { return acc + x; };

    // Run parallel reduce
    int result = ParallelExecutor::parallelReduce<int, int>(input, reducer, 0);

    // Check result (sum of 1 to 10 = 55)
    EXPECT_EQ(55, result);
}

// Test parallel reduce with large input
TEST(ExecutorTest, ParallelReduceLargeInput) {
    // Create large input
    std::vector<int> input(1000);
    int expectedSum = 0;
    for (int i = 0; i < 1000; ++i) {
        input[i] = i;
        expectedSum += i;
    }

    // Reducer function
    auto reducer = [](int acc, const int& x) { return acc + x; };

    // Run parallel reduce
    int result = ParallelExecutor::parallelReduce<int, int>(input, reducer, 0, 4);

    // Check result
    EXPECT_EQ(expectedSum, result);
}

// Test parallel map with different types
TEST(ExecutorTest, ParallelMapDifferentTypes) {
    // Input data
    std::vector<std::string> input = {"1", "2", "3", "4", "5"};

    // Mapper function
    auto mapper = [](const std::string& x) { return std::stoi(x); };

    // Run parallel map
    auto result = ParallelExecutor::parallelMap<std::string, int>(input, mapper);

    // Check result
    ASSERT_EQ(input.size(), result.size());
    for (size_t i = 0; i < input.size(); ++i) {
        EXPECT_EQ(i + 1, result[i]);
    }
}

} // namespace test
} // namespace parallel
} // namespace suzume
