/**
 * @file buffer_api.cpp
 * @brief Buffer-based API implementation for core processing
 */

#include "buffer_api.h"
#include "normalize.h"
#include "pmi.h"
#include "text_utils.h"
#include <vector>
#include <string>
#include <sstream>
#include <atomic>
#include <memory>
#include <algorithm>
#include <cstring>

namespace suzume {
namespace core {

// Helper function to convert buffer to lines
std::vector<std::string> bufferToLines(const uint8_t* data, size_t length) {
    std::vector<std::string> lines;
    std::string currentLine;

    for (size_t i = 0; i < length; ++i) {
        char c = static_cast<char>(data[i]);
        if (c == '\n') {
            lines.push_back(currentLine);
            currentLine.clear();
        } else {
            currentLine += c;
        }
    }

    // Add the last line if it doesn't end with a newline
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }

    return lines;
}

// Helper function to convert lines to buffer
void linesToBuffer(const std::vector<std::string>& lines, uint8_t** outputData, size_t* outputLength) {
    // Calculate total size needed
    size_t totalSize = 0;
    for (const auto& line : lines) {
        totalSize += line.size() + 1; // +1 for newline
    }

    // Allocate memory
    *outputData = new uint8_t[totalSize];
    *outputLength = totalSize;

    // Copy data
    size_t offset = 0;
    for (const auto& line : lines) {
        std::memcpy(*outputData + offset, line.c_str(), line.size());
        offset += line.size();
        (*outputData)[offset++] = '\n';
    }
}

void updateProgress(uint32_t* progressBuffer, uint32_t phase, uint32_t current, uint32_t total) {
    if (!progressBuffer) {
        return;
    }

    // Update phase
    std::atomic_store_explicit(
        reinterpret_cast<std::atomic<uint32_t>*>(&progressBuffer[0]),
        phase,
        std::memory_order_release
    );

    // Update current progress
    std::atomic_store_explicit(
        reinterpret_cast<std::atomic<uint32_t>*>(&progressBuffer[1]),
        current,
        std::memory_order_release
    );

    // Update total
    std::atomic_store_explicit(
        reinterpret_cast<std::atomic<uint32_t>*>(&progressBuffer[2]),
        total,
        std::memory_order_release
    );
}

NormalizeResult normalizeBuffer(
    const uint8_t* inputData,
    size_t inputLength,
    uint8_t** outputData,
    size_t* outputLength,
    const NormalizeOptions& options,
    uint32_t* progressBuffer
) {
    NormalizeResult result;

    // Initialize progress
    if (progressBuffer) {
        updateProgress(progressBuffer, 0, 0, 100); // Phase 0: Reading
    }

    // Convert buffer to lines
    std::vector<std::string> lines = bufferToLines(inputData, inputLength);

    // Update progress
    if (progressBuffer) {
        updateProgress(progressBuffer, 1, 0, lines.size()); // Phase 1: Processing
    }

    // Process lines in batches
    std::vector<std::string> normalizedLines;
    size_t processedLines = 0;

    // Create a copy of options for internal use
    NormalizeOptions internalOptions = options;

    // Replace progress callback with our own
    if (progressBuffer) {
        internalOptions.progressCallback = [&processedLines, &lines, progressBuffer](double) {
            updateProgress(progressBuffer, 1, processedLines, lines.size());
        };
    }

    // Process lines in batches
    const size_t batchSize = 1000;
    for (size_t i = 0; i < lines.size(); i += batchSize) {
        size_t end = std::min(i + batchSize, lines.size());
        std::vector<std::string> batch(lines.begin() + i, lines.begin() + end);

        auto batchResult = processBatch(batch, internalOptions.form, internalOptions.bloomFalsePositiveRate);
        normalizedLines.insert(normalizedLines.end(), batchResult.begin(), batchResult.end());

        processedLines += batch.size();

        // Update progress
        if (progressBuffer) {
            updateProgress(progressBuffer, 1, processedLines, lines.size());
        }
    }

    // Update progress
    if (progressBuffer) {
        updateProgress(progressBuffer, 2, 0, 100); // Phase 2: Writing
    }

    // Convert normalized lines to buffer
    linesToBuffer(normalizedLines, outputData, outputLength);

    // Update result statistics
    result.rows = lines.size();
    result.uniques = normalizedLines.size();
    result.duplicates = lines.size() - normalizedLines.size();

    // Update progress to complete
    if (progressBuffer) {
        updateProgress(progressBuffer, 4, 100, 100); // Phase 4: Complete
    }

    return result;
}

PmiResult calculatePmiFromBuffer(
    const uint8_t* inputData,
    size_t inputLength,
    uint8_t** outputData,
    size_t* outputLength,
    const PmiOptions& options,
    uint32_t* progressBuffer
) {
    PmiResult result;

    // Initialize progress
    if (progressBuffer) {
        updateProgress(progressBuffer, 0, 0, 100); // Phase 0: Reading
    }

    // Convert buffer to a single string
    std::string text(reinterpret_cast<const char*>(inputData), inputLength);

    // Update progress
    if (progressBuffer) {
        updateProgress(progressBuffer, 1, 0, 100); // Phase 1: Processing
    }

    // Create a copy of options for internal use
    PmiOptions internalOptions = options;

    // Replace progress callback with our own
    if (progressBuffer) {
        internalOptions.progressCallback = [progressBuffer](double ratio) {
            updateProgress(progressBuffer, 1, static_cast<uint32_t>(ratio * 100), 100);
        };
    }

    // Count n-grams
    auto ngramCounts = countNgrams(text, internalOptions.n);

    // Update progress
    if (progressBuffer) {
        updateProgress(progressBuffer, 2, 0, 100); // Phase 2: Calculating
    }

    // Calculate PMI scores
    auto pmiScores = calculatePmiScores(ngramCounts, internalOptions.n, internalOptions.minFreq);

    // Sort by score (descending)
    std::sort(pmiScores.begin(), pmiScores.end(), [](const PmiItem& a, const PmiItem& b) {
        return a.score > b.score;
    });

    // Take top K
    if (pmiScores.size() > internalOptions.topK) {
        pmiScores.resize(internalOptions.topK);
    }

    // Update progress
    if (progressBuffer) {
        updateProgress(progressBuffer, 3, 0, 100); // Phase 3: Writing
    }

    // Convert PMI scores to TSV format
    std::stringstream ss;
    for (const auto& item : pmiScores) {
        ss << item.ngram << "\t" << item.score << "\t" << item.frequency << "\n";
    }

    // Convert to buffer
    std::string output = ss.str();
    *outputLength = output.size();
    *outputData = new uint8_t[*outputLength];
    std::memcpy(*outputData, output.c_str(), *outputLength);

    // Update result statistics
    result.grams = ngramCounts.size();
    result.distinctNgrams = pmiScores.size();

    // Update progress to complete
    if (progressBuffer) {
        updateProgress(progressBuffer, 4, 100, 100); // Phase 4: Complete
    }

    return result;
}

} // namespace core
} // namespace suzume
