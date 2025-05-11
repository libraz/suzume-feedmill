#include <suzume_feedmill.h>
#include <iostream>
#include <iomanip>

/**
 * Basic normalization example
 *
 * This example demonstrates:
 * 1. Basic text normalization
 * 2. Using min/max length filters
 */
int main(int argc, char* argv[]) {
    // Check arguments
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = argv[2];

    try {
        // Basic normalization
        std::cout << "Performing basic normalization..." << std::endl;

        suzume::NormalizeOptions normOpt;
        normOpt.form = suzume::NormalizationForm::NFKC;
        normOpt.threads = 4;
        normOpt.progressFormat = suzume::ProgressFormat::TTY;
        normOpt.progressCallback = [](double ratio) {
            std::cout << "\rNormalization progress: " << std::fixed << std::setprecision(1)
                      << (ratio * 100.0) << "%" << std::flush;
        };

        auto result = suzume::normalize(inputFile, outputFile, normOpt);

        std::cout << "\nNormalization complete!" << std::endl;
        std::cout << "Processed " << result.rows << " rows, "
                  << result.uniques << " unique rows, "
                  << result.duplicates << " duplicates removed" << std::endl;
        std::cout << "Processing speed: " << result.mbPerSec << " MB/s" << std::endl;

        // Normalization with line length filters
        std::string filteredOutput = outputFile + ".filtered";
        std::cout << "\nPerforming normalization with line length filters..." << std::endl;

        suzume::NormalizeOptions filterOpt;
        filterOpt.form = suzume::NormalizationForm::NFKC;
        filterOpt.threads = 4;
        filterOpt.minLength = 10;  // Filter out lines shorter than 10 characters
        filterOpt.maxLength = 200; // Filter out lines longer than 200 characters
        filterOpt.progressFormat = suzume::ProgressFormat::TTY;
        filterOpt.progressCallback = [](double ratio) {
            std::cout << "\rFiltering progress: " << std::fixed << std::setprecision(1)
                      << (ratio * 100.0) << "%" << std::flush;
        };

        auto filteredResult = suzume::normalize(inputFile, filteredOutput, filterOpt);

        std::cout << "\nFiltering complete!" << std::endl;
        std::cout << "Processed " << filteredResult.rows << " rows, "
                  << filteredResult.uniques << " unique rows, "
                  << filteredResult.duplicates << " duplicates removed" << std::endl;
        std::cout << "Lines filtered by length: "
                  << (result.rows - filteredResult.rows) << std::endl;
        std::cout << "Processing speed: " << filteredResult.mbPerSec << " MB/s" << std::endl;

        std::cout << "\nResults saved to:" << std::endl;
        std::cout << "  - Basic normalization: " << outputFile << std::endl;
        std::cout << "  - Filtered normalization: " << filteredOutput << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
