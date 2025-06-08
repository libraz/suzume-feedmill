/**
 * @file filter.cpp
 * @brief Implementation of candidate filter for word extraction
 */

#include "filter.h"
#include <algorithm>
#include <unordered_set>
#include <numeric>

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

    // Keep track of candidates to remove - use efficient set for O(log n) lookups
    std::unordered_set<std::string> toRemove;
    
    // Create a map for faster substring checking - group by length
    std::unordered_map<size_t, std::vector<std::pair<std::string, double>>> lengthGroups;
    for (const auto& candidate : sorted) {
        lengthGroups[candidate.text.length()].emplace_back(candidate.text, candidate.score);
    }
    
    // Check each candidate against potentially containing candidates only
    for (const auto& candidate : sorted) {
        if (toRemove.count(candidate.text)) continue;
        
        // Only check against longer candidates (potential containers)
        for (auto& [length, group] : lengthGroups) {
            if (length <= candidate.text.length()) continue;
            
            for (const auto& [longerText, longerScore] : group) {
                if (toRemove.count(longerText)) continue;
                
                // Check if current candidate is substring of longer candidate
                if (longerText.find(candidate.text) != std::string::npos) {
                    // Mark current candidate for removal if its score is much lower
                    if (candidate.score < longerScore * 0.8) {
                        toRemove.insert(candidate.text);
                        break; // No need to check more containers for this candidate
                    }
                }
            }
            
            if (toRemove.count(candidate.text)) break;
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
    if (candidates.empty()) {
        return candidates;
    }

    std::vector<VerifiedCandidate> result;
    std::vector<bool> removed(candidates.size(), false);

    // Sort candidates by score (descending) to prioritize higher-scoring candidates
    std::vector<size_t> indices(candidates.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), 
        [&candidates](size_t a, size_t b) {
            return candidates[a].score > candidates[b].score;
        });

    // Check each candidate against all others
    for (size_t i = 0; i < indices.size(); ++i) {
        if (removed[indices[i]]) continue;
        
        const auto& current = candidates[indices[i]];
        bool shouldKeep = true;

        // Check against all other candidates
        for (size_t j = 0; j < indices.size(); ++j) {
            if (i == j || removed[indices[j]]) continue;
            
            const auto& other = candidates[indices[j]];
            
            // Check for substring relationship or exact duplicates
            if (isOverlapping(current.text, other.text)) {
                // If current candidate has lower score, mark it for removal
                if (current.score <= other.score) {
                    shouldKeep = false;
                    break;
                } else {
                    // Mark the other candidate for removal
                    removed[indices[j]] = true;
                }
            }
        }

        if (shouldKeep) {
            result.push_back(current);
        } else {
            removed[indices[i]] = true;
        }
    }

    return result;
}

bool CandidateFilter::isOverlapping(const std::string& text1, const std::string& text2) {
    // Exact match
    if (text1 == text2) {
        return true;
    }
    
    // Check if one is a substring of the other
    if (text1.find(text2) != std::string::npos || text2.find(text1) != std::string::npos) {
        return true;
    }
    
    return false;
}

std::vector<VerifiedCandidate> CandidateFilter::applyLanguageSpecificFilters(
    const std::vector<VerifiedCandidate>& candidates
) {
    if (!options_.useLanguageSpecificRules) {
        return candidates;
    }

    std::vector<VerifiedCandidate> filtered;
    
    for (const auto& candidate : candidates) {
        // For new word discovery, we should be PERMISSIVE, not restrictive
        // Only filter out clearly invalid candidates that are likely noise
        if (isLikelyValidWordCandidate(candidate.text)) {
            filtered.push_back(candidate);
        }
    }

    return filtered;
}

bool CandidateFilter::isLikelyValidWordCandidate(const std::string& text) {
    if (text.empty()) {
        return false;
    }

    // For new word discovery, be VERY permissive
    // Only filter out obvious noise patterns
    
    // Filter out single punctuation or symbols
    if (text.length() <= 3) {
        const char* ptr = text.c_str();
        unsigned char c = *ptr;
        
        // Single ASCII punctuation/symbols
        if (text.length() == 1 && (c < 0x30 || (c > 0x39 && c < 0x41) || 
                                   (c > 0x5A && c < 0x61) || c > 0x7A)) {
            return false;
        }
    }
    
    // Filter out strings that are clearly broken encoding
    // Check for valid UTF-8 sequences
    const char* ptr = text.c_str();
    const char* end = ptr + text.length();
    bool hasValidChars = false;
    
    while (ptr < end) {
        unsigned char c = *ptr;
        int bytes = 0;
        
        if (c < 0x80) {
            // ASCII
            bytes = 1;
            hasValidChars = true;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte UTF-8
            bytes = 2;
            hasValidChars = true;
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte UTF-8 (Japanese characters)
            bytes = 3;
            hasValidChars = true;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte UTF-8
            bytes = 4;
            hasValidChars = true;
        } else {
            // Invalid UTF-8 sequence
            return false;
        }
        
        // Verify continuation bytes
        for (int i = 1; i < bytes && ptr + i < end; i++) {
            if ((ptr[i] & 0xC0) != 0x80) {
                return false;  // Invalid continuation byte
            }
        }
        
        ptr += bytes;
    }
    
    // Only filter out whitespace-only or empty content
    if (!hasValidChars) {
        return false;
    }
    
    // Allow everything else - new words could be anything!
    return true;
}

bool CandidateFilter::isLikelyFunctionalWord(const std::string& text, int hiraganaCount, int totalChars) {
    // Length-based heuristics: very short words are often particles
    if (totalChars == 1) {
        // Single character hiragana are almost always particles
        return true;
    }
    
    if (totalChars <= 2) {
        // Two-character hiragana need more sophisticated checking
        // Common two-character particles: から, まで, など, だけ, etc.
        // For now, be conservative and allow them unless they match specific patterns
        return text.find("から") != std::string::npos ||
               text.find("まで") != std::string::npos ||
               text.find("など") != std::string::npos ||
               text.find("だけ") != std::string::npos;
    }
    
    // Pattern-based checks for functional expressions
    if (text.find("ということ") != std::string::npos ||
        text.find("というの") != std::string::npos ||
        text.find("ではない") != std::string::npos ||
        text.find("かもしれ") != std::string::npos ||
        text.find("だろう") != std::string::npos ||
        text.find("である") != std::string::npos) {
        return true;
    }
    
    // Polite form endings
    if (text.length() >= 6) {
        if (text.find("です") == text.length() - 6 ||
            text.find("ます") == text.length() - 6 ||
            text.find("でしょう") == text.length() - 12) {
            return true;
        }
    }
    
    // If none of the above patterns match, assume it's a content word
    return false;
}

bool CandidateFilter::isCommonParticleOrExpression(const std::string& text) {
    // This method is kept for backward compatibility but now delegates to the more sophisticated approach
    return isLikelyFunctionalWord(text, 0, 0);  // We don't have char counts here, so use pattern-only approach
}

} // namespace core
} // namespace suzume
