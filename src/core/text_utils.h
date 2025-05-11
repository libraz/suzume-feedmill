/**
 * @file text_utils.h
 * @brief Text processing utilities
 */

#ifndef SUZUME_CORE_TEXT_UTILS_H_
#define SUZUME_CORE_TEXT_UTILS_H_

#include <string>
#include <vector>
#include <unordered_set>
#include <random>
#include "suzume_feedmill.h"

namespace suzume {
namespace core {

/**
 * @brief Normalize a line of text
 *
 * @param line Input line
 * @param form Normalization form
 * @return std::string Normalized line
 */
std::string normalizeLine(const std::string& line, NormalizationForm form);

/**
 * @brief Check if a line should be excluded
 *
 * @param line Input line
 * @return bool True if line should be excluded
 */
bool shouldExcludeLine(const std::string& line);

/**
 * @brief Check if a line should be excluded with length filters
 *
 * @param line Input line
 * @param minLength Minimum line length (0 = no minimum)
 * @param maxLength Maximum line length (0 = no maximum)
 * @return bool True if line should be excluded
 */
bool shouldExcludeLine(const std::string& line, uint32_t minLength, uint32_t maxLength);

/**
 * @brief Generate n-grams from text
 *
 * @param text Input text
 * @param n N-gram size
 * @return std::vector<std::string> Generated n-grams
 */
std::vector<std::string> generateNgrams(const std::string& text, int n);

/**
 * @brief Calculate hash for a string
 *
 * @param str Input string
 * @return uint64_t Hash value
 */
uint64_t calculateHash(const std::string& str);

/**
 * @brief Check if a string is a duplicate
 *
 * @param str Input string
 * @param uniqueSet Set of unique strings
 * @param bloomFalsePositiveRate Bloom filter false positive rate
 * @return bool True if string is a duplicate
 */
bool isDuplicate(const std::string& str,
                std::unordered_set<std::string>& uniqueSet,
                double bloomFalsePositiveRate);

/**
 * @brief Sample N lines from a file using Reservoir sampling
 *
 * @param inputPath Path to input file
 * @param sampleSize Number of lines to sample
 * @param seed Random seed (0 for time-based seed)
 * @return std::vector<std::string> Sampled lines
 */
std::vector<std::string> sampleLines(
    const std::string& inputPath,
    size_t sampleSize,
    unsigned int seed = 0
);

/**
 * @brief Sample N lines from a vector of strings using Reservoir sampling
 *
 * @param lines Vector of input lines
 * @param sampleSize Number of lines to sample
 * @param seed Random seed (0 for time-based seed)
 * @return std::vector<std::string> Sampled lines
 */
std::vector<std::string> sampleLines(
    const std::vector<std::string>& lines,
    size_t sampleSize,
    unsigned int seed = 0
);

} // namespace core
} // namespace suzume

#endif // SUZUME_CORE_TEXT_UTILS_H_
