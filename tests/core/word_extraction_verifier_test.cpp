/**
 * @file word_extraction_verifier_test.cpp
 * @brief Tests for CandidateVerifier class
 */

#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/normalizer2.h>
#include "../../src/core/word_extraction/verifier.h"

using icu::UnicodeString;

namespace suzume {
namespace core {
namespace test {

class CandidateVerifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test original text file
        createOriginalTextFile();

        // Create test dictionary file
        createDictionaryFile();

        // Create options
        options.verifyInOriginalText = true;
        options.useContextualAnalysis = true;
        options.useStatisticalValidation = true;
        options.useDictionaryLookup = true;
        options.dictionaryPath = dictionaryPath_;

        // Create verifier
        verifier = std::make_unique<CandidateVerifier>(options);

        // Create test candidates
        createTestCandidates();
    }

    void TearDown() override {
        // Clean up test files
        std::remove(originalTextPath_.c_str());
        std::remove(dictionaryPath_.c_str());
    }

    // Create a test original text file
    void createOriginalTextFile() {
        std::ofstream file(originalTextPath_);
        ASSERT_TRUE(file.is_open());

        // Add some test text containing the candidates
        file << "人工知能と機械学習の研究が進んでいます。\n";
        file << "深層学習を用いた自然言語処理技術の開発が行われています。\n";
        file << "人工知能研究開発者が集まるカンファレンスが開催されました。\n";

        file.close();
    }

    // Create a test dictionary file
    void createDictionaryFile() {
        std::ofstream file(dictionaryPath_);
        ASSERT_TRUE(file.is_open());

        // Add some words to the dictionary
        file << "人工知能\n";  // This one is in the dictionary
        file << "辞書単語\n";

        file.close();
    }

    // Create test candidates
    void createTestCandidates() {
        // Candidate in text and not in dictionary
        WordCandidate candidate1;
        candidate1.text = "機械学習";
        candidate1.score = 4.8;
        candidate1.frequency = 8;
        candidate1.verified = false;
        candidates.push_back(candidate1);

        // Candidate in text and in dictionary
        WordCandidate candidate2;
        candidate2.text = "人工知能";
        candidate2.score = 5.2;
        candidate2.frequency = 10;
        candidate2.verified = false;
        candidates.push_back(candidate2);

        // Candidate not in text
        WordCandidate candidate3;
        candidate3.text = "存在しない";
        candidate3.score = 3.5;
        candidate3.frequency = 3;
        candidate3.verified = false;
        candidates.push_back(candidate3);
    }

    // Test files
    std::string originalTextPath_ = "test_original_text.txt";
    std::string dictionaryPath_ = "test_dictionary.txt";

    // Test objects
    WordExtractionOptions options;
    std::unique_ptr<CandidateVerifier> verifier;
    std::vector<WordCandidate> candidates;
};

// Test basic verification
TEST_F(CandidateVerifierTest, BasicVerification) {
    // Verify candidates
    auto verifiedCandidates = verifier->verifyCandidates(candidates, originalTextPath_);

    // Check that we got the expected number of verified candidates
    // Only "機械学習" should be verified (in text and not in dictionary)
    EXPECT_EQ(verifiedCandidates.size(), 1);

    // Check the verified candidate
    if (!verifiedCandidates.empty()) {
        EXPECT_EQ(verifiedCandidates[0].text, "機械学習");
        EXPECT_DOUBLE_EQ(verifiedCandidates[0].score, 4.8);
        EXPECT_EQ(verifiedCandidates[0].frequency, 8);
        EXPECT_FALSE(verifiedCandidates[0].context.empty());
    }
}

// Test with verification disabled
TEST_F(CandidateVerifierTest, VerificationDisabled) {
    // Disable verification
    options.verifyInOriginalText = false;
    verifier = std::make_unique<CandidateVerifier>(options);

    // Verify candidates
    auto verifiedCandidates = verifier->verifyCandidates(candidates, originalTextPath_);

    // Check that we got more verified candidates
    // "機械学習" and "存在しない" should be verified (not in dictionary)
    EXPECT_EQ(verifiedCandidates.size(), 2);

    // Check that specific candidates are included
    bool found_ml = false;
    bool found_nonexistent = false;

    for (const auto& candidate : verifiedCandidates) {
        if (candidate.text == "機械学習") {
            found_ml = true;
        } else if (candidate.text == "存在しない") {
            found_nonexistent = true;
        }
    }

    EXPECT_TRUE(found_ml);
    EXPECT_TRUE(found_nonexistent);
}

// Test with dictionary lookup disabled
TEST_F(CandidateVerifierTest, DictionaryLookupDisabled) {
    // Disable dictionary lookup
    options.useDictionaryLookup = false;
    verifier = std::make_unique<CandidateVerifier>(options);

    // Verify candidates
    auto verifiedCandidates = verifier->verifyCandidates(candidates, originalTextPath_);

    // Check that we got more verified candidates
    // "機械学習" and "人工知能" should be verified (in text)
    EXPECT_EQ(verifiedCandidates.size(), 2);

    // Check that specific candidates are included
    bool found_ml = false;
    bool found_ai = false;

    for (const auto& candidate : verifiedCandidates) {
        if (candidate.text == "機械学習") {
            found_ml = true;
        } else if (candidate.text == "人工知能") {
            found_ai = true;
        }
    }

    EXPECT_TRUE(found_ml);
    EXPECT_TRUE(found_ai);
}

// Test with contextual analysis disabled
TEST_F(CandidateVerifierTest, ContextualAnalysisDisabled) {
    // Disable contextual analysis
    options.useContextualAnalysis = false;
    verifier = std::make_unique<CandidateVerifier>(options);

    // Verify candidates
    auto verifiedCandidates = verifier->verifyCandidates(candidates, originalTextPath_);

    // Check that we still got the expected number of verified candidates
    EXPECT_EQ(verifiedCandidates.size(), 1);

    // Check that the context is empty
    if (!verifiedCandidates.empty()) {
        EXPECT_TRUE(verifiedCandidates[0].context.empty());
        EXPECT_DOUBLE_EQ(verifiedCandidates[0].contextScore, 0.0);
    }
}

// Test with statistical validation disabled
TEST_F(CandidateVerifierTest, StatisticalValidationDisabled) {
    // Disable statistical validation
    options.useStatisticalValidation = false;
    verifier = std::make_unique<CandidateVerifier>(options);

    // Verify candidates
    auto verifiedCandidates = verifier->verifyCandidates(candidates, originalTextPath_);

    // Check that we still got the expected number of verified candidates
    EXPECT_EQ(verifiedCandidates.size(), 1);

    // Check that the statistical score is zero
    if (!verifiedCandidates.empty()) {
        EXPECT_DOUBLE_EQ(verifiedCandidates[0].statisticalScore, 0.0);
    }
}

// Test with progress callback
TEST_F(CandidateVerifierTest, ProgressCallback) {
    // Progress tracking
    std::vector<double> progressValues;

    // Verify candidates with progress callback
    auto verifiedCandidates = verifier->verifyCandidates(
        candidates,
        originalTextPath_,
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

// Test with invalid input
TEST_F(CandidateVerifierTest, InvalidInput) {
    // Test with non-existent file
    EXPECT_THROW(
        verifier->verifyCandidates(candidates, "non_existent_file.txt"),
        std::runtime_error
    );
}

// Test UTF-8 context extraction
TEST_F(CandidateVerifierTest, UTF8ContextExtraction) {
    // Create a test file with UTF-8 text containing multi-byte characters
    std::string utf8TestPath = "test_utf8_context.txt";
    {
        std::ofstream file(utf8TestPath);
        ASSERT_TRUE(file.is_open());

        // Japanese text with various UTF-8 characters
        file << "人工知能技術の発展により、様々な分野での応用が進んでいます。\n";
        file << "深層学習モデルを用いた画像認識の精度が向上しています。\n";
        file << "自然言語処理技術の進歩により、機械翻訳の品質が改善されました。\n";
        file << "強化学習アルゴリズムを用いたロボット制御の研究が進んでいます。\n";
        file << "転移学習手法を活用することで、少ないデータでも高い性能を実現できます。\n";

        file.close();
    }

    // Create test candidates with positions that would cause broken UTF-8 if not handled properly
    std::vector<WordCandidate> testCandidates;

    // Candidate at a position that includes multi-byte character
    WordCandidate candidate1;
    candidate1.text = "知能";  // This is in the text
    candidate1.score = 5.0;
    candidate1.frequency = 5;
    candidate1.verified = false;
    testCandidates.push_back(candidate1);

    // Create verifier with contextual analysis enabled
    WordExtractionOptions testOptions = options;
    testOptions.useContextualAnalysis = true;
    auto testVerifier = std::make_unique<CandidateVerifier>(testOptions);

    // Verify candidates - this will extract context
    auto verifiedCandidates = testVerifier->verifyCandidates(testCandidates, utf8TestPath);

    // Check that we got verified candidates
    ASSERT_FALSE(verifiedCandidates.empty());

    // Check that the context doesn't have broken UTF-8 sequences
    for (const auto& candidate : verifiedCandidates) {
        // Context should not be empty
        EXPECT_FALSE(candidate.context.empty());

        // Check for broken UTF-8 sequences by round-tripping through ICU
        UnicodeString ustr = UnicodeString::fromUTF8(candidate.context);
        std::string roundTrip;
        ustr.toUTF8String(roundTrip);

        // If the round-trip produces the same string, UTF-8 is valid
        EXPECT_EQ(candidate.context, roundTrip)
            << "Context contains broken UTF-8 sequences: " << candidate.context;

        // Context should contain the candidate text
        EXPECT_NE(candidate.context.find(candidate.text), std::string::npos)
            << "Context doesn't contain the candidate text";

        // Context should contain surrounding text
        EXPECT_TRUE(candidate.context.length() > candidate.text.length())
            << "Context is too short, should include surrounding text";
    }

    // Clean up
    std::remove(utf8TestPath.c_str());
}

} // namespace test
} // namespace core
} // namespace suzume
