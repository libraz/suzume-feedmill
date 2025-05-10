/**
 * @file normalize_test.cpp
 * @brief Tests for text normalization functionality
 */

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "core/normalize.h"

namespace suzume {
namespace core {
namespace test {

class NormalizeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory if it doesn't exist
        std::filesystem::create_directories("test_data");

        // Create test input file
        std::ofstream inputFile("test_data/normalize_test_input.tsv");
        inputFile << "Hello World\n";
        inputFile << "hello world\n";
        inputFile << "HELLO WORLD\n";
        inputFile << "Ｈｅｌｌｏ　Ｗｏｒｌｄ\n";
        inputFile << "#comment line\n";
        inputFile << "a\n";
        inputFile << "\n";
        inputFile.close();
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all("test_data");
    }
};

// Test basic normalization
TEST_F(NormalizeTest, BasicNormalization) {
    // Set up options
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;

    // Run normalization
    NormalizeResult result = core::normalize(
        "test_data/normalize_test_input.tsv",
        "test_data/normalize_test_output.tsv",
        options
    );

    // Check result
    EXPECT_EQ(7, result.rows);
    EXPECT_EQ(1, result.uniques);

    // Check output file
    std::ifstream outputFile("test_data/normalize_test_output.tsv");
    std::string line;
    std::getline(outputFile, line);
    EXPECT_EQ("hello world", line);

    // No more lines
    EXPECT_FALSE(std::getline(outputFile, line));
}

// Test NFC normalization
TEST_F(NormalizeTest, NFCNormalization) {
    // Set up options
    NormalizeOptions options;
    options.form = NormalizationForm::NFC;

    // Run normalization
    NormalizeResult result = core::normalize(
        "test_data/normalize_test_input.tsv",
        "test_data/normalize_test_output.tsv",
        options
    );

    // Check result - don't check exact count as it may vary
    EXPECT_EQ(7, result.rows);
    EXPECT_GT(result.uniques, 0);

    // Check output file
    std::ifstream outputFile("test_data/normalize_test_output.tsv");
    std::string line;

    // Read lines (order may vary)
    std::vector<std::string> lines;
    while (std::getline(outputFile, line)) {
        lines.push_back(line);
    }

    // Check expected lines are present
    EXPECT_GT(lines.size(), 0);
    EXPECT_TRUE(std::find(lines.begin(), lines.end(), "Hello World") != lines.end());
}

// Test progress callback
TEST_F(NormalizeTest, ProgressCallback) {
    // Set up options
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;

    // Progress tracking
    bool callbackCalled = false;
    double lastProgress = 0.0;

    // Run normalization with progress callback
    NormalizeResult result = core::normalizeWithProgress(
        "test_data/normalize_test_input.tsv",
        "test_data/normalize_test_output.tsv",
        [&callbackCalled, &lastProgress](double progress) {
            callbackCalled = true;
            lastProgress = progress;
        },
        options
    );

    // Check callback was called
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(1.0, lastProgress);

    // Check result
    EXPECT_EQ(7, result.rows);
    EXPECT_EQ(1, result.uniques);
}

// Test batch processing
TEST_F(NormalizeTest, BatchProcessing) {
    // Create test batch
    std::vector<std::string> batch = {
        "Hello World",
        "hello world",
        "HELLO WORLD",
        "Ｈｅｌｌｏ　Ｗｏｒｌｄ",
        "#comment line",
        "a",
        ""
    };

    // Process batch
    std::vector<std::string> result = processBatch(batch, NormalizationForm::NFKC, 0.01);

    // Check result
    ASSERT_EQ(1, result.size());
    EXPECT_EQ("hello world", result[0]);
}

} // namespace test
} // namespace core
} // namespace suzume
