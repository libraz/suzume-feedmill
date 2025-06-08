/**
 * @file trie.cpp
 * @brief Implementation of N-gram trie for efficient prefix/suffix matching
 */

#include "trie.h"

namespace suzume {
namespace core {

NGramTrie::NGramTrie() : root_(std::make_unique<Node>()), nodeCount_(1) {}

void NGramTrie::add(
    const std::string& ngram,
    double score,
    uint32_t frequency
) {
    Node* current = root_.get();

    // Forward trie insertion
    for (char c : ngram) {
        if (!current->children[c]) {
            current->children[c] = std::make_unique<Node>();
            nodeCount_++;
        }
        current = current->children[c].get();
    }

    // Mark end of word and store score and frequency
    current->isEndOfWord = true;
    current->score = score;
    current->frequency = frequency;
    current->ngram = ngram;
}

std::vector<std::tuple<std::string, double, uint32_t>> NGramTrie::findByPrefix(
    const std::string& prefix
) const {
    std::vector<std::tuple<std::string, double, uint32_t>> results;

    // Find the node corresponding to the prefix
    const Node* current = root_.get();
    for (char c : prefix) {
        auto it = current->children.find(c);
        if (it == current->children.end()) {
            // Prefix not found
            return results;
        }
        current = it->second.get();
    }

    // Collect all words with the given prefix
    collectWords(current, results);
    return results;
}

std::vector<std::tuple<std::string, double, uint32_t>> NGramTrie::findBySuffix(
    const std::string& suffix
) const {
    // For suffix search, we need to check all nodes
    // This is less efficient than prefix search but necessary for suffix matching
    std::vector<std::tuple<std::string, double, uint32_t>> results;
    collectWordsBySuffix(root_.get(), suffix, results);
    return results;
}

void NGramTrie::collectWords(
    const Node* node,
    std::vector<std::tuple<std::string, double, uint32_t>>& results
) const {
    if (!node) return;

    // Add this node if it's an end of word
    if (node->isEndOfWord) {
        results.emplace_back(node->ngram, node->score, node->frequency);
    }

    // Recursively collect words from all children
    for (const auto& [_, child] : node->children) {
        collectWords(child.get(), results);
    }
}

void NGramTrie::collectWordsBySuffix(
    const Node* node,
    const std::string& suffix,
    std::vector<std::tuple<std::string, double, uint32_t>>& results
) const {
    if (!node) return;

    // Add this node if it's an end of word and ends with the suffix
    if (node->isEndOfWord) {
        const std::string& ngram = node->ngram;
        if (ngram.length() >= suffix.length() &&
            ngram.substr(ngram.length() - suffix.length()) == suffix) {
            results.emplace_back(ngram, node->score, node->frequency);
        }
    }

    // Recursively collect words from all children
    for (const auto& [_, child] : node->children) {
        collectWordsBySuffix(child.get(), suffix, results);
    }
}

size_t NGramTrie::getMemoryUsage() {
    // Estimate memory usage based on node count
    return nodeCount_ * (sizeof(Node) + 32); // Rough estimate including overhead
}

size_t NGramTrie::getNodeCount() {
    return nodeCount_;
}

size_t NGramTrie::countNodes(const Node* node) const {
    if (!node) return 0;
    
    size_t count = 1; // Count this node
    for (const auto& [_, child] : node->children) {
        count += countNodes(child.get());
    }
    return count;
}

} // namespace core
} // namespace suzume
