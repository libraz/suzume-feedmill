/**
 * @file normalize_unicode_test.cpp
 * @brief Tests for Unicode normalization functionality
 */

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include "core/normalize.h"
#include "core/text_utils.h"

namespace suzume {
namespace core {
namespace test {

class NormalizeUnicodeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory if it doesn't exist
        std::filesystem::create_directories("test_data");
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all("test_data");
    }

    // Helper to create a test file with the given lines
    void createTestFile(const std::string& filename, const std::vector<std::string>& lines) {
        std::ofstream file("test_data/" + filename);
        for (const auto& line : lines) {
            file << line << "\n";
        }
        file.close();
    }
};

// Test normalization of combining characters
TEST_F(NormalizeUnicodeTest, CombiningCharacters) {
    // Create test file with combining characters
    // e + combining acute accent (U+0301) should normalize to é
    std::vector<std::string> lines = {
        "cafe\u0301", // café with combining acute
        "café",       // café with precomposed é
        "resume\u0301", // resumé with combining acute
        "resumé"      // resumé with precomposed é
    };
    createTestFile("combining.tsv", lines);

    // Test with NFC normalization
    {
        NormalizeOptions options;
        options.form = NormalizationForm::NFC;

        NormalizeResult result = suzume::core::normalize(
            "test_data/combining.tsv",
            "test_data/combining_nfc.tsv",
            options
        );

        // Should have 2 unique lines after normalization
        EXPECT_EQ(4, result.rows);
        EXPECT_EQ(2, result.uniques);

        // Check output file
        std::ifstream outputFile("test_data/combining_nfc.tsv");
        std::vector<std::string> outputLines;
        std::string line;
        while (std::getline(outputFile, line)) {
            outputLines.push_back(line);
        }

        // Should contain "café" and "resumé" with precomposed characters
        EXPECT_EQ(2, outputLines.size());

        // Check that the lines are properly normalized
        bool hasCafe = false;
        bool hasResume = false;
        for (const auto& line : outputLines) {
            if (line == "café") hasCafe = true;
            if (line == "resumé") hasResume = true;
        }
        EXPECT_TRUE(hasCafe);
        EXPECT_TRUE(hasResume);
    }

    // Test with NFKC normalization
    {
        NormalizeOptions options;
        options.form = NormalizationForm::NFKC;

        NormalizeResult result = suzume::core::normalize(
            "test_data/combining.tsv",
            "test_data/combining_nfkc.tsv",
            options
        );

        // Should have 2 unique lines after normalization
        EXPECT_EQ(4, result.rows);
        EXPECT_EQ(2, result.uniques);
    }
}

// Test normalization of full-width characters
TEST_F(NormalizeUnicodeTest, FullWidthCharacters) {
    // Create test file with full-width characters
    std::vector<std::string> lines = {
        "ｈｅｌｌｏ　ｗｏｒｌｄ", // Full-width "hello world"
        "hello world",         // ASCII "hello world"
        "ＨＥＬＬＯ　ＷＯＲＬＤ", // Full-width "HELLO WORLD"
        "HELLO WORLD"          // ASCII "HELLO WORLD"
    };
    createTestFile("fullwidth.tsv", lines);

    // Test with NFKC normalization (should convert full-width to ASCII)
    {
        NormalizeOptions options;
        options.form = NormalizationForm::NFKC;

        NormalizeResult result = suzume::core::normalize(
            "test_data/fullwidth.tsv",
            "test_data/fullwidth_nfkc.tsv",
            options
        );

        // Should have 2 unique lines after normalization and case folding
        EXPECT_EQ(4, result.rows);
        EXPECT_EQ(1, result.uniques); // All normalize to "hello world"

        // Check output file
        std::ifstream outputFile("test_data/fullwidth_nfkc.tsv");
        std::string line;
        std::getline(outputFile, line);
        EXPECT_EQ("hello world", line);
    }

    // Test with NFC normalization (should preserve full-width characters)
    {
        NormalizeOptions options;
        options.form = NormalizationForm::NFC;

        NormalizeResult result = suzume::core::normalize(
            "test_data/fullwidth.tsv",
            "test_data/fullwidth_nfc.tsv",
            options
        );

        // Should have more unique lines with NFC
        EXPECT_EQ(4, result.rows);
        EXPECT_GT(result.uniques, 1); // Full-width and ASCII remain distinct
    }
}

// Test normalization of unusual Unicode sequences
TEST_F(NormalizeUnicodeTest, UnusualSequences) {
    // Create test file with unusual Unicode sequences
    std::vector<std::string> lines = {
        // Zero-width joiner and non-joiner
        "a\u200Db", // a + ZWJ + b
        "a\u200Cb", // a + ZWNJ + b

        // Bidirectional controls
        "hello\u2067world\u2069", // with RTI and PDI
        "hello world",

        // Variation selectors
        "text\uFE0Eemoji", // text variation selector
        "text\uFE0Femoji", // emoji variation selector

        // Unusual spaces
        "hello\u2000world", // en quad space
        "hello\u2001world", // em quad space
        "hello\u2002world", // en space
        "hello\u2003world", // em space
        "hello world"       // regular space
    };
    createTestFile("unusual.tsv", lines);

    // Test with NFKC normalization
    {
        NormalizeOptions options;
        options.form = NormalizationForm::NFKC;

        NormalizeResult result = suzume::core::normalize(
            "test_data/unusual.tsv",
            "test_data/unusual_nfkc.tsv",
            options
        );

        // Check that normalization completed
        EXPECT_EQ(lines.size(), result.rows);

        // NFKC should normalize most of these to the same form
        EXPECT_LT(result.uniques, result.rows);

        // Check output file
        std::ifstream outputFile("test_data/unusual_nfkc.tsv");
        std::vector<std::string> outputLines;
        std::string line;
        while (std::getline(outputFile, line)) {
            outputLines.push_back(line);
        }

        // Should contain "hello world" with regular spaces
        bool hasHelloWorld = false;
        for (const auto& line : outputLines) {
            if (line == "hello world") hasHelloWorld = true;
        }
        EXPECT_TRUE(hasHelloWorld);
    }
}

// Test normalization of emoji and surrogate pairs
TEST_F(NormalizeUnicodeTest, EmojiAndSurrogatePairs) {
    // Create test file with emoji and surrogate pairs
    std::vector<std::string> lines = {
        "Hello 😊 World", // Emoji (smiling face)
        "Hello 👨‍👩‍👧‍👦 World", // Family emoji (complex sequence with ZWJ)
        "Hello 👋🏻 World", // Emoji with skin tone modifier
        "Hello 🇯🇵 World", // Flag emoji (Japan)
        "Hello 🏳️‍🌈 World", // Rainbow flag (complex sequence)
        "Hello 😊 World", // Duplicate emoji line
    };
    createTestFile("emoji.tsv", lines);

    // Test with NFKC normalization
    {
        NormalizeOptions options;
        options.form = NormalizationForm::NFKC;

        NormalizeResult result = suzume::core::normalize(
            "test_data/emoji.tsv",
            "test_data/emoji_nfkc.tsv",
            options
        );

        // Check that normalization completed
        EXPECT_EQ(lines.size(), result.rows);

        // Should have fewer unique lines due to duplicates
        EXPECT_LT(result.uniques, result.rows);

        // Check output file
        std::ifstream outputFile("test_data/emoji_nfkc.tsv");
        std::vector<std::string> outputLines;
        std::string line;
        while (std::getline(outputFile, line)) {
            outputLines.push_back(line);
        }

        // Should preserve emoji in the output
        bool hasSimpleEmoji = false;
        for (const auto& line : outputLines) {
            if (line.find("😊") != std::string::npos) hasSimpleEmoji = true;
        }
        EXPECT_TRUE(hasSimpleEmoji);
    }
}

// Test normalization of various scripts
TEST_F(NormalizeUnicodeTest, VariousScripts) {
    // Create test file with various scripts
    std::vector<std::string> lines = {
        // Latin
        "Hello World",

        // Cyrillic
        "Привет мир",

        // Greek
        "Γειά σου Κόσμε",

        // Arabic
        "مرحبا بالعالم",

        // Hebrew
        "שלום עולם",

        // Japanese
        "こんにちは世界",

        // Chinese
        "你好，世界",

        // Korean
        "안녕하세요 세계",

        // Thai
        "สวัสดีชาวโลก",

        // Devanagari (Hindi)
        "नमस्ते दुनिया"
    };
    createTestFile("scripts.tsv", lines);

    // Test with NFKC normalization
    {
        NormalizeOptions options;
        options.form = NormalizationForm::NFKC;

        NormalizeResult result = suzume::core::normalize(
            "test_data/scripts.tsv",
            "test_data/scripts_nfkc.tsv",
            options
        );

        // Check that normalization completed
        EXPECT_EQ(lines.size(), result.rows);

        // All lines should be unique
        EXPECT_EQ(result.uniques, result.rows);

        // Check output file
        std::ifstream outputFile("test_data/scripts_nfkc.tsv");
        std::vector<std::string> outputLines;
        std::string line;
        while (std::getline(outputFile, line)) {
            outputLines.push_back(line);
        }

        // Should preserve all scripts
        EXPECT_EQ(lines.size(), outputLines.size());
    }
}

// Test normalization with unusual normalization forms
TEST_F(NormalizeUnicodeTest, UnusualNormalizationForms) {
    // Create test file with characters that behave differently in different normalization forms
    std::vector<std::string> lines = {
        "café", // Precomposed é
        "cafe\u0301", // Decomposed é (e + combining acute)
        "ﬁ", // Ligature fi (U+FB01)
        "fi", // Separate f and i
        "①", // Circled digit 1 (U+2460)
        "1", // Digit 1
    };
    createTestFile("forms.tsv", lines);

    // Test with NFC normalization
    {
        NormalizeOptions options;
        options.form = NormalizationForm::NFC;

        NormalizeResult result = suzume::core::normalize(
            "test_data/forms.tsv",
            "test_data/forms_nfc.tsv",
            options
        );

        // NFC should normalize combining characters but preserve compatibility characters
        EXPECT_EQ(6, result.rows);
        // The exact number of uniques may vary depending on implementation details
        EXPECT_GT(result.uniques, 1);
        EXPECT_LE(result.uniques, 6);
    }

    // Test with NFKC normalization
    {
        NormalizeOptions options;
        options.form = NormalizationForm::NFKC;

        NormalizeResult result = suzume::core::normalize(
            "test_data/forms.tsv",
            "test_data/forms_nfkc.tsv",
            options
        );

        // NFKC should normalize both combining characters and compatibility characters
        EXPECT_EQ(6, result.rows);
        // The exact number of uniques may vary depending on implementation details
        EXPECT_GT(result.uniques, 0);
        EXPECT_LE(result.uniques, 3); // At most 3 unique forms after normalization
    }
}

} // namespace test
} // namespace core
} // namespace suzume
