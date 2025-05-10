/**
 * @file generator.h
 * @brief Candidate generator for word extraction
 */

#ifndef SUZUME_CORE_WORD_EXTRACTION_GENERATOR_H_
#define SUZUME_CORE_WORD_EXTRACTION_GENERATOR_H_

#include <string>
#include <vector>
#include <tuple>
#include <functional>
#include "common.h"
#include "trie.h"
#include "suzume_feedmill.h"

namespace suzume {
namespace core {

/**
 * @brief Candidate generator class
 *
 * Generates word candidates from PMI results
 */
class CandidateGenerator {
public:
    /**
     * @brief Constructor
     *
     * @param options Word extraction options
     */
    CandidateGenerator(const WordExtractionOptions& options);

    /**
     * @brief Generate candidates from PMI results
     *
     * @param pmiResultsPath Path to PMI results file
     * @param progressCallback Progress callback function (optional)
     * @return std::vector<WordCandidate> Generated candidates
     */
    std::vector<WordCandidate> generateCandidates(
        const std::string& pmiResultsPath,
        const std::function<void(double)>& progressCallback = nullptr
    );

private:
    /**
     * @brief Generate candidates in parallel
     *
     * @param ngrams Input n-grams
     * @return std::vector<WordCandidate> Generated candidates
     */
    std::vector<WordCandidate> generateCandidatesParallel(
        const std::vector<std::tuple<std::string, double, uint32_t>>& ngrams
    );

    /**
     * @brief Generate candidates sequentially
     *
     * @param ngrams Input n-grams
     * @return std::vector<WordCandidate> Generated candidates
     */
    std::vector<WordCandidate> generateCandidatesSequential(
        const std::vector<std::tuple<std::string, double, uint32_t>>& ngrams
    );

    WordExtractionOptions options_;
    NGramTrie forwardTrie_;  // For prefix matching
    NGramTrie backwardTrie_; // For suffix matching
};

} // namespace core
} // namespace suzume

#endif // SUZUME_CORE_WORD_EXTRACTION_GENERATOR_H_
