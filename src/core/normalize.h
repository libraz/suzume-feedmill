/**
 * @file normalize.h
 * @brief Text normalization functionality
 */

#ifndef SUZUME_CORE_NORMALIZE_H_
#define SUZUME_CORE_NORMALIZE_H_

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include "suzume_feedmill.h"
#include "buffer_api.h"

namespace suzume {
namespace core {

/**
 * @brief Normalize text data
 *
 * @param inputPath Path to input TSV file
 * @param outputPath Path to output TSV file
 * @param options Normalization options
 * @return NormalizeResult Results of the normalization operation
 */
NormalizeResult normalize(
    const std::string& inputPath,
    const std::string& outputPath,
    const NormalizeOptions& options = NormalizeOptions()
);

/**
 * @brief Normalize text data with progress reporting
 *
 * @param inputPath Path to input TSV file
 * @param outputPath Path to output TSV file
 * @param progressCallback Progress callback function
 * @param options Normalization options
 * @return NormalizeResult Results of the normalization operation
 */
NormalizeResult normalizeWithProgress(
    const std::string& inputPath,
    const std::string& outputPath,
    const std::function<void(double)>& progressCallback,
    const NormalizeOptions& options = NormalizeOptions()
);

/**
 * @brief Normalize text data with structured progress reporting
 *
 * @param inputPath Path to input TSV file
 * @param outputPath Path to output TSV file
 * @param progressCallback Structured progress callback function
 * @param options Normalization options
 * @return NormalizeResult Results of the normalization operation
 */
NormalizeResult normalizeWithStructuredProgress(
    const std::string& inputPath,
    const std::string& outputPath,
    const std::function<void(const ProgressInfo&)>& progressCallback,
    const NormalizeOptions& options = NormalizeOptions()
);

/**
 * @brief Process a batch of lines for normalization
 *
 * @param lines Input lines
 * @param form Normalization form
 * @param bloomFalsePositiveRate Bloom filter false positive rate
 * @return std::vector<std::string> Normalized unique lines
 */
std::vector<std::string> processBatch(
    const std::vector<std::string>& lines,
    NormalizationForm form,
    double bloomFalsePositiveRate
);

/**
 * @brief Process a batch of lines for normalization with length filters
 *
 * @param lines Input lines
 * @param form Normalization form
 * @param bloomFalsePositiveRate Bloom filter false positive rate
 * @param minLength Minimum line length (0 = no minimum)
 * @param maxLength Maximum line length (0 = no maximum)
 * @return std::vector<std::string> Normalized unique lines
 */
std::vector<std::string> processBatch(
    const std::vector<std::string>& lines,
    NormalizationForm form,
    double bloomFalsePositiveRate,
    uint32_t minLength,
    uint32_t maxLength
);

/**
 * @brief Process a batch of lines for normalization
 *
 * @param lines Input lines
 * @param options Normalization options
 * @return std::vector<std::string> Normalized unique lines
 */
std::vector<std::string> processBatch(
    const std::vector<std::string>& lines,
    const NormalizeOptions& options
);

} // namespace core
} // namespace suzume

#endif // SUZUME_CORE_NORMALIZE_H_
