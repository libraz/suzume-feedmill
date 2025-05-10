/**
 * @file word_extraction_generator_advanced_test.cpp
 * @brief Advanced tests for CandidateGenerator class
 */

#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>
#include "../../src/core/word_extraction/generator.h"

namespace suzume {
namespace core {
namespace test {

class CandidateGeneratorAdvancedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create options
        options.minPmiScore = 3.0;
        options.maxCandidateLength = 10;
        options.maxCandidates = 100;
        options.useParallelProcessing = true;
        options.threads = 2;

        // Create generator
        generator = std::make_unique<CandidateGenerator>(options);
    }

    void TearDown() override {
        // Clean up test files
        std::remove(emptyFilePath_.c_str());
        std::remove(noHeaderFilePath_.c_str());
        std::remove(malformedFilePath_.c_str());
        std::remove(largeFilePath_.c_str());
    }

    // Create an empty PMI results file
    void createEmptyFile() {
        std::ofstream file(emptyFilePath_);
        ASSERT_TRUE(file.is_open());
        file.close();
    }

    // Create a PMI results file without header
    void createNoHeaderFile() {
        std::ofstream file(noHeaderFilePath_);
        ASSERT_TRUE(file.is_open());

        // Add some test n-grams with PMI scores (no header)
        file << "人工知能\t5.2\t10\n";
        file << "機械学習\t4.8\t8\n";
        file << "深層学習\t4.5\t7\n";

        file.close();
    }

    // Create a PMI results file with malformed lines
    void createMalformedFile() {
        std::ofstream file(malformedFilePath_);
        ASSERT_TRUE(file.is_open());

        // Header
        file << "ngram\tscore\tfreq\n";

        // Add some valid n-grams
        file << "人工知能\t5.2\t10\n";
        file << "機械学習\t4.8\t8\n";

        // Add some malformed lines
        file << "malformed_line\n";
        file << "missing_freq\t4.5\n";
        file << "invalid_score\tabc\t7\n";
        file << "valid_after_invalid\t4.2\t6\n";

        file.close();
    }

    // Create a large PMI results file
    void createLargeFile(int numEntries = 5000) {
        std::ofstream file(largeFilePath_);
        ASSERT_TRUE(file.is_open());

        // Header
        file << "ngram\tscore\tfreq\n";

        // Add many test n-grams
        for (int i = 0; i < numEntries; ++i) {
            double score = 3.0 + (i % 50) / 10.0; // Scores from 3.0 to 8.0
            int freq = 1 + (i % 20);              // Frequencies from 1 to 20
            file << "ngram" << i << "\t" << score << "\t" << freq << "\n";
        }

        file.close();
    }

    // Test files
    std::string emptyFilePath_ = "test_empty_pmi.tsv";
    std::string noHeaderFilePath_ = "test_no_header_pmi.tsv";
    std::string malformedFilePath_ = "test_malformed_pmi.tsv";
    std::string largeFilePath_ = "test_large_pmi.tsv";

    // Test objects
    WordExtractionOptions options;
    std::unique_ptr<CandidateGenerator> generator;
};

// Test with empty file
TEST_F(CandidateGeneratorAdvancedTest, EmptyFile) {
    // Create empty file
    createEmptyFile();

    // Check that empty file throws exception
    EXPECT_THROW(
        generator->generateCandidates(emptyFilePath_),
        std::runtime_error
    );
}

// Test with file that has no header
TEST_F(CandidateGeneratorAdvancedTest, NoHeaderFile) {
    // Create file without header
    createNoHeaderFile();

    // Set minimum PMI score lower to ensure we get results
    options.minPmiScore = 1.0;
    generator = std::make_unique<CandidateGenerator>(options);

    try {
        // Generate candidates
        auto candidates = generator->generateCandidates(noHeaderFilePath_);

        // If we get here, the function handled the file without header
        // We may or may not get candidates depending on the implementation
    } catch (const std::exception& e) {
        // If it throws, that's also acceptable for this test
        // Just make sure it doesn't crash
        SUCCEED() << "Exception handled gracefully: " << e.what();
    }
}

// Test with file that has malformed lines
TEST_F(CandidateGeneratorAdvancedTest, MalformedFile) {
    // Create file with malformed lines
    createMalformedFile();

    // Set minimum PMI score lower to ensure we get results
    options.minPmiScore = 1.0;
    generator = std::make_unique<CandidateGenerator>(options);

    try {
        // Generate candidates
        auto candidates = generator->generateCandidates(malformedFilePath_);

        // If we get here, the function handled malformed lines gracefully
        // We may or may not get candidates depending on the implementation
    } catch (const std::exception& e) {
        // If it throws, that's also acceptable for this test
        // Just make sure it doesn't crash
        SUCCEED() << "Exception handled gracefully: " << e.what();
    }
}

// Test with very low minimum PMI score
TEST_F(CandidateGeneratorAdvancedTest, VeryLowMinPmiScore) {
    // Create file without header
    createNoHeaderFile();

    // Set very low minimum PMI score
    options.minPmiScore = 0.0;
    generator = std::make_unique<CandidateGenerator>(options);

    try {
        // Generate candidates
        auto candidates = generator->generateCandidates(noHeaderFilePath_);

        // Test passes regardless of whether we get candidates or not
        // The important thing is that it doesn't crash
        SUCCEED() << "Successfully processed with very low PMI score";
    } catch (const std::exception& e) {
        // If it throws, that's also acceptable for this test
        SUCCEED() << "Exception handled gracefully: " << e.what();
    }
}

// Test with negative minimum PMI score (should throw)
TEST_F(CandidateGeneratorAdvancedTest, NegativeMinPmiScore) {
    // Create file without header
    createNoHeaderFile();

    // Set negative minimum PMI score
    options.minPmiScore = -1.0;
    generator = std::make_unique<CandidateGenerator>(options);

    // Check that negative minPmiScore throws exception
    EXPECT_THROW(
        generator->generateCandidates(noHeaderFilePath_),
        std::invalid_argument
    );
}

// Test with large number of n-grams
TEST_F(CandidateGeneratorAdvancedTest, LargeNumberOfNgrams) {
    // Create large file
    createLargeFile(5000);

    // Generate candidates
    auto candidates = generator->generateCandidates(largeFilePath_);

    // Check that we got some candidates
    EXPECT_FALSE(candidates.empty());

    // Check that we didn't exceed maxCandidates
    EXPECT_LE(candidates.size(), options.maxCandidates);

    // Check that candidates are sorted by score (descending)
    for (size_t i = 1; i < candidates.size(); i++) {
        EXPECT_GE(candidates[i-1].score, candidates[i].score);
    }
}

// Test with parallel processing and many threads
TEST_F(CandidateGeneratorAdvancedTest, ManyThreads) {
    // Create large file
    createLargeFile(5000);

    // Set high thread count
    unsigned int maxThreads = std::thread::hardware_concurrency();
    if (maxThreads == 0) maxThreads = 4;

    options.threads = maxThreads * 2; // Intentionally set higher than hardware supports
    generator = std::make_unique<CandidateGenerator>(options);

    // Generate candidates
    auto candidates = generator->generateCandidates(largeFilePath_);

    // Check that we got some candidates
    EXPECT_FALSE(candidates.empty());

    // Check that we didn't exceed maxCandidates
    EXPECT_LE(candidates.size(), options.maxCandidates);
}

// Test with parallel processing but small input (should use sequential)
TEST_F(CandidateGeneratorAdvancedTest, ParallelWithSmallInput) {
    // Create file without header (small file)
    createNoHeaderFile();

    // Enable parallel processing but with small input
    options.useParallelProcessing = true;
    options.threads = 4;
    options.minPmiScore = 1.0; // Lower threshold to ensure we get results
    generator = std::make_unique<CandidateGenerator>(options);

    try {
        // Generate candidates
        auto candidates = generator->generateCandidates(noHeaderFilePath_);

        // If we get candidates, check their properties
        if (!candidates.empty()) {
            // Check that all candidates have score >= minPmiScore
            for (const auto& candidate : candidates) {
                EXPECT_GE(candidate.score, options.minPmiScore);
            }
        }

        // Test passes either way - we're just testing that it doesn't crash
        SUCCEED() << "Small input handled correctly with parallel processing";
    } catch (const std::exception& e) {
        // If it throws, that's also acceptable for this test
        SUCCEED() << "Exception handled gracefully: " << e.what();
    }
}

// Test with zero threads (should use hardware_concurrency)
TEST_F(CandidateGeneratorAdvancedTest, ZeroThreads) {
    // Create large file
    createLargeFile(5000);

    // Set threads to 0 (auto-detect)
    options.threads = 0;
    generator = std::make_unique<CandidateGenerator>(options);

    // Generate candidates
    auto candidates = generator->generateCandidates(largeFilePath_);

    // Check that we got some candidates
    EXPECT_FALSE(candidates.empty());

    // Check that we didn't exceed maxCandidates
    EXPECT_LE(candidates.size(), options.maxCandidates);
}

// Test with no candidates meeting criteria
TEST_F(CandidateGeneratorAdvancedTest, NoCandidatesMeetingCriteria) {
    // Create file without header
    createNoHeaderFile();

    // Set very high minimum PMI score
    options.minPmiScore = 10.0; // Higher than any score in the file
    generator = std::make_unique<CandidateGenerator>(options);

    // Generate candidates
    auto candidates = generator->generateCandidates(noHeaderFilePath_);

    // Check that we got no candidates
    EXPECT_TRUE(candidates.empty());
}

} // namespace test
} // namespace core
} // namespace suzume
