/**
 * @file ranker.cpp
 * @brief Implementation of candidate ranker for word extraction
 */

#include "ranker.h"
#include <algorithm>
#include <cmath>

namespace suzume {
namespace core {

CandidateRanker::CandidateRanker(const WordExtractionOptions& options)
    : options_(options)
{
}

std::vector<RankedCandidate> CandidateRanker::rankCandidates(
    const std::vector<VerifiedCandidate>& candidates,
    const std::function<void(double)>& progressCallback
) {
    std::vector<RankedCandidate> rankedCandidates;
    rankedCandidates.reserve(candidates.size());

    // Process each candidate
    size_t total = candidates.size();
    size_t processed = 0;

    for (const auto& candidate : candidates) {
        // Calculate combined score
        double score = calculateCombinedScore(candidate);

        // Create ranked candidate
        RankedCandidate rankedCandidate;
        rankedCandidate.text = candidate.text;
        rankedCandidate.score = score;
        rankedCandidate.frequency = candidate.frequency;
        rankedCandidate.context = candidate.context;

        rankedCandidates.push_back(rankedCandidate);

        // Report progress
        processed++;
        if (progressCallback) {
            progressCallback(static_cast<double>(processed) / total * 0.5);
        }
    }

    // Sort by score (descending)
    std::sort(rankedCandidates.begin(), rankedCandidates.end(),
        [](const RankedCandidate& a, const RankedCandidate& b) {
            return a.score > b.score;
        });

    if (progressCallback) {
        progressCallback(1.0);
    }

    return rankedCandidates;
}

double CandidateRanker::calculateCombinedScore(const VerifiedCandidate& candidate) {
    if (options_.rankingModel == "combined") {
        double pmiScore = calculatePmiScore(candidate);
        double lengthScore = calculateLengthScore(candidate);
        double contextScore = calculateContextScore(candidate);
        double statisticalScore = calculateStatisticalScore(candidate);

        return options_.pmiWeight * pmiScore +
               options_.lengthWeight * lengthScore +
               options_.contextWeight * contextScore +
               options_.statisticalWeight * statisticalScore;
    } else {
        // Default to PMI score
        return candidate.score;
    }
}

double CandidateRanker::calculatePmiScore(const VerifiedCandidate& candidate) {
    // Normalize PMI score to 0-1 range
    return std::min(1.0, candidate.score / 10.0);
}

double CandidateRanker::calculateLengthScore(const VerifiedCandidate& candidate) {
    // Prefer longer candidates, but not too long
    double length = candidate.text.length();
    double optimalLength = 4.0; // Optimal length is around 4 characters

    return std::exp(-(length - optimalLength) * (length - optimalLength) / 8.0);
}

double CandidateRanker::calculateContextScore(const VerifiedCandidate& candidate) {
    return candidate.contextScore;
}

double CandidateRanker::calculateStatisticalScore(const VerifiedCandidate& candidate) {
    return candidate.statisticalScore;
}

} // namespace core
} // namespace suzume
