/**
 * @file shared_memory_mock.h
 * @brief Mock for shared memory tests
 */

#ifndef SUZUME_TEST_SHARED_MEMORY_MOCK_H_
#define SUZUME_TEST_SHARED_MEMORY_MOCK_H_

#include "suzume_feedmill.h"
#include <cstdint>
#include <functional>

// Mock the necessary parts of the binding namespace for testing
namespace suzume {
namespace binding {

// Progress phase enumeration (copy from shared_memory.h)
enum class ProgressPhase : uint32_t {
    Reading = 0,    ///< Reading input
    Processing = 1, ///< Processing data
    Calculating = 2, ///< Calculating results
    Writing = 3,    ///< Writing output
    Complete = 4    ///< Operation complete
};

// Function declarations (copy from shared_memory.h)
void UpdateSharedMemoryProgress(
    uint32_t* progressBuffer,
    ProgressPhase phase,
    uint32_t current,
    uint32_t total
);

void UpdateSharedMemoryProgress(
    uint32_t* progressBuffer,
    double ratio
);

void UpdateSharedMemoryProgress(
    uint32_t* progressBuffer,
    const ProgressInfo& info
);

std::function<void(double)> CreateSharedMemoryProgressCallback(uint32_t* progressBuffer);

std::function<void(const ProgressInfo&)> CreateSharedMemoryStructuredProgressCallback(uint32_t* progressBuffer);

} // namespace binding
} // namespace suzume

#endif // SUZUME_TEST_SHARED_MEMORY_MOCK_H_
