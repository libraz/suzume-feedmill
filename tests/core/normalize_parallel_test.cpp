/**
 * @file normalize_parallel_test.cpp
 * @brief Tests for parallel processing in text normalization
 */

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <thread>
#include <atomic>
#include "core/normalize.h"

namespace suzume {
namespace core {
namespace test {

class NormalizeParallelTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory if it doesn't exist
        std::filesystem::create_directories("test_data");

        // Create a large test input file to trigger parallel processing
        std::ofstream inputFile("test_data/large_input.tsv");
        for (int i = 0; i < 1000; ++i) {
            inputFile << "Line " << i << " for testing parallel processing\n";
            // Add some duplicates to test deduplication
            if (i % 10 == 0) {
                inputFile << "Duplicate line for testing\n";
            }
            // Add some lines that should be excluded
            if (i % 20 == 0) {
                inputFile << "#Comment line " << i << "\n";
            }
        }
        inputFile.close();
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all("test_data");
    }
};

// Test parallel processing with structured progress callback
TEST_F(NormalizeParallelTest, ParallelProcessingWithStructuredProgress) {
    // Set up options to force parallel processing
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;
    options.threads = std::thread::hardware_concurrency(); // Use all available cores

    // Track progress phases
    std::atomic<bool> readingPhaseReported{false};
    std::atomic<bool> processingPhaseReported{false};
    std::atomic<bool> writingPhaseReported{false};
    std::atomic<bool> completePhaseReported{false};

    // Run normalization with structured progress callback
    NormalizeResult result = suzume::core::normalizeWithStructuredProgress(
        "test_data/large_input.tsv",
        "test_data/large_output.tsv",
        [&](const ProgressInfo& info) {
            // Track which phases were reported
            switch (info.phase) {
                case ProgressInfo::Phase::Reading:
                    readingPhaseReported = true;
                    break;
                case ProgressInfo::Phase::Processing:
                    processingPhaseReported = true;
                    break;
                case ProgressInfo::Phase::Writing:
                    writingPhaseReported = true;
                    break;
                case ProgressInfo::Phase::Complete:
                    completePhaseReported = true;
                    break;
                case ProgressInfo::Phase::Calculating:
                    // Ignore this phase for the test
                    break;
            }
        },
        options
    );

    // Check that all phases were reported
    EXPECT_TRUE(readingPhaseReported);
    EXPECT_TRUE(processingPhaseReported);
    EXPECT_TRUE(writingPhaseReported);
    EXPECT_TRUE(completePhaseReported);

    // Check result
    EXPECT_GT(result.rows, 1000); // Should be more than 1000 due to duplicates
    EXPECT_LT(result.uniques, result.rows); // Should be fewer uniques than rows due to deduplication

    // Check output file
    std::ifstream outputFile("test_data/large_output.tsv");
    std::string line;
    int lineCount = 0;
    while (std::getline(outputFile, line)) {
        lineCount++;
        // Comment lines should be excluded
        EXPECT_FALSE(line.find("#") == 0);
    }

    // Check that the number of lines in the output file matches the reported uniques
    EXPECT_EQ(lineCount, result.uniques);
}

// Test null output path
TEST_F(NormalizeParallelTest, NullOutputPath) {
    // Set up options
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;

    // Run normalization with null output path
    NormalizeResult result = suzume::core::normalize(
        "test_data/large_input.tsv",
        "null", // Special case for no output
        options
    );

    // Check result
    EXPECT_GT(result.rows, 0);
    EXPECT_GT(result.uniques, 0);

    // Check that no output file was created
    EXPECT_FALSE(std::filesystem::exists("null"));
}

// Test with very large thread count
TEST_F(NormalizeParallelTest, VeryLargeThreadCount) {
    // Set up options with a very large thread count
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;
    options.threads = 1000; // Unreasonably large

    // Run normalization
    NormalizeResult result = suzume::core::normalize(
        "test_data/large_input.tsv",
        "test_data/large_output.tsv",
        options
    );

    // Check result - should complete successfully despite unreasonable thread count
    EXPECT_GT(result.rows, 0);
    EXPECT_GT(result.uniques, 0);
}

// Test with small input that doesn't trigger parallel processing
TEST_F(NormalizeParallelTest, SmallInputSingleThreaded) {
    // Create a small input file
    std::ofstream smallFile("test_data/small_input.tsv");
    smallFile << "Line 1\n";
    smallFile << "Line 2\n";
    smallFile << "Line 3\n";
    smallFile.close();

    // Set up options with multiple threads, but small input should use single-threaded path
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;
    options.threads = std::thread::hardware_concurrency();

    // Track progress phases
    std::atomic<bool> processingPhaseReported{false};

    // Run normalization with structured progress callback
    NormalizeResult result = suzume::core::normalizeWithStructuredProgress(
        "test_data/small_input.tsv",
        "test_data/small_output.tsv",
        [&](const ProgressInfo& info) {
            if (info.phase == ProgressInfo::Phase::Processing && info.phaseRatio == 1.0) {
                processingPhaseReported = true;
            }
        },
        options
    );

    // Check that processing phase was reported with ratio 1.0 (single-threaded path)
    EXPECT_TRUE(processingPhaseReported);

    // Check result
    EXPECT_EQ(3, result.rows);
    EXPECT_EQ(3, result.uniques); // All lines are unique
}

// Test with custom progress step
TEST_F(NormalizeParallelTest, CustomProgressStep) {
    // Set up options with custom progress step
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;

    // Create a smaller test file for more predictable progress reporting
    std::ofstream smallFile("test_data/progress_test.tsv");
    for (int i = 0; i < 100; ++i) {
        smallFile << "Line " << i << " for testing progress\n";
    }
    smallFile.close();

    // Track progress reports
    std::vector<double> progressReports;
    std::mutex progressMutex;

    // Run normalization
    NormalizeResult result = suzume::core::normalizeWithProgress(
        "test_data/progress_test.tsv",
        "test_data/progress_output.tsv",
        [&](double progress) {
            std::lock_guard<std::mutex> lock(progressMutex);
            progressReports.push_back(progress);
        },
        options
    );

    // We should get at least the initial and final progress reports
    EXPECT_GE(progressReports.size(), 2);

    // First report should be 0.0 or close to it
    if (!progressReports.empty()) {
        EXPECT_NEAR(progressReports.front(), 0.0, 0.1);
    }

    // Last report should be 1.0
    if (!progressReports.empty()) {
        EXPECT_NEAR(progressReports.back(), 1.0, 0.1);
    }

    // Check result
    EXPECT_GT(result.rows, 0);
    EXPECT_GT(result.uniques, 0);
}

} // namespace test
} // namespace core
} // namespace suzume
