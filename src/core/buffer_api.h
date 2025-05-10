/**
 * @file buffer_api.h
 * @brief Buffer-based API for core processing
 */

#ifndef SUZUME_CORE_BUFFER_API_H_
#define SUZUME_CORE_BUFFER_API_H_

#include <cstdint>
#include <cstddef>
#include "suzume_feedmill.h"

namespace suzume {
namespace core {

/**
 * @brief Normalize text data from buffer
 *
 * @param inputData Input buffer data
 * @param inputLength Input buffer length
 * @param outputData Output buffer (will be allocated by the function)
 * @param outputLength Output buffer length
 * @param options Normalization options
 * @param progressBuffer Optional shared memory buffer for progress updates
 * @return NormalizeResult Results of the normalization operation
 */
NormalizeResult normalizeBuffer(
    const uint8_t* inputData,
    size_t inputLength,
    uint8_t** outputData,
    size_t* outputLength,
    const NormalizeOptions& options,
    uint32_t* progressBuffer = nullptr
);

/**
 * @brief Calculate PMI from buffer
 *
 * @param inputData Input buffer data
 * @param inputLength Input buffer length
 * @param outputData Output buffer (will be allocated by the function)
 * @param outputLength Output buffer length
 * @param options PMI calculation options
 * @param progressBuffer Optional shared memory buffer for progress updates
 * @return PmiResult Results of the PMI calculation
 */
PmiResult calculatePmiFromBuffer(
    const uint8_t* inputData,
    size_t inputLength,
    uint8_t** outputData,
    size_t* outputLength,
    const PmiOptions& options,
    uint32_t* progressBuffer = nullptr
);

/**
 * @brief Update progress in shared memory buffer
 *
 * @param progressBuffer Shared memory buffer
 * @param phase Processing phase (0-4)
 * @param current Current progress value
 * @param total Total progress value
 */
void updateProgress(
    uint32_t* progressBuffer,
    uint32_t phase,
    uint32_t current,
    uint32_t total
);

} // namespace core
} // namespace suzume

#endif // SUZUME_CORE_BUFFER_API_H_
