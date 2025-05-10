/**
 * @file common.h
 * @brief Common definitions for word extraction functionality
 */

#ifndef SUZUME_CORE_WORD_EXTRACTION_COMMON_H_
#define SUZUME_CORE_WORD_EXTRACTION_COMMON_H_

#include <string>
#include <vector>
#include <functional>
#include "suzume_feedmill.h"

namespace suzume {
namespace core {

// Forward declarations
class CandidateGenerator;
class CandidateVerifier;
class CandidateFilter;
class CandidateRanker;

/**
 * @brief Word candidate structure
 */
struct WordCandidate {
    std::string text;       // Candidate text
    double score;           // PMI score
    uint32_t frequency;     // Frequency
    bool verified;          // Verified in original text
};

/**
 * @brief Verified candidate structure
 */
struct VerifiedCandidate {
    std::string text;       // Candidate text
    double score;           // Combined score
    uint32_t frequency;     // Frequency
    std::string context;    // Context (optional)
    double contextScore;    // Context score
    double statisticalScore; // Statistical score
};

/**
 * @brief Ranked candidate structure
 */
struct RankedCandidate {
    std::string text;       // Candidate text
    double score;           // Final score
    uint32_t frequency;     // Frequency
    std::string context;    // Context (optional)
};

} // namespace core
} // namespace suzume

#endif // SUZUME_CORE_WORD_EXTRACTION_COMMON_H_
