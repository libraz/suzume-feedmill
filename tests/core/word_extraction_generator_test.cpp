/**
 * @file word_extraction_generator_test.cpp
 * @brief Tests for CandidateGenerator class
 */

#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include "../../src/core/word_extraction/generator.h"

namespace suzume {
namespace core {
namespace test {

class CandidateGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test PMI results file
        createPmiResultsFile();

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
        std::remove(pmiResultsPath_.c_str());
    }

    // Create a test PMI results file
    void createPmiResultsFile() {
        std::ofstream file(pmiResultsPath_);
        ASSERT_TRUE(file.is_open());

        // Header
        file << "ngram\tscore\tfreq\n";

        // Add some test n-grams with PMI scores
        file << "人工知能\t5.2\t10\n";
        file << "機械学習\t4.8\t8\n";
        file << "深層学習\t4.5\t7\n";
        file << "自然言語\t4.2\t6\n";
        file << "処理技術\t4.0\t5\n";
        file << "人工知\t3.8\t4\n";
        file << "知能研\t3.5\t3\n";
        file << "研究開\t3.2\t2\n";
        file << "開発者\t3.0\t1\n";
        file << "低スコア\t2.5\t1\n"; // Below threshold

        file.close();
    }

    // Test files
    std::string pmiResultsPath_ = "test_pmi_results.tsv";

    // Test objects
    WordExtractionOptions options;
    std::unique_ptr<CandidateGenerator> generator;
};

// Test basic candidate generation
TEST_F(CandidateGeneratorTest, BasicGeneration) {
    // Generate candidates
    auto candidates = generator->generateCandidates(pmiResultsPath_);

    // Check that we got some candidates
    EXPECT_FALSE(candidates.empty());

    // Check that candidates are sorted by score (descending)
    for (size_t i = 1; i < candidates.size(); i++) {
        EXPECT_GE(candidates[i-1].score, candidates[i].score);
    }

    // Check that candidates have the expected properties
    for (const auto& candidate : candidates) {
        EXPECT_FALSE(candidate.text.empty());
        EXPECT_GE(candidate.score, options.minPmiScore);
        EXPECT_GT(candidate.frequency, 0);
        EXPECT_FALSE(candidate.verified);
    }
}

// Test with minimum PMI score threshold
TEST_F(CandidateGeneratorTest, MinPmiScoreThreshold) {
    // Set higher minimum PMI score
    options.minPmiScore = 3.5;  // Lower threshold to ensure we get some candidates
    generator = std::make_unique<CandidateGenerator>(options);

    // Generate candidates
    auto candidates = generator->generateCandidates(pmiResultsPath_);

    // Check that we got some candidates
    EXPECT_FALSE(candidates.empty());

    // Check that all candidates have score >= minPmiScore
    for (const auto& candidate : candidates) {
        EXPECT_GE(candidate.score, options.minPmiScore);
    }
}

// Test with maximum candidate length
TEST_F(CandidateGeneratorTest, MaxCandidateLength) {
    // Set lower maximum candidate length
    options.maxCandidateLength = 3;
    generator = std::make_unique<CandidateGenerator>(options);

    // Generate candidates
    auto candidates = generator->generateCandidates(pmiResultsPath_);

    // Check that all candidates have length <= 3
    for (const auto& candidate : candidates) {
        EXPECT_LE(candidate.text.length(), 3);
    }
}

// Test with maximum number of candidates
TEST_F(CandidateGeneratorTest, MaxCandidates) {
    // Set lower maximum number of candidates
    options.maxCandidates = 3;
    generator = std::make_unique<CandidateGenerator>(options);

    // Generate candidates
    auto candidates = generator->generateCandidates(pmiResultsPath_);

    // Check that we got the expected number of candidates
    EXPECT_LE(candidates.size(), 3);

    // Check that candidates are sorted by score (descending)
    for (size_t i = 1; i < candidates.size(); i++) {
        EXPECT_GE(candidates[i-1].score, candidates[i].score);
    }
}

// Test with progress callback
TEST_F(CandidateGeneratorTest, ProgressCallback) {
    // Progress tracking
    std::vector<double> progressValues;

    // Generate candidates with progress callback
    auto candidates = generator->generateCandidates(
        pmiResultsPath_,
        [&progressValues](double progress) {
            progressValues.push_back(progress);
        }
    );

    // Check that progress callback was called
    EXPECT_FALSE(progressValues.empty());

    // Check that progress values are increasing
    for (size_t i = 1; i < progressValues.size(); i++) {
        EXPECT_GE(progressValues[i], progressValues[i-1]);
    }
}

// Test with sequential processing
TEST_F(CandidateGeneratorTest, SequentialProcessing) {
    // Disable parallel processing
    options.useParallelProcessing = false;
    generator = std::make_unique<CandidateGenerator>(options);

    // Generate candidates
    auto candidates = generator->generateCandidates(pmiResultsPath_);

    // Check that we got some candidates
    EXPECT_FALSE(candidates.empty());

    // Check that candidates are sorted by score (descending)
    for (size_t i = 1; i < candidates.size(); i++) {
        EXPECT_GE(candidates[i-1].score, candidates[i].score);
    }
}

// Test with invalid input
TEST_F(CandidateGeneratorTest, InvalidInput) {
    // Test with non-existent file
    EXPECT_THROW(
        generator->generateCandidates("non_existent_file.tsv"),
        std::runtime_error
    );
}

} // namespace test
} // namespace core
} // namespace suzume
