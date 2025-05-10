/**
 * @file word_extraction.cpp
 * @brief Implementation of unknown word extraction functionality
 */

#include "word_extraction.h"
#include <chrono>
#include <algorithm>
#include <stdexcept>
#include <memory>
#include "word_extraction/generator.h"
#include "word_extraction/verifier.h"
#include "word_extraction/filter.h"
#include "word_extraction/ranker.h"

// Forward declarations
namespace suzume {
namespace core {
    uint64_t estimateMemoryUsage(
        const std::vector<WordCandidate>& candidates,
        const std::vector<VerifiedCandidate>& verifiedCandidates,
        const std::vector<VerifiedCandidate>& filteredCandidates,
        const std::vector<RankedCandidate>& rankedCandidates
    );
}
}

namespace suzume {
namespace core {

// Helper function to convert internal results to public API result
WordExtractionResult convertToResult(
    const std::vector<RankedCandidate>& rankedCandidates,
    uint64_t processingTimeMs,
    uint64_t memoryUsageBytes
) {
    WordExtractionResult result;

    result.words.reserve(rankedCandidates.size());
    result.scores.reserve(rankedCandidates.size());
    result.frequencies.reserve(rankedCandidates.size());
    result.contexts.reserve(rankedCandidates.size());

    for (const auto& candidate : rankedCandidates) {
        result.words.push_back(candidate.text);
        result.scores.push_back(candidate.score);
        result.frequencies.push_back(candidate.frequency);
        result.contexts.push_back(candidate.context);
    }

    result.processingTimeMs = processingTimeMs;
    result.memoryUsageBytes = memoryUsageBytes;

    return result;
}

// Main implementation of word extraction
WordExtractionResult extractWords(
    const std::string& pmiResultsPath,
    const std::string& originalTextPath,
    const WordExtractionOptions& options
) {
    // Validate input paths
    if (pmiResultsPath.empty()) {
        throw std::invalid_argument("PMI results file path cannot be empty");
    }

    if (originalTextPath.empty()) {
        throw std::invalid_argument("Original text file path cannot be empty");
    }

    // Validate options
    if (options.minPmiScore < 0) {
        throw std::invalid_argument("Minimum PMI score must be non-negative");
    }

    if (options.maxCandidateLength < 1) {
        throw std::invalid_argument("Maximum candidate length must be at least 1");
    }

    if (options.maxCandidates < 1) {
        throw std::invalid_argument("Maximum number of candidates must be at least 1");
    }

    if (options.minLength < 1) {
        throw std::invalid_argument("Minimum length must be at least 1");
    }

    if (options.maxLength < options.minLength) {
        throw std::invalid_argument("Maximum length must be greater than or equal to minimum length");
    }

    if (options.topK < 1) {
        throw std::invalid_argument("Top K must be at least 1");
    }

    // threads is uint32_t, so it can't be negative
    // No validation needed

    // If progress callbacks are provided, use the appropriate version with progress reporting
    // Note: If both callbacks are provided, simple progress callback takes precedence
    if (options.progressCallback) {
        return extractWordsWithProgress(
            pmiResultsPath,
            originalTextPath,
            options.progressCallback,
            options
        );
    } else if (options.structuredProgressCallback) {
        return extractWordsWithStructuredProgress(
            pmiResultsPath,
            originalTextPath,
            options.structuredProgressCallback,
            options
        );
    }

    // Start timing
    auto startTime = std::chrono::high_resolution_clock::now();

    // Create components
    CandidateGenerator generator(options);
    CandidateVerifier verifier(options);
    CandidateFilter filter(options);
    CandidateRanker ranker(options);

    // Step 1: Generate candidates
    auto candidates = generator.generateCandidates(pmiResultsPath);

    // Step 2: Verify candidates
    auto verifiedCandidates = verifier.verifyCandidates(candidates, originalTextPath);

    // Step 3: Filter candidates
    auto filteredCandidates = filter.filterCandidates(verifiedCandidates);

    // Step 4: Rank candidates
    auto rankedCandidates = ranker.rankCandidates(filteredCandidates);

    // Limit to top K results
    if (rankedCandidates.size() > options.topK) {
        rankedCandidates.resize(options.topK);
    }

    // Calculate processing time
    auto endTime = std::chrono::high_resolution_clock::now();
    uint64_t processingTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime
    ).count();

    // Estimate memory usage
    uint64_t memoryUsageBytes = estimateMemoryUsage(
        candidates, verifiedCandidates, filteredCandidates, rankedCandidates
    );

    // Convert to result
    return convertToResult(rankedCandidates, processingTimeMs, memoryUsageBytes);
}

// Implementation with simple progress reporting
WordExtractionResult extractWordsWithProgress(
    const std::string& pmiResultsPath,
    const std::string& originalTextPath,
    const std::function<void(double)>& progressCallback,
    const WordExtractionOptions& options
) {
    // Create a copy of options with progress callback
    WordExtractionOptions optionsWithProgress = options;
    optionsWithProgress.progressCallback = progressCallback;

    // Start timing
    auto startTime = std::chrono::high_resolution_clock::now();

    // Create components
    CandidateGenerator generator(optionsWithProgress);
    CandidateVerifier verifier(optionsWithProgress);
    CandidateFilter filter(optionsWithProgress);
    CandidateRanker ranker(optionsWithProgress);

    // Step 1: Generate candidates (25% of progress)
    auto candidates = generator.generateCandidates(
        pmiResultsPath,
        [&progressCallback](double ratio) {
            progressCallback(ratio * 0.25);
        }
    );

    // Step 2: Verify candidates (25% of progress)
    auto verifiedCandidates = verifier.verifyCandidates(
        candidates,
        originalTextPath,
        [&progressCallback](double ratio) {
            progressCallback(0.25 + ratio * 0.25);
        }
    );

    // Step 3: Filter candidates (25% of progress)
    auto filteredCandidates = filter.filterCandidates(
        verifiedCandidates,
        [&progressCallback](double ratio) {
            progressCallback(0.5 + ratio * 0.25);
        }
    );

    // Step 4: Rank candidates (25% of progress)
    auto rankedCandidates = ranker.rankCandidates(
        filteredCandidates,
        [&progressCallback](double ratio) {
            progressCallback(0.75 + ratio * 0.25);
        }
    );

    // Limit to top K results
    if (rankedCandidates.size() > options.topK) {
        rankedCandidates.resize(options.topK);
    }

    // Calculate processing time
    auto endTime = std::chrono::high_resolution_clock::now();
    uint64_t processingTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime
    ).count();

    // Estimate memory usage
    uint64_t memoryUsageBytes = estimateMemoryUsage(
        candidates, verifiedCandidates, filteredCandidates, rankedCandidates
    );

    // Report completion
    progressCallback(1.0);

    // Convert to result
    return convertToResult(rankedCandidates, processingTimeMs, memoryUsageBytes);
}

// Implementation with structured progress reporting
WordExtractionResult extractWordsWithStructuredProgress(
    const std::string& pmiResultsPath,
    const std::string& originalTextPath,
    const std::function<void(const ProgressInfo&)>& progressCallback,
    const WordExtractionOptions& options
) {
    // Create a copy of options with structured progress callback
    WordExtractionOptions optionsWithProgress = options;
    optionsWithProgress.structuredProgressCallback = progressCallback;

    // Start timing
    auto startTime = std::chrono::high_resolution_clock::now();

    // Create progress info
    ProgressInfo info;
    info.phase = ProgressInfo::Phase::Reading;
    info.phaseRatio = 0.0;
    info.overallRatio = 0.0;

    // Report initial progress
    progressCallback(info);

    // Create components
    CandidateGenerator generator(optionsWithProgress);
    CandidateVerifier verifier(optionsWithProgress);
    CandidateFilter filter(optionsWithProgress);
    CandidateRanker ranker(optionsWithProgress);

    // Step 1: Generate candidates
    info.phase = ProgressInfo::Phase::Reading;
    progressCallback(info);

    auto candidates = generator.generateCandidates(
        pmiResultsPath,
        [&info, &progressCallback](double ratio) {
            info.phaseRatio = ratio;
            info.overallRatio = ratio * 0.25;
            progressCallback(info);
        }
    );

    // Step 2: Verify candidates
    info.phase = ProgressInfo::Phase::Processing;
    info.phaseRatio = 0.0;
    info.overallRatio = 0.25;
    progressCallback(info);

    auto verifiedCandidates = verifier.verifyCandidates(
        candidates,
        originalTextPath,
        [&info, &progressCallback](double ratio) {
            info.phaseRatio = ratio;
            info.overallRatio = 0.25 + ratio * 0.25;
            progressCallback(info);
        }
    );

    // Step 3: Filter candidates
    info.phase = ProgressInfo::Phase::Calculating;
    info.phaseRatio = 0.0;
    info.overallRatio = 0.5;
    progressCallback(info);

    auto filteredCandidates = filter.filterCandidates(
        verifiedCandidates,
        [&info, &progressCallback](double ratio) {
            info.phaseRatio = ratio;
            info.overallRatio = 0.5 + ratio * 0.25;
            progressCallback(info);
        }
    );

    // Step 4: Rank candidates
    info.phase = ProgressInfo::Phase::Writing;
    info.phaseRatio = 0.0;
    info.overallRatio = 0.75;
    progressCallback(info);

    auto rankedCandidates = ranker.rankCandidates(
        filteredCandidates,
        [&info, &progressCallback](double ratio) {
            info.phaseRatio = ratio;
            info.overallRatio = 0.75 + ratio * 0.25;
            progressCallback(info);
        }
    );

    // Limit to top K results
    if (rankedCandidates.size() > options.topK) {
        rankedCandidates.resize(options.topK);
    }

    // Calculate processing time
    auto endTime = std::chrono::high_resolution_clock::now();
    uint64_t processingTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime
    ).count();

    // Estimate memory usage
    uint64_t memoryUsageBytes = estimateMemoryUsage(
        candidates, verifiedCandidates, filteredCandidates, rankedCandidates
    );

    // Report completion
    info.phase = ProgressInfo::Phase::Complete;
    info.phaseRatio = 1.0;
    info.overallRatio = 1.0;
    progressCallback(info);

    // Convert to result
    return convertToResult(rankedCandidates, processingTimeMs, memoryUsageBytes);
}

// Helper function to estimate memory usage
uint64_t estimateMemoryUsage(
    const std::vector<WordCandidate>& candidates,
    const std::vector<VerifiedCandidate>& verifiedCandidates,
    const std::vector<VerifiedCandidate>& filteredCandidates,
    const std::vector<RankedCandidate>& rankedCandidates
) {
    uint64_t memoryUsageBytes = 0;

    // Estimate memory for candidates
    memoryUsageBytes += sizeof(WordCandidate) * candidates.size();
    for (const auto& candidate : candidates) {
        memoryUsageBytes += candidate.text.capacity() * sizeof(char);
    }

    // Estimate memory for verified candidates
    memoryUsageBytes += sizeof(VerifiedCandidate) * verifiedCandidates.size();
    for (const auto& candidate : verifiedCandidates) {
        memoryUsageBytes += candidate.text.capacity() * sizeof(char);
        memoryUsageBytes += candidate.context.capacity() * sizeof(char);
    }

    // Estimate memory for filtered candidates
    memoryUsageBytes += sizeof(VerifiedCandidate) * filteredCandidates.size();
    for (const auto& candidate : filteredCandidates) {
        memoryUsageBytes += candidate.text.capacity() * sizeof(char);
        memoryUsageBytes += candidate.context.capacity() * sizeof(char);
    }

    // Estimate memory for ranked candidates
    memoryUsageBytes += sizeof(RankedCandidate) * rankedCandidates.size();
    for (const auto& candidate : rankedCandidates) {
        memoryUsageBytes += candidate.text.capacity() * sizeof(char);
        memoryUsageBytes += candidate.context.capacity() * sizeof(char);
    }

    return memoryUsageBytes;
}

} // namespace core
} // namespace suzume
