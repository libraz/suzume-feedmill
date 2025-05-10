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

    WordExtractionOptions options_;
};

} // namespace core
} // namespace suzume

#endif // SUZUME_CORE_WORD_EXTRACTION_FILTER_H_
