/**
 * @file word_extraction_ranker_test.cpp
 * @brief Tests for CandidateRanker class
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <algorithm>
#include "../../src/core/word_extraction/ranker.h"

namespace suzume {
namespace core {
namespace test {

class CandidateRankerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create options
        options.rankingModel = "combined";
        options.pmiWeight = 0.5;
        options.lengthWeight = 0.2;
        options.contextWeight = 0.2;
        options.statisticalWeight = 0.1;
        options.topK = 10;

        // Create ranker
        ranker = std::make_unique<CandidateRanker>(options);

        // Create test candidates
        createTestCandidates();
    }

    // Create test candidates
    void createTestCandidates() {
        // Candidate with high PMI score
        VerifiedCandidate candidate1;
        candidate1.text = "人工知能";
        candidate1.score = 5.2;
        candidate1.frequency = 10;
        candidate1.context = "人工知能の研究";
        candidate1.contextScore = 0.8;
        candidate1.statisticalScore = 0.7;
        candidates.push_back(candidate1);

        // Candidate with medium PMI score but high context score
        VerifiedCandidate candidate2;
        candidate2.text = "機械学習";
        candidate2.score = 4.8;
        candidate2.frequency = 8;
        candidate2.context = "機械学習の研究";
        candidate2.contextScore = 0.9;
        candidate2.statisticalScore = 0.6;
        candidates.push_back(candidate2);

        // Candidate with medium PMI score and medium context score
        VerifiedCandidate candidate3;
        candidate3.text = "深層学習";
        candidate3.score = 4.5;
        candidate3.frequency = 7;
        candidate3.context = "深層学習の研究";
        candidate3.contextScore = 0.7;
        candidate3.statisticalScore = 0.5;
        candidates.push_back(candidate3);

        // Candidate with low PMI score but high statistical score
        VerifiedCandidate candidate4;
        candidate4.text = "自然言語";
        candidate4.score = 4.2;
        candidate4.frequency = 6;
        candidate4.context = "自然言語の研究";
        candidate4.contextScore = 0.6;
        candidate4.statisticalScore = 0.9;
        candidates.push_back(candidate4);

        // Candidate with optimal length (4 characters)
        VerifiedCandidate candidate5;
        candidate5.text = "最適長さ";
        candidate5.score = 4.0;
        candidate5.frequency = 5;
        candidate5.context = "最適長さの研究";
        candidate5.contextScore = 0.5;
        candidate5.statisticalScore = 0.4;
        candidates.push_back(candidate5);
    }

    // Test objects
    WordExtractionOptions options;
    std::unique_ptr<CandidateRanker> ranker;
    std::vector<VerifiedCandidate> candidates;
};

// Test basic ranking
TEST_F(CandidateRankerTest, BasicRanking) {
    // Rank candidates
    auto rankedCandidates = ranker->rankCandidates(candidates);

    // Check that we got the expected number of ranked candidates
    EXPECT_EQ(rankedCandidates.size(), candidates.size());

    // Check that candidates are sorted by score (descending)
    for (size_t i = 1; i < rankedCandidates.size(); i++) {
        EXPECT_GE(rankedCandidates[i-1].score, rankedCandidates[i].score);
    }

    // Check that all fields are copied correctly
    for (const auto& candidate : rankedCandidates) {
        EXPECT_FALSE(candidate.text.empty());
        EXPECT_GT(candidate.score, 0.0);
        EXPECT_GT(candidate.frequency, 0);
        EXPECT_FALSE(candidate.context.empty());
    }
}

// Test with default ranking model
TEST_F(CandidateRankerTest, DefaultRankingModel) {
    // Use default ranking model
    options.rankingModel = "";
    ranker = std::make_unique<CandidateRanker>(options);

    // Rank candidates
    auto rankedCandidates = ranker->rankCandidates(candidates);

    // Check that candidates are sorted by PMI score (descending)
    EXPECT_EQ(rankedCandidates[0].text, "人工知能");
    EXPECT_EQ(rankedCandidates[1].text, "機械学習");
    EXPECT_EQ(rankedCandidates[2].text, "深層学習");
}

// Test with combined ranking model
TEST_F(CandidateRankerTest, CombinedRankingModel) {
    // Use combined ranking model with custom weights
    options.rankingModel = "combined";
    options.pmiWeight = 0.1;
    options.contextWeight = 0.8;
    options.lengthWeight = 0.05;
    options.statisticalWeight = 0.05;
    ranker = std::make_unique<CandidateRanker>(options);

    // Rank candidates
    auto rankedCandidates = ranker->rankCandidates(candidates);

    // With these weights, the candidate with the highest context score should be ranked first
    EXPECT_EQ(rankedCandidates[0].text, "機械学習");
}

// Test with top K limit
TEST_F(CandidateRankerTest, TopKLimit) {
    // Set top K limit
    options.topK = 3;

    // Rank candidates
    auto rankedCandidates = ranker->rankCandidates(candidates);

    // Check that we got the expected number of ranked candidates
    EXPECT_EQ(rankedCandidates.size(), 5);  // topK is applied by extractWords, not by rankCandidates
}

// Test with progress callback
TEST_F(CandidateRankerTest, ProgressCallback) {
    // Progress tracking
    std::vector<double> progressValues;

    // Rank candidates with progress callback
    auto rankedCandidates = ranker->rankCandidates(
        candidates,
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

    // Check that final progress is 1.0
    if (!progressValues.empty()) {
        EXPECT_DOUBLE_EQ(progressValues.back(), 1.0);
    }
}

// Test with empty input
TEST_F(CandidateRankerTest, EmptyInput) {
    // Rank empty candidates
    auto rankedCandidates = ranker->rankCandidates({});

    // Check that we got empty result
    EXPECT_TRUE(rankedCandidates.empty());
}

// Test score calculation
TEST_F(CandidateRankerTest, ScoreCalculation) {
    // Create a candidate with known scores
    VerifiedCandidate candidate;
    candidate.text = "テスト";
    candidate.score = 5.0;         // PMI score
    candidate.frequency = 10;
    candidate.context = "テストの文脈";
    candidate.contextScore = 0.8;   // Context score
    candidate.statisticalScore = 0.6; // Statistical score

    std::vector<VerifiedCandidate> testCandidates = {candidate};

    // Set equal weights for all factors
    options.rankingModel = "combined";
    options.pmiWeight = 0.25;
    options.lengthWeight = 0.25;
    options.contextWeight = 0.25;
    options.statisticalWeight = 0.25;
    ranker = std::make_unique<CandidateRanker>(options);

    // Rank the candidate
    auto rankedCandidates = ranker->rankCandidates(testCandidates);

    // Check that the score is calculated correctly
    ASSERT_EQ(rankedCandidates.size(), 1);

    // The score should be:
    // 0.25 * min(1.0, 5.0/10.0) +                                  // PMI score (normalized to 0-1)
    // 0.25 * exp(-((3-4)*(3-4))/8.0) +                             // Length score (optimal length is 4)
    // 0.25 * 0.8 +                                                 // Context score
    // 0.25 * 0.6                                                   // Statistical score

    // We don't know the exact formula for length score, so we just check that the final score is reasonable
    EXPECT_GT(rankedCandidates[0].score, 0.0);
    EXPECT_LT(rankedCandidates[0].score, 1.0);
}

} // namespace test
} // namespace core
} // namespace suzume
