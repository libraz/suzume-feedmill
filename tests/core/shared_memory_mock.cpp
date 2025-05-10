/**
 * @file shared_memory_mock.cpp
 * @brief Mock implementation for shared memory tests
 */

#include "shared_memory_mock.h"
#include <atomic>
#include <cstring>

namespace suzume {
namespace binding {

void UpdateSharedMemoryProgress(
    uint32_t* progressBuffer,
    ProgressPhase phase,
    uint32_t current,
    uint32_t total
) {
    if (!progressBuffer) {
        return;
    }

    // Update phase
    progressBuffer[0] = static_cast<uint32_t>(phase);

    // Update current progress
    progressBuffer[1] = current;

    // Update total
    progressBuffer[2] = total;
}

void UpdateSharedMemoryProgress(
    uint32_t* progressBuffer,
    double ratio
) {
    if (!progressBuffer) {
        return;
    }

    // Determine phase based on ratio
    ProgressPhase phase;
    if (ratio >= 1.0) {
        phase = ProgressPhase::Complete;
    } else if (ratio >= 0.75) {
        phase = ProgressPhase::Writing;
    } else if (ratio >= 0.5) {
        phase = ProgressPhase::Calculating;
    } else if (ratio >= 0.25) {
        phase = ProgressPhase::Processing;
    } else {
        phase = ProgressPhase::Reading;
    }

    // Convert ratio to 0-100 range
    uint32_t current = static_cast<uint32_t>(ratio * 100);
    uint32_t total = 100;

    UpdateSharedMemoryProgress(progressBuffer, phase, current, total);
}

void UpdateSharedMemoryProgress(
    uint32_t* progressBuffer,
    const ProgressInfo& info
) {
    if (!progressBuffer) {
        return;
    }

    // Map ProgressInfo::Phase to ProgressPhase
    ProgressPhase phase;
    switch (info.phase) {
        case ProgressInfo::Phase::Reading:
            phase = ProgressPhase::Reading;
            break;
        case ProgressInfo::Phase::Processing:
            phase = ProgressPhase::Processing;
            break;
        case ProgressInfo::Phase::Calculating:
            phase = ProgressPhase::Calculating;
            break;
        case ProgressInfo::Phase::Writing:
            phase = ProgressPhase::Writing;
            break;
        case ProgressInfo::Phase::Complete:
            phase = ProgressPhase::Complete;
            break;
        default:
            phase = ProgressPhase::Processing;
            break;
    }

    // Convert ratio to 0-100 range
    uint32_t current = static_cast<uint32_t>(info.overallRatio * 100);
    uint32_t total = 100;

    UpdateSharedMemoryProgress(progressBuffer, phase, current, total);
}

std::function<void(double)> CreateSharedMemoryProgressCallback(uint32_t* progressBuffer) {
    if (!progressBuffer) {
        return [](double) {}; // No-op callback
    }

    return [progressBuffer](double ratio) {
        UpdateSharedMemoryProgress(progressBuffer, ratio);
    };
}

std::function<void(const ProgressInfo&)> CreateSharedMemoryStructuredProgressCallback(uint32_t* progressBuffer) {
    if (!progressBuffer) {
        return [](const ProgressInfo&) {}; // No-op callback
    }

    return [progressBuffer](const ProgressInfo& info) {
        UpdateSharedMemoryProgress(progressBuffer, info);
    };
}

} // namespace binding
} // namespace suzume
