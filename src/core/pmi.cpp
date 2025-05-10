/**
 * @file pmi.cpp
 * @brief Implementation of PMI (Pointwise Mutual Information) calculation functionality
 */

#include "core/pmi.h"
#include "core/text_utils.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <atomic>

namespace suzume {
namespace core {

PmiResult calculatePmi(
    const std::string& inputPath,
    const std::string& outputPath,
    const PmiOptions& options
) {
    // Use structured callback if provided, otherwise use simple callback
    if (options.structuredProgressCallback) {
        return calculatePmiWithStructuredProgress(inputPath, outputPath, options.structuredProgressCallback, options);
    } else if (options.progressCallback) {
        return calculatePmiWithProgress(inputPath, outputPath, options.progressCallback, options);
    } else {
        // Create a no-op callback
        auto noopCallback = [](double) {};
        return calculatePmiWithProgress(inputPath, outputPath, noopCallback, options);
    }
}

PmiResult calculatePmiWithStructuredProgress(
    const std::string& inputPath,
    const std::string& outputPath,
    const std::function<void(const ProgressInfo&)>& progressCallback,
    const PmiOptions& options
) {
    try {
        // Track last reported progress to avoid excessive callbacks
        // Use atomic for thread safety
        std::atomic<double> lastReportedProgress{0.0};

        // Start timing
        auto startTime = std::chrono::high_resolution_clock::now();

        // Initial progress report
        ProgressInfo info;
        info.phase = ProgressInfo::Phase::Reading;
        info.phaseRatio = 0.0;
        info.overallRatio = 0.0;
        progressCallback(info);

        // Check if input file exists
        if (!std::filesystem::exists(inputPath)) {
            throw std::runtime_error("Input file does not exist: " + inputPath);
        }

        // Early validation of options
        if (options.n < 1 || options.n > 3) {
            throw std::invalid_argument("Invalid n-gram size: " + std::to_string(options.n) + " (must be 1, 2, or 3)");
        }

        if (options.topK < 1) {
            throw std::invalid_argument("Invalid topK: " + std::to_string(options.topK) + " (must be at least 1)");
        }

        if (options.minFreq < 1) {
            throw std::invalid_argument("Invalid minFreq: " + std::to_string(options.minFreq) + " (must be at least 1)");
        }

        // Determine number of threads
        unsigned int numThreads = options.threads;
        if (numThreads == 0) {
            numThreads = std::thread::hardware_concurrency();
        }

        // Log configuration if verbose mode is enabled
        if (options.verbose) {
            std::cerr << "PMI calculation started with:" << std::endl
                      << "  n-gram size: " << options.n << std::endl
                      << "  topK: " << options.topK << std::endl
                      << "  minFreq: " << options.minFreq << std::endl
                      << "  threads: " << numThreads << std::endl;
        }

        // Read the entire input file
        std::string text;
        size_t fileSize = 0;

        try {
            fileSize = std::filesystem::file_size(inputPath);
            text.reserve(fileSize);
        } catch (const std::exception& e) {
            // Continue without reserving space
        }

        // Open input file
        std::ifstream inputFile(inputPath, std::ios::binary);
        if (!inputFile) {
            throw std::runtime_error("Failed to open input file: " + inputPath);
        }

        // Read file content
        std::string line;
        size_t bytesRead = 0;

        while (std::getline(inputFile, line)) {
            text += line;
            text += '\n';

            bytesRead += line.size() + 1; // +1 for newline

            // Update progress for reading phase
            if (fileSize > 0) {
                double progress = static_cast<double>(bytesRead) / fileSize;

                // Update progress info
                info.phase = ProgressInfo::Phase::Reading;
                info.phaseRatio = progress;
                info.overallRatio = progress * 0.3; // Reading is about 30% of total work

                // Report at specific thresholds or if significant progress made
                double currentProgress = lastReportedProgress.load();
                if (info.overallRatio >= currentProgress + options.progressStep ||
                    (progress >= 0.99 && currentProgress < 0.29)) {
                    progressCallback(info);
                    lastReportedProgress.store(info.overallRatio);
                }
            }
        }

        // Update progress after reading complete
        info.phase = ProgressInfo::Phase::Processing;
        info.phaseRatio = 0.0;
        info.overallRatio = 0.3; // Reading phase complete
        progressCallback(info);
        lastReportedProgress.store(info.overallRatio);

        // Count n-grams
        std::unordered_map<std::string, uint32_t> ngramCounts;

        if (numThreads > 1 && text.size() > 10000) {
            // Parallel n-gram counting for large inputs
            std::vector<std::thread> threads;
            std::vector<std::unordered_map<std::string, uint32_t>> threadCounts(numThreads);
            std::mutex progressMutex;

            // Calculate chunk size
            size_t chunkSize = text.size() / numThreads;
            if (chunkSize == 0) chunkSize = 1;

            // Launch threads
            for (unsigned int i = 0; i < numThreads; ++i) {
                size_t start = i * chunkSize;
                size_t end = (i == numThreads - 1) ? text.size() : (i + 1) * chunkSize;

                // Adjust end to avoid cutting in the middle of a character
                while (end < text.size() && (text[end] & 0xC0) == 0x80) {
                    end++;
                }

                threads.emplace_back([&, i, start, end]() {
                    // Extract chunk
                    std::string chunk = text.substr(start, end - start);

                    // Count n-grams in chunk
                    threadCounts[i] = countNgrams(chunk, options.n);

                    // Update progress
                    {
                        std::lock_guard<std::mutex> lock(progressMutex);

                        // Calculate thread progress
                        double threadProgress = static_cast<double>(i + 1) / numThreads;

                        // Update progress info
                        ProgressInfo threadInfo;
                        threadInfo.phase = ProgressInfo::Phase::Processing;
                        threadInfo.phaseRatio = threadProgress;
                        threadInfo.overallRatio = 0.3 + threadProgress * 0.5; // Processing is 50% of total work

                        // Report progress if significant
                        double currentProgress = lastReportedProgress.load();
                        if (threadInfo.overallRatio >= currentProgress + options.progressStep) {
                            progressCallback(threadInfo);
                            lastReportedProgress.store(threadInfo.overallRatio);
                        }
                    }
                });
            }

            // Wait for all threads to complete
            for (auto& thread : threads) {
                thread.join();
            }

            // Merge results
            for (const auto& counts : threadCounts) {
                for (const auto& [ngram, count] : counts) {
                    ngramCounts[ngram] += count;
                }
            }
        } else {
            // Single-threaded n-gram counting
            ngramCounts = countNgrams(text, options.n);

            // Update progress
            info.phase = ProgressInfo::Phase::Processing;
            info.phaseRatio = 1.0;
            info.overallRatio = 0.8; // Processing complete
            progressCallback(info);
            lastReportedProgress.store(info.overallRatio);
        }

        // Update progress for calculation phase
        info.phase = ProgressInfo::Phase::Calculating;
        info.phaseRatio = 0.0;
        info.overallRatio = 0.8;
        progressCallback(info);
        lastReportedProgress.store(info.overallRatio);

        // Calculate PMI scores
        std::vector<PmiItem> pmiScores = calculatePmiScores(ngramCounts, options.n, options.minFreq);

        // Update progress after calculation
        info.phase = ProgressInfo::Phase::Calculating;
        info.phaseRatio = 1.0;
        info.overallRatio = 0.9;
        progressCallback(info);
        lastReportedProgress.store(info.overallRatio);

        // Sort by PMI score (descending)
        std::sort(pmiScores.begin(), pmiScores.end(), [](const PmiItem& a, const PmiItem& b) {
            return a.score > b.score;
        });

        // Limit to top K results
        if (pmiScores.size() > options.topK) {
            pmiScores.resize(options.topK);
        }

        // Update progress for writing phase
        info.phase = ProgressInfo::Phase::Writing;
        info.phaseRatio = 0.0;
        info.overallRatio = 0.9;
        progressCallback(info);
        lastReportedProgress.store(info.overallRatio);

        // Check if output is null (special case for no output)
        if (outputPath != "null") {
            // Open output file
            std::ofstream outputFile(outputPath, std::ios::binary);
            if (!outputFile) {
                throw std::runtime_error("Failed to open output file: " + outputPath);
            }

            // Write header
            outputFile << "ngram\tpmi\tfrequency\n";

            // Write results
            for (const auto& item : pmiScores) {
                outputFile << item.ngram << "\t" << item.score << "\t" << item.frequency << "\n";
            }
        }

        // Final progress update
        info.phase = ProgressInfo::Phase::Complete;
        info.phaseRatio = 1.0;
        info.overallRatio = 1.0;
        progressCallback(info);

        // Calculate elapsed time
        auto endTime = std::chrono::high_resolution_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        // Calculate processing speed
        double mbProcessed = static_cast<double>(fileSize) / (1024 * 1024);
        double mbPerSec = mbProcessed / (static_cast<double>(elapsedMs) / 1000.0);

        // Log results if verbose mode is enabled
        if (options.verbose) {
            std::cerr << "PMI calculation completed:" << std::endl
                      << "  Total n-grams: " << ngramCounts.size() << std::endl
                      << "  Elapsed time: " << elapsedMs << " ms" << std::endl
                      << "  Processing speed: " << mbPerSec << " MB/s" << std::endl;
        }

        // Return results
        PmiResult result;
        result.grams = ngramCounts.size();
        result.distinctNgrams = pmiScores.size();
        result.elapsedMs = elapsedMs;
        result.mbPerSec = mbPerSec;
        return result;
    } catch (const std::exception& e) {
        std::cerr << "Exception in calculatePmi(): " << e.what() << std::endl;

        // Report error progress
        ProgressInfo errorInfo;
        errorInfo.phase = ProgressInfo::Phase::Complete;
        errorInfo.phaseRatio = 1.0;
        errorInfo.overallRatio = 1.0;
        progressCallback(errorInfo);

        throw; // Re-throw the exception
    } catch (...) {
        std::cerr << "Unknown exception in calculatePmi()" << std::endl;

        // Report error progress
        ProgressInfo errorInfo;
        errorInfo.phase = ProgressInfo::Phase::Complete;
        errorInfo.phaseRatio = 1.0;
        errorInfo.overallRatio = 1.0;
        progressCallback(errorInfo);

        throw; // Re-throw the exception
    }
}

PmiResult calculatePmiWithProgress(
    const std::string& inputPath,
    const std::string& outputPath,
    const std::function<void(double)>& progressCallback,
    const PmiOptions& options
) {
    // If no callback provided, just call the structured version with a null callback
    if (!progressCallback) {
        return calculatePmiWithStructuredProgress(inputPath, outputPath, nullptr, options);
    }

    try {
        // Create a shared pointer to the callback to ensure it remains valid
        // throughout the lifetime of the structured callback
        auto callbackPtr = std::make_shared<std::function<void(double)>>(progressCallback);

        // Create an adapter that converts structured progress to simple progress
        auto structuredCallback = [callbackPtr](const ProgressInfo& info) {
            // Safely call the original callback with the overall ratio
            if (callbackPtr && *callbackPtr) {
                (*callbackPtr)(info.overallRatio);
            }
        };

        // Call the structured version with our adapter
        return calculatePmiWithStructuredProgress(inputPath, outputPath, structuredCallback, options);
    } catch (const std::exception& e) {
        std::cerr << "Exception in calculatePmiWithProgress(): " << e.what() << std::endl;

        // Report error progress if callback exists
        if (progressCallback) {
            // In exception handler, just report 100% directly
            progressCallback(1.0); // Report 100% even on error to ensure callback is called
        }

        throw; // Re-throw the exception
    } catch (...) {
        std::cerr << "Unknown exception in calculatePmiWithProgress()" << std::endl;

        // Report error progress if callback exists
        if (progressCallback) {
            // In exception handler, just report 100% directly
            progressCallback(1.0); // Report 100% even on error to ensure callback is called
        }

        throw; // Re-throw the exception
    }
}

std::unordered_map<std::string, uint32_t> countNgrams(
    const std::string& text,
    uint32_t n
) {
    std::unordered_map<std::string, uint32_t> counts;

    // Split text into lines
    std::istringstream iss(text);
    std::string line;

    while (std::getline(iss, line)) {
        // Skip empty lines
        if (line.empty()) {
            continue;
        }

        // Generate n-grams for this line
        std::vector<std::string> ngrams = generateNgrams(line, n);

        // Count n-grams
        for (const auto& ngram : ngrams) {
            counts[ngram]++;
        }
    }

    return counts;
}

std::vector<PmiItem> calculatePmiScores(
    const std::unordered_map<std::string, uint32_t>& ngramCounts,
    uint32_t n,
    uint32_t minFreq
) {
    std::vector<PmiItem> results;

    // Skip calculation for unigrams
    if (n <= 1) {
        // For unigrams, just return frequency
        for (const auto& [ngram, count] : ngramCounts) {
            if (count >= minFreq) {
                results.push_back({ngram, static_cast<double>(count), count});
            }
        }
        return results;
    }

    // Calculate total count of all n-grams
    uint64_t totalCount = 0;
    for (const auto& [ngram, count] : ngramCounts) {
        totalCount += count;
    }

    // Skip if no n-grams found
    if (totalCount == 0) {
        return results;
    }

    // Count individual characters/components
    std::unordered_map<std::string, uint32_t> componentCounts;

    for (const auto& [ngram, count] : ngramCounts) {
        // Skip n-grams below minimum frequency
        if (count < minFreq) {
            continue;
        }

        // Generate component n-grams
        std::vector<std::string> components = generateNgrams(ngram, 1);

        // Count components
        for (const auto& component : components) {
            componentCounts[component] += count;
        }
    }

    // Calculate PMI scores
    for (const auto& [ngram, count] : ngramCounts) {
        // Skip n-grams below minimum frequency
        if (count < minFreq) {
            continue;
        }

        // Generate component n-grams
        std::vector<std::string> components = generateNgrams(ngram, 1);

        // Skip if components are missing
        bool skipNgram = false;
        for (const auto& component : components) {
            if (componentCounts.find(component) == componentCounts.end()) {
                skipNgram = true;
                break;
            }
        }

        if (skipNgram) {
            continue;
        }

        // Calculate joint probability P(x,y)
        double jointProb = static_cast<double>(count) / totalCount;

        // Calculate marginal probabilities P(x) and P(y)
        double marginalProbProduct = 1.0;
        for (const auto& component : components) {
            double marginalProb = static_cast<double>(componentCounts[component]) / totalCount;
            marginalProbProduct *= marginalProb;
        }

        // Calculate PMI = log(P(x,y) / (P(x) * P(y)))
        double pmi = std::log2(jointProb / marginalProbProduct);

        // Add to results
        results.push_back({ngram, pmi, count});
    }

    return results;
}

} // namespace core
} // namespace suzume
