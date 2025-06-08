/**
 * @file filter.h
 * @brief Candidate filter for word extraction
 */

#ifndef SUZUME_CORE_WORD_EXTRACTION_FILTER_H_
#define SUZUME_CORE_WORD_EXTRACTION_FILTER_H_

#include <string>
#include <vector>
#include <functional>
#include "common.h"
#include "suzume_feedmill.h"

namespace suzume {
namespace core {

/**
 * @brief Candidate filter class
 *
 * Filters verified candidates
 */
class CandidateFilter {
public:
    /**
     * @brief Constructor
     *
     * @param options Word extraction options
     */
    CandidateFilter(const WordExtractionOptions& options);

    /**
     * @brief Filter candidates
     *
     * @param candidates Verified candidates
     * @param progressCallback Progress callback function (optional)
     * @return std::vector<VerifiedCandidate> Filtered candidates
     */
    std::vector<VerifiedCandidate> filterCandidates(
        const std::vector<VerifiedCandidate>& candidates,
        const std::function<void(double)>& progressCallback = nullptr
    );

private:
    /**
     * @brief Remove substring candidates
     *
     * @param candidates Candidates to filter
     * @return std::vector<VerifiedCandidate> Filtered candidates
     */
    std::vector<VerifiedCandidate> removeSubstringCandidates(
        const std::vector<VerifiedCandidate>& candidates
    );

    /**
     * @brief Remove overlapping candidates
     *
     * @param candidates Candidates to filter
     * @return std::vector<VerifiedCandidate> Filtered candidates
     */
    std::vector<VerifiedCandidate> removeOverlappingCandidates(
        const std::vector<VerifiedCandidate>& candidates
    );

    /**
     * @brief Apply language-specific filters
     *
     * @param candidates Candidates to filter
     * @return std::vector<VerifiedCandidate> Filtered candidates
     */
    std::vector<VerifiedCandidate> applyLanguageSpecificFilters(
        const std::vector<VerifiedCandidate>& candidates
    );

    /**
     * @brief Check if two candidate texts are overlapping
     *
     * @param text1 First candidate text
     * @param text2 Second candidate text
     * @return bool True if texts overlap (substring or exact match)
     */
    bool isOverlapping(const std::string& text1, const std::string& text2);

    /**
     * @brief Check if text is a likely valid word candidate for discovery
     *
     * @param text Text to validate
     * @return bool True if text could be a valid word (permissive for new word discovery)
     */
    bool isLikelyValidWordCandidate(const std::string& text);

    /**
     * @brief Check if text is a common Japanese particle or expression
     *
     * @param text Text to check
     * @return bool True if text is a common particle/expression to filter out
     */
    bool isCommonParticleOrExpression(const std::string& text);

    /**
     * @brief Check if text is likely a functional word using heuristics
     *
     * @param text Text to check
     * @param hiraganaCount Number of hiragana characters
     * @param totalChars Total character count
     * @return bool True if text is likely a functional word to filter out
     */
    bool isLikelyFunctionalWord(const std::string& text, int hiraganaCount, int totalChars);

    WordExtractionOptions options_;
};

} // namespace core
} // namespace suzume

#endif // SUZUME_CORE_WORD_EXTRACTION_FILTER_H_
