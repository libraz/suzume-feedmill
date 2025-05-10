/**
 * @file filter.cpp
 * @brief Implementation of candidate filter for word extraction
 */

#include "filter.h"
#include <algorithm>
#include <unordered_set>

namespace suzume {
namespace core {

CandidateFilter::CandidateFilter(const WordExtractionOptions& options)
    : options_(options)
{
}

std::vector<VerifiedCandidate> CandidateFilter::filterCandidates(
    const std::vector<VerifiedCandidate>& candidates,
    const std::function<void(double)>& progressCallback
) {
    // Filter by length
    std::vector<VerifiedCandidate> lengthFiltered;
    for (const auto& candidate : candidates) {
        if (candidate.text.length() >= options_.minLength &&
            candidate.text.length() <= options_.maxLength) {
            lengthFiltered.push_back(candidate);
        }
    }

    if (progressCallback) {
        progressCallback(0.25);
    }

    // Filter by score
    std::vector<VerifiedCandidate> scoreFiltered;
    for (const auto& candidate : lengthFiltered) {
        if (candidate.score >= options_.minScore) {
            scoreFiltered.push_back(candidate);
        }
    }

    if (progressCallback) {
        progressCallback(0.5);
    }

    // Remove substrings if required
    std::vector<VerifiedCandidate> substringFiltered = scoreFiltered;
    if (options_.removeSubstrings) {
        substringFiltered = removeSubstringCandidates(scoreFiltered);
    }

    if (progressCallback) {
        progressCallback(0.75);
    }

    // Remove overlapping if required
    std::vector<VerifiedCandidate> overlapFiltered = substringFiltered;
    if (options_.removeOverlapping) {
        overlapFiltered = removeOverlappingCandidates(substringFiltered);
    }

    // Apply language-specific filters if required
    std::vector<VerifiedCandidate> languageFiltered = overlapFiltered;
    if (options_.useLanguageSpecificRules) {
        languageFiltered = applyLanguageSpecificFilters(overlapFiltered);
    }

    if (progressCallback) {
        progressCallback(1.0);
    }

    return languageFiltered;
}

std::vector<VerifiedCandidate> CandidateFilter::removeSubstringCandidates(
    const std::vector<VerifiedCandidate>& candidates
) {
    // Sort by length (descending)
    std::vector<VerifiedCandidate> sorted = candidates;
    std::sort(sorted.begin(), sorted.end(),
        [](const VerifiedCandidate& a, const VerifiedCandidate& b) {
            return a.text.length() > b.text.length();
        });

    // Keep track of candidates to remove
    std::unordered_set<std::string> toRemove;

    // Check each candidate
    for (size_t i = 0; i < sorted.size(); i++) {
        const auto& candidate = sorted[i];

        // Skip if already marked for removal
        if (toRemove.find(candidate.text) != toRemove.end()) {
            continue;
        }

        // Check against all shorter candidates
        for (size_t j = i + 1; j < sorted.size(); j++) {
            const auto& other = sorted[j];

            // Skip if already marked for removal
            if (toRemove.find(other.text) != toRemove.end()) {
                continue;
            }

            // Check if other is substring of candidate
            if (candidate.text.find(other.text) != std::string::npos) {
                // Mark for removal if score is lower
                if (other.score < candidate.score * 0.8) {
                    toRemove.insert(other.text);
                }
            }
        }
    }

    // Filter out removed candidates
    std::vector<VerifiedCandidate> filtered;
    for (const auto& candidate : candidates) {
        if (toRemove.find(candidate.text) == toRemove.end()) {
            filtered.push_back(candidate);
        }
    }

    return filtered;
}

std::vector<VerifiedCandidate> CandidateFilter::removeOverlappingCandidates(
    const std::vector<VerifiedCandidate>& candidates
) {
    // Placeholder implementation - will be expanded in future
    return candidates;
}

std::vector<VerifiedCandidate> CandidateFilter::applyLanguageSpecificFilters(
    const std::vector<VerifiedCandidate>& candidates
) {
    // Placeholder implementation - will be expanded in future
    return candidates;
}

} // namespace core
} // namespace suzume
