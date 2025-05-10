/**
 * @file progress_callback_test.cpp
 * @brief Tests for progress callback functionality
 */

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include "core/normalize.h"
#include "core/pmi.h"

namespace suzume {
namespace core {
namespace test {

class ProgressCallbackTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory if it doesn't exist
        std::filesystem::create_directories("test_data");

        // Create a larger test file to ensure multiple progress updates
        std::ofstream inputFile("test_data/progress_test_input.txt");
        for (int i = 0; i < 100; i++) {
            inputFile << "Line " << i << ": This is a test line with some text to process.\n";
        }
        inputFile.close();
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all("test_data");
    }
};

// Test that progress callback is called multiple times with increasing values
TEST_F(ProgressCallbackTest, ProgressIncreases) {
    // Set up options
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;

    // Progress tracking
    std::vector<double> progressValues;

    // Run normalization with progress callback
    NormalizeResult result = normalizeWithProgress(
        "test_data/progress_test_input.txt",
        "test_data/progress_test_output.txt",
        [&progressValues](double progress) {
            progressValues.push_back(progress);
        },
        options
    );

    // Check that callback was called multiple times
    EXPECT_GT(progressValues.size(), 1);

    // Check that progress values are increasing
    for (size_t i = 1; i < progressValues.size(); i++) {
        EXPECT_GE(progressValues[i], progressValues[i-1]);
    }

    // Check that final progress is 1.0
    if (!progressValues.empty()) {
        EXPECT_EQ(1.0, progressValues.back());
    }

    // Check result
    EXPECT_EQ(100, result.rows);
}

// Test that PMI progress callback is called multiple times with increasing values
TEST_F(ProgressCallbackTest, PmiProgressIncreases) {
    // Set up options
    PmiOptions options;
    options.n = 2;
    options.topK = 10;
    options.minFreq = 2;

    // Progress tracking
    std::vector<double> progressValues;

    // Run PMI calculation with progress callback
    PmiResult result = calculatePmiWithProgress(
        "test_data/progress_test_input.txt",
        "test_data/progress_test_output.tsv",
        [&progressValues](double progress) {
            progressValues.push_back(progress);
        },
        options
    );

    // Check that callback was called multiple times
    EXPECT_GT(progressValues.size(), 1);

    // Check that progress values are increasing
    for (size_t i = 1; i < progressValues.size(); i++) {
        EXPECT_GE(progressValues[i], progressValues[i-1]);
    }

    // Check that final progress is 1.0
    if (!progressValues.empty()) {
        EXPECT_EQ(1.0, progressValues.back());
    }

    // Check result
    EXPECT_GT(result.grams, 0);
}

// Test that progress callback that throws an exception is handled properly
TEST_F(ProgressCallbackTest, ExceptionInCallback) {
    // Set up options
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;

    // Run normalization with a callback that throws an exception
    EXPECT_THROW({
        normalizeWithProgress(
            "test_data/progress_test_input.txt",
            "test_data/progress_test_output.txt",
            [](double /* progress */) {
                throw std::runtime_error("Test exception from progress callback");
            },
            options
        );
    }, std::runtime_error);
}

// Test that PMI progress callback that throws an exception is handled properly
TEST_F(ProgressCallbackTest, PmiExceptionInCallback) {
    // Set up options
    PmiOptions options;
    options.n = 2;
    options.topK = 10;
    options.minFreq = 2;

    // Run PMI calculation with a callback that throws an exception
    EXPECT_THROW({
        calculatePmiWithProgress(
            "test_data/progress_test_input.txt",
            "test_data/progress_test_output.tsv",
            [](double /* progress */) {
                throw std::runtime_error("Test exception from progress callback");
            },
            options
        );
    }, std::runtime_error);
}

// Test progress callback with different thread counts
TEST_F(ProgressCallbackTest, DifferentThreadCounts) {
    // Test with different thread counts
    std::vector<uint32_t> threadCounts = {1, 2, 4};

    for (uint32_t threads : threadCounts) {
        // Set up options
        NormalizeOptions options;
        options.form = NormalizationForm::NFKC;
        options.threads = threads;

        // Progress tracking
        std::vector<double> progressValues;

        // Run normalization with progress callback
        NormalizeResult result = normalizeWithProgress(
            "test_data/progress_test_input.txt",
            "test_data/progress_test_output.txt",
            [&progressValues](double progress) {
                progressValues.push_back(progress);
            },
            options
        );

        // Check that callback was called multiple times
        EXPECT_GT(progressValues.size(), 1);

        // Check that progress values are increasing
        for (size_t i = 1; i < progressValues.size(); i++) {
            EXPECT_GE(progressValues[i], progressValues[i-1]);
        }

        // Check that final progress is 1.0
        if (!progressValues.empty()) {
            EXPECT_EQ(1.0, progressValues.back());
        }

        // Check result
        EXPECT_EQ(100, result.rows);
    }
}

} // namespace test
} // namespace core
} // namespace suzume
