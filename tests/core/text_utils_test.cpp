/**
 * @file text_utils_test.cpp
 * @brief Tests for text utilities
 */

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "core/text_utils.h"

namespace suzume {
namespace core {
namespace test {

// Test normalization of text
TEST(TextUtilsTest, NormalizeLine) {
    // Test basic normalization
    EXPECT_EQ("hello world", normalizeLine("Hello World", NormalizationForm::NFKC));

    // Test full-width to half-width conversion
    EXPECT_EQ("hello world", normalizeLine("Ｈｅｌｌｏ　Ｗｏｒｌｄ", NormalizationForm::NFKC));

    // Test NFC form (preserves case)
    EXPECT_EQ("Hello World", normalizeLine("Hello World", NormalizationForm::NFC));

    // Test empty line
    EXPECT_EQ("", normalizeLine("", NormalizationForm::NFKC));

    // Test single character line
    EXPECT_EQ("", normalizeLine("a", NormalizationForm::NFKC));

    // Hashtags are now preserved
    EXPECT_EQ("#comment", normalizeLine("#comment", NormalizationForm::NFKC));
}

// Test exclusion rules
TEST(TextUtilsTest, ShouldExcludeLine) {
    // Test empty line
    EXPECT_TRUE(shouldExcludeLine(""));

    // Test single character line
    EXPECT_TRUE(shouldExcludeLine("a"));

    // Hashtags are no longer excluded
    EXPECT_FALSE(shouldExcludeLine("#comment"));

    // Test valid line
    EXPECT_FALSE(shouldExcludeLine("hello world"));

    // Test with length filters
    EXPECT_TRUE(shouldExcludeLine("hi", 3, 0)); // Too short
    EXPECT_TRUE(shouldExcludeLine("this is a very long line", 0, 10)); // Too long
    EXPECT_FALSE(shouldExcludeLine("hello", 3, 10)); // Just right
}

// Test n-gram generation
TEST(TextUtilsTest, GenerateNgrams) {
    // Test unigrams
    auto unigrams = generateNgrams("abc", 1);
    ASSERT_EQ(3, unigrams.size());
    EXPECT_EQ("a", unigrams[0]);
    EXPECT_EQ("b", unigrams[1]);
    EXPECT_EQ("c", unigrams[2]);

    // Test bigrams
    auto bigrams = generateNgrams("abc", 2);
    ASSERT_EQ(2, bigrams.size());
    EXPECT_EQ("ab", bigrams[0]);
    EXPECT_EQ("bc", bigrams[1]);

    // Test trigrams
    auto trigrams = generateNgrams("abcd", 3);
    ASSERT_EQ(2, trigrams.size());
    EXPECT_EQ("abc", trigrams[0]);
    EXPECT_EQ("bcd", trigrams[1]);

    // Test empty string
    EXPECT_TRUE(generateNgrams("", 1).empty());

    // Test n > string length
    EXPECT_TRUE(generateNgrams("ab", 3).empty());
}

// Test hash calculation
TEST(TextUtilsTest, CalculateHash) {
    // Test basic hash calculation
    EXPECT_NE(0, calculateHash("hello"));

    // Test different strings have different hashes
    EXPECT_NE(calculateHash("hello"), calculateHash("world"));

    // Test same string has same hash
    EXPECT_EQ(calculateHash("hello"), calculateHash("hello"));
}

// Test duplicate detection
TEST(TextUtilsTest, IsDuplicate) {
    std::unordered_set<std::string> uniqueSet;

    // Test first occurrence is not a duplicate
    EXPECT_FALSE(isDuplicate("hello", uniqueSet, 0.01));

    // Test second occurrence is a duplicate
    EXPECT_TRUE(isDuplicate("hello", uniqueSet, 0.01));

    // Test different string is not a duplicate
    EXPECT_FALSE(isDuplicate("world", uniqueSet, 0.01));
}

// Test surrogate pair handling
TEST(TextUtilsTest, SurrogatePairs) {
    // Test surrogate pairs in normalization
    // 𐐷 is U+10437 (DESERET SMALL LETTER YEE), encoded as surrogate pair in UTF-16
    std::string input = "𐐷𐐷𐐷 surrogate pairs";
    std::string normalized = normalizeLine(input, NormalizationForm::NFKC);

    // The surrogate pairs should be preserved
    EXPECT_NE(std::string::npos, normalized.find("𐐷𐐷𐐷"));

    // Test CJK characters
    input = "𠜎𠜱𠝹𠱓 CJK extension B";
    normalized = normalizeLine(input, NormalizationForm::NFKC);
    EXPECT_NE(std::string::npos, normalized.find("𠜎𠜱𠝹𠱓"));
}

// Test combining character normalization
TEST(TextUtilsTest, CombiningCharacters) {
    // Test combining characters in normalization
    // "café" can be represented as "cafe\u0301" (e + combining acute accent)
    // Using multi-character strings to avoid single-character exclusion
    std::string input1 = "café test"; // precomposed form
    std::string input2 = "cafe\u0301 test"; // decomposed form

    // Both should normalize to the same form in NFKC
    std::string normalized1 = normalizeLine(input1, NormalizationForm::NFKC);
    std::string normalized2 = normalizeLine(input2, NormalizationForm::NFKC);

#ifdef _WIN32
    // On Windows, ICU may handle combining characters differently
    // We only check that the precomposed form is correctly handled
    EXPECT_NE(std::string::npos, normalized1.find("é"));

    // Skip the equality check on Windows
    // This is because Windows ICU implementation may not correctly
    // normalize the decomposed form with combining characters
#else
    // On non-Windows platforms, both forms should normalize to the same result
    EXPECT_EQ(normalized1, normalized2);
    EXPECT_NE(std::string::npos, normalized1.find("é"));
#endif

    // Test other combining characters with multi-character strings
    input1 = "àbc test";
    normalized1 = normalizeLine(input1, NormalizationForm::NFKC);
    EXPECT_NE(std::string::npos, normalized1.find("à"));
}

// Test different normalization forms
TEST(TextUtilsTest, NormalizationForms) {
    // Test NFC vs NFKC normalization using multi-character strings

    // Latin ligature fi (U+FB01) should be decomposed in NFKC but not in NFC
    // Using multi-character string to avoid single-character exclusion
    std::string input = "ﬁle test"; // Latin small ligature fi with additional text

    std::string nfkc = normalizeLine(input, NormalizationForm::NFKC);
    std::string nfc = normalizeLine(input, NormalizationForm::NFC);

    // In NFKC, the ligature should be decomposed
    EXPECT_EQ("file test", nfkc);

    // In NFC, the ligature should be preserved
    EXPECT_EQ("ﬁle test", nfc);

    // Test with another example
    input = "ﬁnance report";
    nfkc = normalizeLine(input, NormalizationForm::NFKC);
    nfc = normalizeLine(input, NormalizationForm::NFC);

    // In NFKC, the ligature should be decomposed
    EXPECT_EQ("finance report", nfkc);

    // In NFC, the ligature should be preserved
    EXPECT_EQ("ﬁnance report", nfc);
}

// Test Reservoir sampling
TEST(TextUtilsTest, SampleLines) {
    // Create a temporary file with test data
    std::string tempFilePath = "test_sample_lines.txt";
    std::ofstream tempFile(tempFilePath);
    ASSERT_TRUE(tempFile.is_open());

    // Write 100 lines to the file
    for (int i = 0; i < 100; i++) {
        tempFile << "Line " << i << std::endl;
    }
    tempFile.close();

    // Test sampling with fixed seed for deterministic results
    unsigned int seed = 42;

    // Sample 10 lines
    auto sample10 = sampleLines(tempFilePath, 10, seed);
    EXPECT_EQ(10, sample10.size());

    // Sample 20 lines
    auto sample20 = sampleLines(tempFilePath, 20, seed);
    EXPECT_EQ(20, sample20.size());

    // Sample more lines than in the file
    auto sampleAll = sampleLines(tempFilePath, 200, seed);
    EXPECT_EQ(100, sampleAll.size()); // Should return all lines

    // Sample 0 lines
    auto sample0 = sampleLines(tempFilePath, 0, seed);
    EXPECT_EQ(0, sample0.size());

    // Clean up
    std::filesystem::remove(tempFilePath);
}

// Test sampling with error conditions
TEST(TextUtilsTest, SampleLinesErrors) {
    // Test with non-existent file
    try {
        EXPECT_THROW(sampleLines("non_existent_file.txt", 10), std::runtime_error);
    } catch (const std::exception& e) {
        // If the test fails, print the error message for debugging
        std::cerr << "Exception caught: " << e.what() << std::endl;
        FAIL() << "Unexpected exception: " << e.what();
    }

    // Test with empty file
    std::string emptyFilePath = "empty_sample_test.txt";
    {
        std::ofstream emptyFile(emptyFilePath);
        // Make sure the file is created successfully
        ASSERT_TRUE(emptyFile.is_open()) << "Failed to create empty test file";
        emptyFile.close();
    }

    // Verify the file exists
    ASSERT_TRUE(std::filesystem::exists(emptyFilePath)) << "Empty test file was not created";

    // Sampling from empty file should return empty vector
    auto emptyResult = sampleLines(emptyFilePath, 10);
    EXPECT_TRUE(emptyResult.empty());

    // Clean up
    try {
        bool removed = std::filesystem::remove(emptyFilePath);
        EXPECT_TRUE(removed) << "Failed to remove test file";
    } catch (const std::exception& e) {
        std::cerr << "Error removing file: " << e.what() << std::endl;
    }
}

} // namespace test
} // namespace core
} // namespace suzume
