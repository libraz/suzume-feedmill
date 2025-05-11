/**
 * @file main.cpp
 * @brief Command-line interface for suzume-feedmill
 */

#include <iostream>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <chrono>
#include <nlohmann/json.hpp>
#include "options.h"
#include "core/normalize.h"
#include "core/pmi.h"
#include "core/word_extraction.h"
#include "core/text_utils.h"
#include "io/file_io.h"

// For convenience
using json = nlohmann::json;

int main(int argc, char* argv[]) {
    // Parse command-line options
    suzume::cli::OptionsParser options;
    int parseResult = options.parse(argc, argv);

    // If parsing failed, return the error code
    if (parseResult != 0) {
        return parseResult;
    }

    try {
        // Execute the selected command
        if (options.isNormalizeCommand()) {
            suzume::NormalizeResult result;

            // Check if sampling is enabled
            if (options.getSampleSize() > 0) {
                // Check if input is stdin
                bool isStdin = suzume::io::TextFileReader::isStdin(options.getInputPath());

                if (isStdin) {
                    // Cannot sample from stdin directly, need to read all lines first
                    std::vector<std::string> allLines = suzume::io::TextFileReader::readAllLines(options.getInputPath());

                    // Sample lines from all lines
                    std::vector<std::string> sampledLines = suzume::core::sampleLines(allLines, options.getSampleSize());

                    // Create a temporary file with sampled lines
                    std::string tempFilePath = std::filesystem::temp_directory_path().string() + "/suzume_sample_" +
                                              std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
                    suzume::io::TextFileWriter::writeLines(tempFilePath, sampledLines);

                    // Run normalize on sampled data
                    result = suzume::core::normalize(
                        tempFilePath,
                        options.getOutputPath(),
                        options.getNormalizeOptions()
                    );

                    // Remove temporary file
                    std::filesystem::remove(tempFilePath);

                    // Adjust result to indicate sampling
                    result.rows = sampledLines.size(); // Original count before normalization
                } else {
                    // Sample lines from input file
                    std::vector<std::string> sampledLines = suzume::core::sampleLines(
                        options.getInputPath(),
                        options.getSampleSize()
                    );

                    // Create a temporary file with sampled lines
                    std::string tempFilePath = std::filesystem::temp_directory_path().string() + "/suzume_sample_" +
                                              std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
                    suzume::io::TextFileWriter::writeLines(tempFilePath, sampledLines);

                    // Run normalize on sampled data
                    result = suzume::core::normalize(
                        tempFilePath,
                        options.getOutputPath(),
                        options.getNormalizeOptions()
                    );

                    // Remove temporary file
                    std::filesystem::remove(tempFilePath);

                    // Adjust result to indicate sampling
                    result.rows = sampledLines.size(); // Original count before normalization
                }
            } else {
                // Run normalize normally
                result = suzume::core::normalize(
                    options.getInputPath(),
                    options.getOutputPath(),
                    options.getNormalizeOptions()
                );
            }

            // Output results
            if (options.isStatsJsonEnabled()) {
                // Output statistics as JSON
                json stats = {
                    {"command", "normalize"},
                    {"input", options.getInputPath()},
                    {"output", options.getOutputPath()},
                    {"sampled", options.getSampleSize() > 0},
                    {"sample_size", options.getSampleSize()},
                    {"rows", result.rows},
                    {"uniques", result.uniques},
                    {"duplicates", result.duplicates},
                    {"elapsed_ms", result.elapsedMs},
                    {"mb_per_sec", result.mbPerSec}
                };
                std::cout << stats.dump() << std::endl;
            } else if (options.getNormalizeOptions().progressCallback) {
                // Print result if progress callback is enabled
                if (options.getSampleSize() > 0) {
                    std::cout << "Sampled " << options.getSampleSize() << " lines, processed "
                            << result.rows << " rows, " << result.uniques << " unique" << std::endl;
                } else {
                    std::cout << "Processed " << result.rows << " rows, "
                            << result.uniques << " unique" << std::endl;
                }
            }

            return 0;
        } else if (options.isPmiCommand()) {
            // Run PMI calculation
            suzume::PmiResult result = suzume::core::calculatePmi(
                options.getInputPath(),
                options.getOutputPath(),
                options.getPmiOptions()
            );

            if (options.isStatsJsonEnabled()) {
                // Output statistics as JSON
                json stats = {
                    {"command", "pmi"},
                    {"input", options.getInputPath()},
                    {"output", options.getOutputPath()},
                    {"n", options.getPmiOptions().n},
                    {"grams", result.grams},
                    {"distinct_ngrams", result.distinctNgrams},
                    {"elapsed_ms", result.elapsedMs},
                    {"mb_per_sec", result.mbPerSec}
                };
                std::cout << stats.dump() << std::endl;
            } else if (options.getPmiOptions().progressCallback) {
                // Print result if progress callback is enabled
                std::cout << "Processed " << result.grams << " n-grams" << std::endl;
            }

            return 0;
        } else if (options.isWordExtractCommand()) {
            // Run word extraction
            suzume::WordExtractionResult result = suzume::core::extractWords(
                options.getInputPath(),
                options.getOriginalTextPath(),
                options.getWordExtractionOptions()
            );

            if (options.isStatsJsonEnabled()) {
                // Output statistics as JSON
                json stats = {
                    {"command", "word-extract"},
                    {"pmi_input", options.getInputPath()},
                    {"original_text", options.getOriginalTextPath()},
                    {"output", options.getOutputPath()},
                    {"words_count", result.words.size()},
                    {"processing_time_ms", result.processingTimeMs},
                    {"memory_usage_bytes", result.memoryUsageBytes}
                };
                std::cout << stats.dump() << std::endl;
            } else if (options.getWordExtractionOptions().progressCallback) {
                // Print result if progress callback is enabled
                std::cout << "Extracted " << result.words.size() << " unknown words" << std::endl;
            }

            return 0;
        } else {
            // This should not happen due to CLI11's requirement for a subcommand
            std::cerr << "Error: No command selected" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
