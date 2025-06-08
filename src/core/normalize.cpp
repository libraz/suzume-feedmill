/**
 * @file normalize.cpp
 * @brief Implementation of text normalization functionality
 */

#include "core/normalize.h"
#include "core/text_utils.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <atomic>

namespace suzume {
namespace core {

NormalizeResult normalize(
    const std::string& inputPath,
    const std::string& outputPath,
    const NormalizeOptions& options
) {
    // Use structured callback if provided, otherwise use simple callback
    if (options.structuredProgressCallback) {
        return normalizeWithStructuredProgress(inputPath, outputPath, options.structuredProgressCallback, options);
    } else if (options.progressCallback) {
        return normalizeWithProgress(inputPath, outputPath, options.progressCallback, options);
    } else {
        // Create a no-op callback
        auto noopCallback = [](double) {};
        return normalizeWithProgress(inputPath, outputPath, noopCallback, options);
    }
}

NormalizeResult normalizeWithProgress(
    const std::string& inputPath,
    const std::string& outputPath,
    const std::function<void(double)>& progressCallback,
    const NormalizeOptions& options
) {
    // If no callback provided, just call the structured version with a null callback
    if (!progressCallback) {
        return normalizeWithStructuredProgress(inputPath, outputPath, nullptr, options);
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
        return normalizeWithStructuredProgress(inputPath, outputPath, structuredCallback, options);
    } catch (const std::exception& e) {
        std::cerr << "Exception in normalizeWithProgress(): " << e.what() << std::endl;

        // Report error progress if callback exists
        if (progressCallback) {
            // In exception handler, just report 100% directly
            if (progressCallback) {
                progressCallback(1.0);
            } // Report 100% even on error to ensure callback is called
        }

        throw; // Re-throw the exception
    } catch (...) {
        std::cerr << "Unknown exception in normalizeWithProgress()" << std::endl;

        // Report error progress if callback exists
        if (progressCallback) {
            // In exception handler, just report 100% directly
            if (progressCallback) {
                progressCallback(1.0);
            } // Report 100% even on error to ensure callback is called
        }

        throw; // Re-throw the exception
    }
}

std::vector<std::string> processBatch(
    const std::vector<std::string>& lines,
    NormalizationForm form,
    double bloomFalsePositiveRate
) {
    return processBatch(lines, form, bloomFalsePositiveRate, 0, 0);
}

std::vector<std::string> processBatch(
    const std::vector<std::string>& lines,
    const NormalizeOptions& options
) {
    return processBatch(lines, options.form, options.bloomFalsePositiveRate,
                       options.minLength, options.maxLength);
}

std::vector<std::string> processBatch(
    const std::vector<std::string>& lines,
    NormalizationForm form,
    double bloomFalsePositiveRate,
    uint32_t minLength,
    uint32_t maxLength
) {
    std::unordered_set<std::string> uniqueSet;
    std::vector<std::string> result;

    // Process each line according to specification
    for (const auto& line : lines) {
        // Check length filters before normalization to save processing time
        if (shouldExcludeLine(line, minLength, maxLength)) {
            continue;
        }

        // Normalize the line (normalizeLine handles whitespace-only lines)
        std::string normalizedLine = normalizeLine(line, form);

        // Skip empty lines (normalizeLine returns empty string for lines to exclude)
        if (normalizedLine.empty()) {
            continue;
        }

        // Apply length filters to normalized line as well
        if (shouldExcludeLine(normalizedLine, minLength, maxLength)) {
            continue;
        }

        // Check for duplicates using xxHash64 and Bloom filter
        if (!isDuplicate(normalizedLine, uniqueSet, bloomFalsePositiveRate)) {
            result.push_back(normalizedLine);
        }
    }

    return result;
}

NormalizeResult normalizeWithStructuredProgress(
    const std::string& inputPath,
    const std::string& outputPath,
    const std::function<void(const ProgressInfo&)>& progressCallback,
    const NormalizeOptions& options
) {
    try {
        // Validate options
        if (options.minLength > 0 && options.maxLength > 0 && options.minLength > options.maxLength) {
            throw std::invalid_argument("Invalid length filters: min-length (" +
                                       std::to_string(options.minLength) +
                                       ") cannot be greater than max-length (" +
                                       std::to_string(options.maxLength) + ")");
        }
        
        // Validate bloom filter false positive rate
        if (options.bloomFalsePositiveRate <= 0.0 || options.bloomFalsePositiveRate >= 1.0) {
            throw std::invalid_argument("Invalid bloomFalsePositiveRate: " +
                                       std::to_string(options.bloomFalsePositiveRate) +
                                       " (must be between 0.0 and 1.0)");
        }
        
        // Validate progress step
        if (options.progressStep <= 0.0 || options.progressStep > 1.0) {
            throw std::invalid_argument("Invalid progressStep: " +
                                       std::to_string(options.progressStep) +
                                       " (must be between 0.0 and 1.0)");
        }

        // Track last reported progress to avoid excessive callbacks
        // Use atomic for thread safety
        std::atomic<double> lastReportedProgress{0.0};

        // Initial progress report
        ProgressInfo info;
        info.phase = ProgressInfo::Phase::Reading;
        info.phaseRatio = 0.0;
        info.overallRatio = 0.0;
        if (progressCallback) {
            progressCallback(info);
        }

        // Check if input is stdin or if file exists
        bool isStdin = (inputPath == "-");
        if (!isStdin && !std::filesystem::exists(inputPath)) {
            throw std::runtime_error("Input file does not exist: " + inputPath);
        }

        // Determine number of threads
        unsigned int numThreads = options.threads;
        if (numThreads == 0) {
            numThreads = std::thread::hardware_concurrency();
        }

        // Read all lines from the input file
        std::vector<std::string> allLines;
        size_t fileSize = 0;

        try {
            fileSize = std::filesystem::file_size(inputPath);
        } catch (const std::exception& e) {
            // Continue without progress reporting
        }

        // Read lines from stdin or file
        std::string line;
        size_t bytesRead = 0;

        if (isStdin) {
            // Read from stdin
            while (std::getline(std::cin, line)) {
                allLines.push_back(line);
                bytesRead += line.size() + 1; // +1 for newline

                // Update progress info (less frequently for stdin)
                if (allLines.size() % 1000 == 0) {
                    info.phase = ProgressInfo::Phase::Reading;
                    info.phaseRatio = 0.5; // Assume we're halfway through for stdin
                    info.overallRatio = 0.25; // Reading is about 50% of total work
                    info.processedBytes = bytesRead;
                    info.totalBytes = 0; // Unknown for stdin

                    if (progressCallback) {
            progressCallback(info);
        }
                    lastReportedProgress.store(info.overallRatio);
                }
            }
        } else {
            // Open input file
            std::ifstream inputFile(inputPath, std::ios::binary);
            if (!inputFile) {
                throw std::runtime_error("Failed to open input file: " + inputPath);
            }

            // Read lines from file
            while (std::getline(inputFile, line)) {
            allLines.push_back(line);

            bytesRead += line.size() + 1; // +1 for newline

            // Limit progress reporting frequency to reduce potential memory issues
            // Only report progress at 25%, 50%, 75% of file reading
            if (fileSize > 0) {
                double progress = static_cast<double>(bytesRead) / fileSize;

                // Update progress info
                info.phase = ProgressInfo::Phase::Reading;
                info.phaseRatio = progress;
                info.overallRatio = progress * 0.5; // Reading is about 50% of total work
                info.processedBytes = bytesRead;
                info.totalBytes = fileSize;

                // Report at specific thresholds or if significant progress made
                if (progressCallback) {
                    double currentProgress = lastReportedProgress.load();
                    if (info.overallRatio >= currentProgress + options.progressStep ||
                        (progress >= 0.99 && currentProgress < 0.49)) {
                        if (progressCallback) {
            progressCallback(info);
        }
                        lastReportedProgress.store(info.overallRatio);
                    }
                }
            }
            }
        }

        // Update progress after reading complete
        info.phase = ProgressInfo::Phase::Processing;
        info.phaseRatio = 0.0;
        info.overallRatio = 0.5; // Reading phase complete
        if (progressCallback) {
            progressCallback(info);
        }
        lastReportedProgress.store(info.overallRatio);

        // Process in parallel or single-threaded based on input size
        std::vector<std::string> uniqueLines;

        // Use parallel processing for larger inputs
        if (allLines.size() > 100 && numThreads > 1) {
            // Calculate chunk size
            size_t chunkSize = allLines.size() / numThreads;
            if (chunkSize == 0) chunkSize = 1;

            // Create threads
            std::vector<std::thread> threads;
            std::vector<std::vector<std::string>> threadResults(numThreads);
            std::mutex progressMutex;
            std::atomic<size_t> processedLines(0);

            // Launch threads
            for (unsigned int i = 0; i < numThreads; ++i) {
                size_t start = i * chunkSize;
                size_t end = (i == numThreads - 1) ? allLines.size() : (i + 1) * chunkSize;

                threads.emplace_back([&, i, start, end]() {
                    // Extract chunk
                    std::vector<std::string> chunk(allLines.begin() + start, allLines.begin() + end);

                    // Process chunk
                    threadResults[i] = processBatch(chunk, options);

                    // Update processed lines count
                    processedLines += (end - start);

                    // Minimize progress callbacks to reduce memory issues
                    // Only report at specific thread completion milestones
                    if (!allLines.empty()) {
                        // Calculate current progress ratio
                        double progressRatio = static_cast<double>(processedLines.load()) / allLines.size();

                        // Update progress info
                        ProgressInfo threadInfo;
                        threadInfo.phase = ProgressInfo::Phase::Processing;
                        threadInfo.phaseRatio = progressRatio;
                        threadInfo.overallRatio = 0.5 + progressRatio * 0.4; // Processing is 40% of total work
                        threadInfo.processedBytes = bytesRead;
                        threadInfo.totalBytes = fileSize;

                        // Report progress if significant and callback exists
                        if (progressCallback) {
                            double currentProgress = lastReportedProgress.load();
                            if (threadInfo.overallRatio >= currentProgress + options.progressStep) {
                                std::lock_guard<std::mutex> lock(progressMutex);
                                // Re-check after acquiring lock to ensure monotonic progress
                                double lockedProgress = lastReportedProgress.load();
                                if (threadInfo.overallRatio > lockedProgress) {
                                    progressCallback(threadInfo);
                                    lastReportedProgress.store(threadInfo.overallRatio);
                                }
                            }
                        }
                    }
                });
            }

            // Wait for all threads to complete
            for (auto& thread : threads) {
                thread.join();
            }

            // Merge results
            std::unordered_set<std::string> uniqueSet;
            for (const auto& result : threadResults) {
                for (const auto& line : result) {
                    if (!line.empty() && uniqueSet.find(line) == uniqueSet.end()) {
                        uniqueSet.insert(line);
                        uniqueLines.push_back(line);
                    }
                }
            }
        } else {
            // Process all lines in single-threaded mode
            uniqueLines = processBatch(allLines, options);

            // Update progress for processing phase
            info.phase = ProgressInfo::Phase::Processing;
            info.phaseRatio = 1.0;
            info.overallRatio = 0.9; // Processing complete
            if (progressCallback) {
            progressCallback(info);
        }
            lastReportedProgress.store(info.overallRatio);
        }

        // Update progress for writing phase
        info.phase = ProgressInfo::Phase::Writing;
        info.phaseRatio = 0.0;
        info.overallRatio = 0.9;
        if (progressCallback) {
            progressCallback(info);
        }
        lastReportedProgress.store(info.overallRatio);

        // Check if output is null (special case for no output) or stdout
        bool isStdout = (outputPath == "-");

        if (outputPath != "null") {
            if (isStdout) {
                // Write to stdout
                for (const auto& line : uniqueLines) {
                    std::cout << line << '\n';
                }
                std::cout.flush();
            } else {
                // Create directory if it doesn't exist
                std::filesystem::path filePath(outputPath);

                try {
                    // Check if the path exists and is a directory
                    if (std::filesystem::exists(outputPath) && std::filesystem::is_directory(outputPath)) {
                        throw std::runtime_error("Cannot write to '" + outputPath + "' because it is a directory");
                    }

                    // Check if the parent path exists or can be created
                    if (!filePath.parent_path().empty()) {
                        std::filesystem::create_directories(filePath.parent_path());
                    }
                } catch (const std::filesystem::filesystem_error& e) {
                    // Handle filesystem errors
                    throw std::runtime_error("Failed to create directory for output file: " +
                                            std::string(e.what()));
                }

                // Open output file
                std::ofstream outputFile(outputPath, std::ios::binary);
                if (!outputFile) {
                    // Provide more detailed error message based on errno
                    std::string errorMsg;
                    if (errno == EACCES || errno == EPERM) {
                        errorMsg = "Permission denied: Cannot write to " + outputPath;
                    } else if (errno == ENOENT) {
                        errorMsg = "Directory does not exist: " + outputPath;
                    } else if (errno == EISDIR) {
                        errorMsg = "Cannot write to '" + outputPath + "' because it is a directory";
                    } else {
                        errorMsg = "Failed to open output file: " + outputPath;
                    }
                    throw std::runtime_error(errorMsg);
                }

                // Write unique lines to output
                for (const auto& line : uniqueLines) {
                    outputFile << line << '\n';
                }
            }
        }

        // Final progress update
        info.phase = ProgressInfo::Phase::Complete;
        info.phaseRatio = 1.0;
        info.overallRatio = 1.0;
        if (progressCallback) {
            progressCallback(info);
        }

        // Return results
        NormalizeResult result;
        result.rows = allLines.size();
        result.uniques = uniqueLines.size();
        return result;
    } catch (const std::exception& e) {
        std::cerr << "Exception in normalize(): " << e.what() << std::endl;

        // Report error progress
        ProgressInfo errorInfo;
        errorInfo.phase = ProgressInfo::Phase::Complete;
        errorInfo.phaseRatio = 1.0;
        errorInfo.overallRatio = 1.0;
        if (progressCallback) {
            progressCallback(errorInfo);
        }

        throw; // Re-throw the exception
    } catch (...) {
        std::cerr << "Unknown exception in normalize()" << std::endl;

        // Report error progress
        ProgressInfo errorInfo;
        errorInfo.phase = ProgressInfo::Phase::Complete;
        errorInfo.phaseRatio = 1.0;
        errorInfo.overallRatio = 1.0;
        if (progressCallback) {
            progressCallback(errorInfo);
        }

        throw; // Re-throw the exception
    }
}

} // namespace core
} // namespace suzume
