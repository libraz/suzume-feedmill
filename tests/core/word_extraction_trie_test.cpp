/**
 * @file word_extraction_trie_test.cpp
 * @brief Tests for NGramTrie class
 */

#include <gtest/gtest.h>
#include "../../src/core/word_extraction/trie.h"

namespace suzume {
namespace core {
namespace test {

class NGramTrieTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Add some test data to the trie
        trie.add("hello", 5.0, 10);
        trie.add("help", 4.0, 8);
        trie.add("helicopter", 3.0, 5);
        trie.add("world", 6.0, 12);
        trie.add("word", 3.5, 7);
    }

    NGramTrie trie;
};

// Test adding and finding by prefix
TEST_F(NGramTrieTest, AddAndFindByPrefix) {
    // Find by prefix "hel"
    auto results = trie.findByPrefix("hel");

    // Should find 3 matches: "hello", "help", "helicopter"
    ASSERT_EQ(results.size(), 3);

    // Check if all expected words are found
    bool foundHello = false;
    bool foundHelp = false;
    bool foundHelicopter = false;

    for (const auto& [text, score, freq] : results) {
        if (text == "hello") {
            foundHello = true;
            EXPECT_DOUBLE_EQ(score, 5.0);
            EXPECT_EQ(freq, 10);
        } else if (text == "help") {
            foundHelp = true;
            EXPECT_DOUBLE_EQ(score, 4.0);
            EXPECT_EQ(freq, 8);
        } else if (text == "helicopter") {
            foundHelicopter = true;
            EXPECT_DOUBLE_EQ(score, 3.0);
            EXPECT_EQ(freq, 5);
        }
    }

    EXPECT_TRUE(foundHello);
    EXPECT_TRUE(foundHelp);
    EXPECT_TRUE(foundHelicopter);
}

// Test finding by suffix
TEST_F(NGramTrieTest, FindBySuffix) {
    // Find by suffix "ld"
    auto results = trie.findBySuffix("ld");

    // Should find 1 match: "world"
    ASSERT_EQ(results.size(), 1);

    const auto& [text, score, freq] = results[0];
    EXPECT_EQ(text, "world");
    EXPECT_DOUBLE_EQ(score, 6.0);
    EXPECT_EQ(freq, 12);
}

// Test finding by prefix with no matches
TEST_F(NGramTrieTest, FindByPrefixNoMatches) {
    auto results = trie.findByPrefix("xyz");
    EXPECT_TRUE(results.empty());
}

// Test finding by suffix with no matches
TEST_F(NGramTrieTest, FindBySuffixNoMatches) {
    auto results = trie.findBySuffix("xyz");
    EXPECT_TRUE(results.empty());
}

// Test finding by prefix with exact match
TEST_F(NGramTrieTest, FindByPrefixExactMatch) {
    auto results = trie.findByPrefix("hello");

    ASSERT_EQ(results.size(), 1);

    const auto& [text, score, freq] = results[0];
    EXPECT_EQ(text, "hello");
    EXPECT_DOUBLE_EQ(score, 5.0);
    EXPECT_EQ(freq, 10);
}

// Test with large number of entries
TEST_F(NGramTrieTest, LargeNumberOfEntries) {
    NGramTrie largeTrie;

    // Add 1000 entries
    for (int i = 0; i < 1000; i++) {
        std::string text = "text" + std::to_string(i);
        double score = i / 100.0;
        uint32_t freq = i;

        largeTrie.add(text, score, freq);
    }

    // Test prefix search
    auto results = largeTrie.findByPrefix("text1");

    // Should find 111 matches: text1, text10, text100, ..., text199
    EXPECT_EQ(results.size(), 111);

    // Test exact match
    results = largeTrie.findByPrefix("text123");
    ASSERT_EQ(results.size(), 1);

    const auto& [text, score, freq] = results[0];
    EXPECT_EQ(text, "text123");
    EXPECT_DOUBLE_EQ(score, 1.23);
    EXPECT_EQ(freq, 123);
}

// Test with Japanese text
TEST_F(NGramTrieTest, JapaneseText) {
    NGramTrie japaneseTrie;

    japaneseTrie.add("こんにちは", 5.0, 10);
    japaneseTrie.add("こんばんは", 4.0, 8);
    japaneseTrie.add("さようなら", 3.0, 5);

    // Test prefix search
    auto results = japaneseTrie.findByPrefix("こん");

    // Should find 2 matches: "こんにちは", "こんばんは"
    ASSERT_EQ(results.size(), 2);

    bool foundKonnichiwa = false;
    bool foundKonbanwa = false;

    for (const auto& [text, score, freq] : results) {
        if (text == "こんにちは") {
            foundKonnichiwa = true;
            EXPECT_DOUBLE_EQ(score, 5.0);
            EXPECT_EQ(freq, 10);
        } else if (text == "こんばんは") {
            foundKonbanwa = true;
            EXPECT_DOUBLE_EQ(score, 4.0);
            EXPECT_EQ(freq, 8);
        }
    }

    EXPECT_TRUE(foundKonnichiwa);
    EXPECT_TRUE(foundKonbanwa);

    // Test suffix search
    results = japaneseTrie.findBySuffix("は");

    // Should find 2 matches: "こんにちは", "こんばんは"
    ASSERT_EQ(results.size(), 2);
}

} // namespace test
} // namespace core
} // namespace suzume
