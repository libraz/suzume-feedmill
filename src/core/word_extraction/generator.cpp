/**
 * @file generator.cpp
 * @brief Implementation of candidate generator for word extraction
 */

#include "generator.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <thread>
#include <future>
#include <iostream>

namespace suzume {
namespace core {

// Helper function to read PMI results from file
std::vector<std::tuple<std::string, double, uint32_t>> readPmiResults(
    const std::string& pmiResultsPath,
    double minPmiScore,
    const std::function<void(double)>& progressCallback = nullptr
) {
    std::vector<std::tuple<std::string, double, uint32_t>> results;
    std::ifstream file(pmiResultsPath);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open PMI results file: " + pmiResultsPath);
    }

    // Validate minPmiScore
    if (minPmiScore < 0) {
        throw std::invalid_argument("Minimum PMI score must be non-negative");
    }

    // Get file size for progress reporting
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Check if file is empty
    if (fileSize == 0) {
        throw std::runtime_error("PMI results file is empty: " + pmiResultsPath);
    }

    std::string line;
    size_t processedBytes = 0;
    size_t lineCount = 0;

    // Skip header if present
    if (std::getline(file, line) && line.find("ngram") != std::string::npos) {
        processedBytes += line.size() + 1; // +1 for newline
    } else {
        // If no header, reset file position
        file.seekg(0, std::ios::beg);
    }

    // Read PMI results
    while (std::getline(file, line)) {
        processedBytes += line.size() + 1; // +1 for newline
        lineCount++;

        std::istringstream iss(line);
        std::string ngram;
        double score;
        uint32_t freq;

        if (std::getline(iss, ngram, '\t') &&
            iss >> score &&
            iss >> freq) {

            if (score >= minPmiScore) {
                results.emplace_back(ngram, score, freq);
            }
        } else {
            // Log malformed line but continue processing
            std::cerr << "Warning: Malformed line in PMI results file: " << line << std::endl;
        }

        // Report progress
        if (progressCallback && fileSize > 0) {
            progressCallback(static_cast<double>(processedBytes) / fileSize * 0.25); // Reading is 25% of total progress
        }
    }

    // Check if we read any valid lines
    if (lineCount == 0) {
        throw std::runtime_error("No valid data found in PMI results file: " + pmiResultsPath);
    }

    // Check if we found any results
    if (results.empty()) {
        std::cerr << "Warning: No n-grams with PMI score >= " << minPmiScore << " found in " << pmiResultsPath << std::endl;
    }

    return results;
}

CandidateGenerator::CandidateGenerator(const WordExtractionOptions& options)
    : options_(options)
{
}

std::vector<WordCandidate> CandidateGenerator::generateCandidates(
    const std::string& pmiResultsPath,
    const std::function<void(double)>& progressCallback
) {
    // Read PMI results
    auto ngrams = readPmiResults(pmiResultsPath, options_.minPmiScore, progressCallback);

    // Build tries
    for (const auto& [ngram, score, freq] : ngrams) {
        forwardTrie_.add(ngram, score, freq);
        backwardTrie_.add(ngram, score, freq);
    }

    // Generate candidates
    if (options_.useParallelProcessing && options_.threads != 1) {
        return generateCandidatesParallel(ngrams);
    } else {
        return generateCandidatesSequential(ngrams);
    }
}

std::vector<WordCandidate> CandidateGenerator::generateCandidatesParallel(
    const std::vector<std::tuple<std::string, double, uint32_t>>& ngrams
) {
    // Determine number of threads to use
    unsigned int numThreads = options_.threads;
    if (numThreads == 0) {
        // Auto-detect number of threads
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) {
            // Fallback if hardware_concurrency returns 0
            numThreads = 4;
        }
    }

    // If only 1 thread or very few ngrams, use sequential processing
    if (numThreads == 1 || ngrams.size() < 1000) {
        return generateCandidatesSequential(ngrams);
    }

    // Divide ngrams into chunks for parallel processing
    std::vector<std::vector<std::tuple<std::string, double, uint32_t>>> chunks(numThreads);
    size_t chunkSize = ngrams.size() / numThreads;
    size_t remainder = ngrams.size() % numThreads;

    size_t currentIndex = 0;
    for (unsigned int i = 0; i < numThreads; ++i) {
        size_t currentChunkSize = chunkSize + (i < remainder ? 1 : 0);
        chunks[i].reserve(currentChunkSize);

        for (size_t j = 0; j < currentChunkSize && currentIndex < ngrams.size(); ++j) {
            chunks[i].push_back(ngrams[currentIndex++]);
        }
    }

    // Process each chunk in a separate thread
    std::vector<std::future<std::vector<WordCandidate>>> futures;
    for (const auto& chunk : chunks) {
        futures.push_back(std::async(std::launch::async, [this, &chunk]() {
            return this->generateCandidatesSequential(chunk);
        }));
    }

    // Collect results
    std::vector<WordCandidate> candidates;
    for (auto& future : futures) {
        auto chunkResults = future.get();
        candidates.insert(candidates.end(), chunkResults.begin(), chunkResults.end());
    }

    // Limit number of candidates if needed
    if (candidates.size() > options_.maxCandidates) {
        // Sort by score
        std::sort(candidates.begin(), candidates.end(),
            [](const WordCandidate& a, const WordCandidate& b) {
                return a.score > b.score;
            });

        // Keep only top candidates
        candidates.resize(options_.maxCandidates);
    }

    return candidates;
}

std::vector<WordCandidate> CandidateGenerator::generateCandidatesSequential(
    const std::vector<std::tuple<std::string, double, uint32_t>>& ngrams
) {
    std::vector<WordCandidate> candidates;
    
    // Pre-allocate vector for better performance
    candidates.reserve(std::min(ngrams.size(), static_cast<size_t>(options_.maxCandidates)));

    // Simple implementation for now - just convert n-grams to candidates
    // This will be expanded with more sophisticated candidate generation logic
    for (const auto& [ngram, score, freq] : ngrams) {
        if (ngram.length() <= options_.maxCandidateLength) {
            WordCandidate candidate;
            candidate.text = ngram;
            candidate.score = score;
            candidate.frequency = freq;
            candidate.verified = false;

            candidates.emplace_back(std::move(candidate)); // Use emplace_back for efficiency
        }
    }

    // Limit number of candidates if needed
    if (candidates.size() > options_.maxCandidates) {
        // Use partial_sort for better performance when we only need top K elements
        std::partial_sort(candidates.begin(), 
                         candidates.begin() + options_.maxCandidates,
                         candidates.end(),
                         [](const WordCandidate& a, const WordCandidate& b) {
                             return a.score > b.score;
                         });

        // Keep only top candidates
        candidates.resize(options_.maxCandidates);
    }

    return candidates;
}

} // namespace core
} // namespace suzume
