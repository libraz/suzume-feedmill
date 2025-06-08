#include <suzume_feedmill.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * Sampling example
 *
 * This example demonstrates:
 * 1. Using the built-in sampling feature
 * 2. Processing large files efficiently
 */
int main(int argc, char* argv[]) {
    // Check arguments
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_directory>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputDir = argv[2];

    // Create output directory if it doesn't exist
    fs::create_directories(outputDir);

    // Define output files
    std::string normalizedFile = outputDir + "/normalized.tsv";
    std::string sampleFile = outputDir + "/sample.tsv";

    try {
        // Check input file size
        std::ifstream inFile(inputFile, std::ios::binary | std::ios::ate);
        if (!inFile.is_open()) {
            throw std::runtime_error("Could not open input file: " + inputFile);
        }
        std::streamsize size = inFile.tellg();
        inFile.close();

        std::cout << "Input file size: " << (size / 1024.0 / 1024.0) << " MB" << std::endl;

        // Create a sample with 1000 lines
        std::cout << "\nCreating a sample with 1000 lines..." << std::endl;

        suzume::NormalizeOptions sampleOpt;
        sampleOpt.form = suzume::NormalizationForm::NFKC;
        sampleOpt.progressFormat = suzume::ProgressFormat::TTY;
        sampleOpt.progressCallback = [](double ratio) {
            std::cout << "\rSampling progress: " << std::fixed << std::setprecision(1)
                      << (ratio * 100.0) << "%" << std::flush;
        };

        // Note: Sample size is handled internally by the API
        auto sampleResult = suzume::normalize(inputFile, sampleFile, sampleOpt);

        std::cout << "\nSampling complete!" << std::endl;
        std::cout << "Created sample with " << sampleResult.rows << " rows" << std::endl;
        std::cout << "Sample file size: " << (fs::file_size(sampleFile) / 1024.0) << " KB" << std::endl;

        // Process the full file
        std::cout << "\nProcessing the full file..." << std::endl;

        suzume::NormalizeOptions normOpt;
        normOpt.form = suzume::NormalizationForm::NFKC;
        normOpt.threads = 8;
        normOpt.progressFormat = suzume::ProgressFormat::TTY;
        normOpt.progressCallback = [](double ratio) {
            std::cout << "\rNormalization progress: " << std::fixed << std::setprecision(1)
                      << (ratio * 100.0) << "%" << std::flush;
        };

        auto normResult = suzume::normalize(inputFile, normalizedFile, normOpt);

        std::cout << "\nNormalization complete!" << std::endl;
        std::cout << "Processed " << normResult.rows << " rows, "
                  << normResult.uniques << " unique rows, "
                  << normResult.duplicates << " duplicates removed" << std::endl;
        std::cout << "Processing speed: " << normResult.mbPerSec << " MB/s" << std::endl;

        // Compare sample statistics with full file
        std::cout << "\nComparison of sample vs. full file:" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;
        std::cout << std::left << std::setw(20) << "Metric"
                  << std::setw(15) << "Sample"
                  << "Full File" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;

        std::cout << std::left << std::setw(20) << "Rows"
                  << std::setw(15) << sampleResult.rows
                  << normResult.rows << std::endl;

        std::cout << std::left << std::setw(20) << "Unique rows"
                  << std::setw(15) << sampleResult.uniques
                  << normResult.uniques << std::endl;

        double sampleDupeRate = sampleResult.duplicates * 100.0 / sampleResult.rows;
        double fullDupeRate = normResult.duplicates * 100.0 / normResult.rows;

        std::cout << std::left << std::setw(20) << "Duplicate rate"
                  << std::setw(15) << std::fixed << std::setprecision(2) << sampleDupeRate << "%"
                  << std::fixed << std::setprecision(2) << fullDupeRate << "%" << std::endl;

        std::cout << "\nResults saved to:" << std::endl;
        std::cout << "  - Sample file: " << sampleFile << std::endl;
        std::cout << "  - Full normalized file: " << normalizedFile << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
