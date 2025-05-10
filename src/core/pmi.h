/**
 * @file pmi.h
 * @brief PMI (Pointwise Mutual Information) calculation functionality
 */

#ifndef SUZUME_CORE_PMI_H_
#define SUZUME_CORE_PMI_H_

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <cstdint>
#include "suzume_feedmill.h"
#include "buffer_api.h"

namespace suzume {
namespace core {

/**
 * @brief Calculate PMI (Pointwise Mutual Information)
 *
 * @param inputPath Path to input text file
 * @param outputPath Path to output TSV file
 * @param options PMI calculation options
 * @return PmiResult Results of the PMI calculation
 */
PmiResult calculatePmi(
    const std::string& inputPath,
    const std::string& outputPath,
    const PmiOptions& options = PmiOptions()
);

/**
 * @brief Calculate PMI with progress reporting
 *
 * @param inputPath Path to input text file
 * @param outputPath Path to output TSV file
 * @param progressCallback Progress callback function
 * @param options PMI calculation options
 * @return PmiResult Results of the PMI calculation
 */
/**
 * @brief Calculate PMI with structured progress reporting
 *
 * @param inputPath Path to input text file
 * @param outputPath Path to output TSV file
 * @param progressCallback Structured progress callback function
 * @param options PMI calculation options
 * @return PmiResult Results of the PMI calculation
 */
PmiResult calculatePmiWithStructuredProgress(
    const std::string& inputPath,
    const std::string& outputPath,
    const std::function<void(const ProgressInfo&)>& progressCallback,
    const PmiOptions& options = PmiOptions()
);

/**
 * @brief Calculate PMI with simple progress reporting
 *
 * @param inputPath Path to input text file
 * @param outputPath Path to output TSV file
 * @param progressCallback Progress callback function
 * @param options PMI calculation options
 * @return PmiResult Results of the PMI calculation
 */
PmiResult calculatePmiWithProgress(
    const std::string& inputPath,
    const std::string& outputPath,
    const std::function<void(double)>& progressCallback,
    const PmiOptions& options = PmiOptions()
);

/**
 * @brief PMI result item
 */
struct PmiItem {
    std::string ngram;
    double score;
    uint32_t frequency;

    // Comparison operator for sorting
    bool operator<(const PmiItem& other) const {
        return score < other.score;
    }
};

/**
 * @brief Count n-grams in text
 *
 * @param text Input text
 * @param n N-gram size
 * @return std::unordered_map<std::string, uint32_t> N-gram counts
 */
std::unordered_map<std::string, uint32_t> countNgrams(
    const std::string& text,
    uint32_t n
);

/**
 * @brief Calculate PMI scores for n-grams
 *
 * @param ngramCounts N-gram counts
 * @param n N-gram size
 * @param minFreq Minimum frequency threshold
 * @return std::vector<PmiItem> PMI scores
 */
std::vector<PmiItem> calculatePmiScores(
    const std::unordered_map<std::string, uint32_t>& ngramCounts,
    uint32_t n,
    uint32_t minFreq
);

} // namespace core
} // namespace suzume

#endif // SUZUME_CORE_PMI_H_
