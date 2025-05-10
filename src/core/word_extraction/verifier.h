/**
 * @file verifier.h
 * @brief Candidate verifier for word extraction
 */

#ifndef SUZUME_CORE_WORD_EXTRACTION_VERIFIER_H_
#define SUZUME_CORE_WORD_EXTRACTION_VERIFIER_H_

#include <string>
#include <vector>
#include <functional>
#include <unordered_set>
#include "common.h"
#include "suzume_feedmill.h"

namespace suzume {
namespace core {

/**
 * @brief Candidate verifier class
 *
 * Verifies candidates against original text
 */
class CandidateVerifier {
public:
    /**
     * @brief Constructor
     *
     * @param options Word extraction options
     */
    CandidateVerifier(const WordExtractionOptions& options);

    /**
     * @brief Verify candidates
     *
     * @param candidates Input candidates
     * @param originalTextPath Path to original text
     * @param progressCallback Progress callback function (optional)
     * @return std::vector<VerifiedCandidate> Verified candidates
     */
    std::vector<VerifiedCandidate> verifyCandidates(
        const std::vector<WordCandidate>& candidates,
        const std::string& originalTextPath,
        const std::function<void(double)>& progressCallback = nullptr
    );

private:
    /**
     * @brief Text index for efficient search
     */
    class TextIndex {
    public:
        /**
         * @brief Constructor
         *
         * @param textPath Path to text file
         */
        TextIndex(const std::string& textPath);

        /**
         * @brief Check if text contains pattern
         *
         * @param pattern Pattern to search for
         * @return bool True if pattern is found
         */
        bool contains(const std::string& pattern) const;

        /**
         * @brief Find all occurrences of pattern
         *
         * @param pattern Pattern to search for
         * @return std::vector<size_t> Positions of pattern occurrences
         */
        std::vector<size_t> findAll(const std::string& pattern) const;

        /**
         * @brief Get context around position
         *
         * @param position Position in text
         * @param contextSize Context size (characters before and after)
         * @return std::string Context string
         */
        std::string getContext(size_t position, size_t contextSize = 20) const;

    private:
        std::string text_;
        // Additional index structures (e.g., suffix array, bloom filter)
    };

    /**
     * @brief Verify candidate in text
     *
     * @param candidate Candidate to verify
     * @param textIndex Text index
     * @return bool True if candidate is verified
     */
    bool verifyInText(const WordCandidate& candidate, const TextIndex& textIndex);

    /**
     * @brief Analyze context
     *
     * @param candidate Candidate to analyze
     * @param textIndex Text index
     * @return std::pair<std::string, double> Context and context score
     */
    std::pair<std::string, double> analyzeContext(const WordCandidate& candidate, const TextIndex& textIndex);

    /**
     * @brief Validate statistically
     *
     * @param candidate Candidate to validate
     * @param textIndex Text index
     * @return double Statistical score
     */
    double validateStatistically(const WordCandidate& candidate, const TextIndex& textIndex);

    /**
     * @brief Lookup in dictionary
     *
     * @param candidate Candidate to lookup
     * @return bool True if candidate is in dictionary
     */
    bool lookupInDictionary(const WordCandidate& candidate);

    WordExtractionOptions options_;
    std::unordered_set<std::string> dictionary_; // Dictionary (if used)
};

} // namespace core
} // namespace suzume

#endif // SUZUME_CORE_WORD_EXTRACTION_VERIFIER_H_
