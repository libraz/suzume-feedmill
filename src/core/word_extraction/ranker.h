/**
 * @file ranker.h
 * @brief Candidate ranker for word extraction
 */

#ifndef SUZUME_CORE_WORD_EXTRACTION_RANKER_H_
#define SUZUME_CORE_WORD_EXTRACTION_RANKER_H_

#include <string>
#include <vector>
#include <functional>
#include "common.h"
#include "suzume_feedmill.h"

namespace suzume {
namespace core {

/**
 * @brief Candidate ranker class
 *
 * Ranks filtered candidates
 */
class CandidateRanker {
public:
    /**
     * @brief Constructor
     *
     * @param options Word extraction options
     */
    CandidateRanker(const WordExtractionOptions& options);

    /**
     * @brief Rank candidates
     *
     * @param candidates Filtered candidates
     * @param progressCallback Progress callback function (optional)
     * @return std::vector<RankedCandidate> Ranked candidates
     */
    std::vector<RankedCandidate> rankCandidates(
        const std::vector<VerifiedCandidate>& candidates,
        const std::function<void(double)>& progressCallback = nullptr
    );

private:
    /**
     * @brief Calculate combined score
     *
     * @param candidate Candidate to score
     * @return double Combined score
     */
    double calculateCombinedScore(const VerifiedCandidate& candidate);

    /**
     * @brief Calculate PMI score
     *
     * @param candidate Candidate to score
     * @return double PMI score
     */
    double calculatePmiScore(const VerifiedCandidate& candidate);

    /**
     * @brief Calculate length score
     *
     * @param candidate Candidate to score
     * @return double Length score
     */
    double calculateLengthScore(const VerifiedCandidate& candidate);

    /**
     * @brief Calculate context score
     *
     * @param candidate Candidate to score
     * @return double Context score
     */
    double calculateContextScore(const VerifiedCandidate& candidate);

    /**
     * @brief Calculate statistical score
     *
     * @param candidate Candidate to score
     * @return double Statistical score
     */
    double calculateStatisticalScore(const VerifiedCandidate& candidate);

    WordExtractionOptions options_;
};

} // namespace core
} // namespace suzume

#endif // SUZUME_CORE_WORD_EXTRACTION_RANKER_H_
