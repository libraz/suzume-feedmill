/**
 * @file pmi_test.cpp
 * @brief Tests for PMI calculation functionality
 */

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include "core/pmi.h"

namespace suzume {
namespace core {
namespace test {

class PmiTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory if it doesn't exist
        std::filesystem::create_directories("test_data");

        // Create test input file
        std::ofstream inputFile("test_data/pmi_test_input.txt");
        inputFile << "This is a test sentence for PMI calculation.\n";
        inputFile << "This is another test sentence with some repeated words.\n";
        inputFile << "PMI calculation requires sufficient text data.\n";
        inputFile << "The more text data, the better the PMI calculation.\n";
        inputFile << "Test sentences with repeated words help PMI calculation.\n";
        inputFile.close();
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all("test_data");
    }
};

// Test basic PMI calculation
TEST_F(PmiTest, BasicPmiCalculation) {
    // Set up options
    PmiOptions options;
    options.n = 2;
    options.topK = 10;
    options.minFreq = 2;

    // Run PMI calculation
    PmiResult result = core::calculatePmi(
        "test_data/pmi_test_input.txt",
        "test_data/pmi_test_output.tsv",
        options
    );

    // Check result
    EXPECT_GT(result.grams, 0);

    // Check output file exists
    EXPECT_TRUE(std::filesystem::exists("test_data/pmi_test_output.tsv"));

    // Check output file has content
    std::ifstream outputFile("test_data/pmi_test_output.tsv");
    std::string line;

    // First line should be header
    EXPECT_TRUE(std::getline(outputFile, line));
    EXPECT_EQ("ngram\tpmi\tfrequency", line);

    // Should have some data lines
    EXPECT_TRUE(std::getline(outputFile, line));

    // Check data line format (should have 3 columns)
    size_t firstTab = line.find('\t');
    EXPECT_NE(std::string::npos, firstTab);

    size_t secondTab = line.find('\t', firstTab + 1);
    EXPECT_NE(std::string::npos, secondTab);

    // No third tab should exist
    EXPECT_EQ(std::string::npos, line.find('\t', secondTab + 1));

    // Check column values
    std::string ngram = line.substr(0, firstTab);
    std::string pmiStr = line.substr(firstTab + 1, secondTab - firstTab - 1);
    std::string freqStr = line.substr(secondTab + 1);

    EXPECT_FALSE(ngram.empty());
    EXPECT_FALSE(pmiStr.empty());
    EXPECT_FALSE(freqStr.empty());

    // Frequency should be a positive integer
    int freq = std::stoi(freqStr);
    EXPECT_GT(freq, 0);

    // Count total number of data lines
    int lineCount = 1; // Already read one line
    while (std::getline(outputFile, line)) {
        lineCount++;
    }

    // Should have at most topK lines of data
    EXPECT_LE(lineCount, options.topK);
}

// Test different n-gram sizes
TEST_F(PmiTest, DifferentNgramSizes) {
    // Test unigrams
    {
        PmiOptions options;
        options.n = 1;
        options.topK = 10;
        options.minFreq = 2;

        PmiResult result = core::calculatePmi(
            "test_data/pmi_test_input.txt",
            "test_data/pmi_test_output_1.tsv",
            options
        );

        EXPECT_GT(result.grams, 0);
        EXPECT_TRUE(std::filesystem::exists("test_data/pmi_test_output_1.tsv"));
    }

    // Test trigrams
    {
        PmiOptions options;
        options.n = 3;
        options.topK = 10;
        options.minFreq = 1; // Lower threshold for test data

        PmiResult result = core::calculatePmi(
            "test_data/pmi_test_input.txt",
            "test_data/pmi_test_output_3.tsv",
            options
        );

        EXPECT_GT(result.grams, 0);
        EXPECT_TRUE(std::filesystem::exists("test_data/pmi_test_output_3.tsv"));
    }
}

// Test topK parameter
TEST_F(PmiTest, TopKParameter) {
    // Create a larger test file to ensure we have enough n-grams
    std::ofstream largerInputFile("test_data/pmi_topk_test_input.txt");
    for (int i = 0; i < 20; i++) {
        largerInputFile << "This is test sentence number " << i << " for PMI calculation.\n";
        largerInputFile << "We need multiple sentences with different words to test topK parameter.\n";
        largerInputFile << "Adding more text data improves the quality of PMI calculation results.\n";
        largerInputFile << "The more varied the vocabulary, the better the test coverage.\n";
    }
    largerInputFile.close();

    // Test with different topK values
    std::vector<uint32_t> topKValues = {5, 10, 20};

    for (uint32_t topK : topKValues) {
        PmiOptions options;
        options.n = 2;
        options.topK = topK;
        options.minFreq = 2;

        std::string outputPath = "test_data/pmi_topk_" + std::to_string(topK) + ".tsv";

        PmiResult result = core::calculatePmi(
            "test_data/pmi_topk_test_input.txt",
            outputPath,
            options
        );

        EXPECT_GT(result.grams, 0);
        EXPECT_TRUE(std::filesystem::exists(outputPath));

        // Check output file
        std::ifstream outputFile(outputPath);
        std::string line;

        // First line should be header
        EXPECT_TRUE(std::getline(outputFile, line));
        EXPECT_EQ("ngram\tpmi\tfrequency", line);

        // Count data lines
        int lineCount = 0;
        while (std::getline(outputFile, line)) {
            lineCount++;
        }

        // Should have at most topK lines of data
        EXPECT_LE(lineCount, topK);

        // For larger topK values, we should get more results
        // (assuming we have enough n-grams in the input)
        if (topK > 5) {
            EXPECT_GT(lineCount, 5);
        }
    }
}

// Test output format consistency
TEST_F(PmiTest, OutputFormatConsistency) {
    // Set up options
    PmiOptions options;
    options.n = 2;
    options.topK = 10;
    options.minFreq = 2;

    // Run PMI calculation
    core::calculatePmi(
        "test_data/pmi_test_input.txt",
        "test_data/pmi_format_test.tsv",
        options
    );

    // Check output file
    std::ifstream outputFile("test_data/pmi_format_test.tsv");
    std::string line;

    // First line should be header
    EXPECT_TRUE(std::getline(outputFile, line));
    EXPECT_EQ("ngram\tpmi\tfrequency", line);

    // Check all data lines for consistent format
    while (std::getline(outputFile, line)) {
        // Should have exactly 2 tabs (3 columns)
        size_t firstTab = line.find('\t');
        EXPECT_NE(std::string::npos, firstTab);

        size_t secondTab = line.find('\t', firstTab + 1);
        EXPECT_NE(std::string::npos, secondTab);

        // No third tab should exist
        EXPECT_EQ(std::string::npos, line.find('\t', secondTab + 1));

        // Extract columns
        std::string ngram = line.substr(0, firstTab);
        std::string pmiStr = line.substr(firstTab + 1, secondTab - firstTab - 1);
        std::string freqStr = line.substr(secondTab + 1);

        // Validate columns
        EXPECT_FALSE(ngram.empty());
        EXPECT_FALSE(pmiStr.empty());
        EXPECT_FALSE(freqStr.empty());

        // PMI should be a floating-point number
        try {
            double pmi = std::stod(pmiStr);
            (void)pmi; // Avoid unused variable warning
        } catch (const std::exception& e) {
            FAIL() << "PMI value is not a valid number: " << pmiStr;
        }

        // Frequency should be a positive integer
        try {
            int freq = std::stoi(freqStr);
            EXPECT_GT(freq, 0);
        } catch (const std::exception& e) {
            FAIL() << "Frequency value is not a valid integer: " << freqStr;
        }
    }
}

// Test progress callback
TEST_F(PmiTest, ProgressCallback) {
    // Set up options
    PmiOptions options;
    options.n = 2;
    options.topK = 10;
    options.minFreq = 2;

    // Progress tracking
    bool callbackCalled = false;
    double lastProgress = 0.0;

    // Run PMI calculation with progress callback
    PmiResult result = core::calculatePmiWithProgress(
        "test_data/pmi_test_input.txt",
        "test_data/pmi_test_output.tsv",
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
    EXPECT_GT(result.grams, 0);
}

// Test n-gram counting
TEST_F(PmiTest, NgramCounting) {
    // Create a simple test string with a single line
    std::string text = "This is a test";

    // Count n-grams
    auto counts = countNgrams(text, 2);

    // Debug output is commented out to keep test output clean
    // Uncomment for debugging if needed
    /*
    std::cout << "Found n-grams:" << std::endl;
    for (const auto& [ngram, count] : counts) {
        std::cout << "  '" << ngram << "': " << count << std::endl;
    }
    */

    // Check that counts are not empty
    EXPECT_GT(counts.size(), 0);

    // Just check that we have some n-grams
    EXPECT_TRUE(!counts.empty());
}

// Test PMI score calculation
TEST_F(PmiTest, PmiScoreCalculation) {
    // Create n-gram counts
    std::unordered_map<std::string, uint32_t> counts;
    counts["This"] = 2;
    counts["is"] = 3;
    counts["a"] = 2;
    counts["test"] = 2;
    counts["This is"] = 2;
    counts["is a"] = 2;
    counts["a test"] = 2;

    // Calculate PMI scores
    auto scores = calculatePmiScores(counts, 2, 1);

    // Check scores
    EXPECT_GT(scores.size(), 0);

    // Verify each score has the expected fields
    for (const auto& score : scores) {
        EXPECT_FALSE(score.ngram.empty());
        EXPECT_GT(score.frequency, 0);
    }
}

} // namespace test
} // namespace core
} // namespace suzume
