/**
 * @file main.cpp
 * @brief Command-line interface for suzume-feedmill
 */

#include <iostream>
#include <string>
#include <stdexcept>
#include "options.h"
#include "core/normalize.h"
#include "core/pmi.h"
#include "core/word_extraction.h"

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
            // Run normalize
            suzume::NormalizeResult result = suzume::core::normalize(
                options.getInputPath(),
                options.getOutputPath(),
                options.getNormalizeOptions()
            );

            // Print result if progress callback is enabled
            if (options.getNormalizeOptions().progressCallback) {
                std::cout << "Processed " << result.rows << " rows, "
                          << result.uniques << " unique" << std::endl;
            }

            return 0;
        } else if (options.isPmiCommand()) {
            // Run PMI calculation
            suzume::PmiResult result = suzume::core::calculatePmi(
                options.getInputPath(),
                options.getOutputPath(),
                options.getPmiOptions()
            );

            // Print result if progress callback is enabled
            if (options.getPmiOptions().progressCallback) {
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

            // Print result if progress callback is enabled
            if (options.getWordExtractionOptions().progressCallback) {
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
