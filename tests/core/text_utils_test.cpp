/**
 * @file text_utils_test.cpp
 * @brief Tests for text utilities
 */

#include <gtest/gtest.h>
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

    // Test comment line
    EXPECT_EQ("", normalizeLine("#comment", NormalizationForm::NFKC));
}

// Test exclusion rules
TEST(TextUtilsTest, ShouldExcludeLine) {
    // Test empty line
    EXPECT_TRUE(shouldExcludeLine(""));

    // Test single character line
    EXPECT_TRUE(shouldExcludeLine("a"));

    // Test comment line
    EXPECT_TRUE(shouldExcludeLine("#comment"));

    // Test valid line
    EXPECT_FALSE(shouldExcludeLine("hello world"));
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
    std::string input1 = "café"; // precomposed form
    std::string input2 = "cafe\u0301"; // decomposed form

    // Both should normalize to the same form in NFKC
    std::string normalized1 = normalizeLine(input1, NormalizationForm::NFKC);
    std::string normalized2 = normalizeLine(input2, NormalizationForm::NFKC);

    // The normalized forms should be equal and should contain the accent
    EXPECT_EQ(normalized1, normalized2);
    EXPECT_NE(std::string::npos, normalized1.find("é"));

    // Test other combining characters
    input1 = "à";
    normalized1 = normalizeLine(input1, NormalizationForm::NFKC);
    EXPECT_NE(std::string::npos, normalized1.find("à"));
}

// Test different normalization forms
TEST(TextUtilsTest, NormalizationForms) {
    // Test NFC vs NFKC normalization

    // Latin ligature fi (U+FB01) should be decomposed in NFKC but not in NFC
    std::string input = "ﬁ"; // Latin small ligature fi

    std::string nfkc = normalizeLine(input, NormalizationForm::NFKC);
    std::string nfc = normalizeLine(input, NormalizationForm::NFC);

    // In NFKC, it should be decomposed to "fi"
    EXPECT_EQ("fi", nfkc);

    // In NFC, it should remain as is (but we're skipping single character lines)
    EXPECT_EQ("", nfc);

    // Test with longer input
    input = "ﬁle test";
    nfkc = normalizeLine(input, NormalizationForm::NFKC);
    nfc = normalizeLine(input, NormalizationForm::NFC);

    // In NFKC, the ligature should be decomposed
    EXPECT_EQ("file test", nfkc);

    // In NFC, the ligature should be preserved
    EXPECT_EQ("ﬁle test", nfc);
}

} // namespace test
} // namespace core
} // namespace suzume
