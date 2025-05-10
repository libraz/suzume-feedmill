/**
 * @file trie.h
 * @brief N-gram trie for efficient prefix/suffix matching
 */

#ifndef SUZUME_CORE_WORD_EXTRACTION_TRIE_H_
#define SUZUME_CORE_WORD_EXTRACTION_TRIE_H_

#include <string>
#include <vector>
#include <tuple>
#include <memory>
#include <unordered_map>

namespace suzume {
namespace core {

/**
 * @brief N-gram trie for efficient prefix/suffix matching
 */
class NGramTrie {
public:
    /**
     * @brief Constructor
     */
    NGramTrie();

    /**
     * @brief Add n-gram to trie
     *
     * @param ngram N-gram text
     * @param score PMI score
     * @param frequency Frequency
     */
    void add(const std::string& ngram, double score, uint32_t frequency);

    /**
     * @brief Find n-grams with given prefix
     *
     * @param prefix Prefix to search for
     * @return std::vector<std::tuple<std::string, double, uint32_t>> Matching n-grams with scores and frequencies
     */
    std::vector<std::tuple<std::string, double, uint32_t>> findByPrefix(const std::string& prefix) const;

    /**
     * @brief Find n-grams with given suffix
     *
     * @param suffix Suffix to search for
     * @return std::vector<std::tuple<std::string, double, uint32_t>> Matching n-grams with scores and frequencies
     */
    std::vector<std::tuple<std::string, double, uint32_t>> findBySuffix(const std::string& suffix) const;

private:
    // Trie node structure
    class Node {
    public:
        std::unordered_map<char, std::unique_ptr<Node>> children;
        bool isEndOfWord = false;
        double score = 0.0;
        uint32_t frequency = 0;
        std::string ngram;
    };

    /**
     * @brief Collect all words from a node and its children
     *
     * @param node Starting node
     * @param results Vector to store results
     */
    void collectWords(const Node* node, std::vector<std::tuple<std::string, double, uint32_t>>& results) const;

    /**
     * @brief Collect all words ending with a suffix
     *
     * @param node Starting node
     * @param suffix Suffix to match
     * @param results Vector to store results
     */
    void collectWordsBySuffix(const Node* node, const std::string& suffix, std::vector<std::tuple<std::string, double, uint32_t>>& results) const;

    std::unique_ptr<Node> root_;  // Root of the trie
};

} // namespace core
} // namespace suzume

#endif // SUZUME_CORE_WORD_EXTRACTION_TRIE_H_
