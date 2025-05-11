#include <suzume_feedmill.h>
#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * Statistics JSON output example
 *
 * This example demonstrates:
 * 1. Using the --stats-json option programmatically
 * 2. Parsing and analyzing JSON statistics
 * 3. Monitoring performance metrics
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
    std::filesystem::create_directories(outputDir);

    // Define output files
    std::string normalizedFile = outputDir + "/normalized.tsv";
    std::string pmiFile = outputDir + "/ngrams.tsv";
    std::string statsFile = outputDir + "/stats.json";

    try {
        // Step 1: Normalize text and collect statistics
        std::cout << "Normalizing text and collecting statistics..." << std::endl;

        suzume::NormalizeOptions normOpt;
        normOpt.form = suzume::NormalizationForm::NFKC;
        normOpt.threads = 4;
        // No progress callback to avoid cluttering the output

        auto normResult = suzume::normalize(inputFile, normalizedFile, normOpt);

        // Create JSON object with normalization statistics
        json normStats = {
            {"command", "normalize"},
            {"input", inputFile},
            {"output", normalizedFile},
            {"rows", normResult.rows},
            {"uniques", normResult.uniques},
            {"duplicates", normResult.duplicates},
            {"elapsed_ms", normResult.elapsedMs},
            {"mb_per_sec", normResult.mbPerSec}
        };

        std::cout << "Normalization statistics:" << std::endl;
        std::cout << normStats.dump(2) << std::endl;

        // Step 2: Calculate PMI and collect statistics
        std::cout << "\nCalculating PMI and collecting statistics..." << std::endl;

        suzume::PmiOptions pmiOpt;
        pmiOpt.n = 2;
        pmiOpt.topK = 1000;
        pmiOpt.minFreq = 3;
        pmiOpt.threads = 4;
        // No progress callback to avoid cluttering the output

        auto pmiResult = suzume::calculatePmi(normalizedFile, pmiFile, pmiOpt);

        // Create JSON object with PMI statistics
        json pmiStats = {
            {"command", "pmi"},
            {"input", normalizedFile},
            {"output", pmiFile},
            {"n", pmiOpt.n},
            {"grams", pmiResult.grams},
            {"distinct_ngrams", pmiResult.distinctNgrams},
            {"elapsed_ms", pmiResult.elapsedMs},
            {"mb_per_sec", pmiResult.mbPerSec}
        };

        std::cout << "PMI statistics:" << std::endl;
        std::cout << pmiStats.dump(2) << std::endl;

        // Step 3: Combine statistics and save to file
        json allStats = {
            {"normalize", normStats},
            {"pmi", pmiStats},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
            {"total_processing_time_ms", normResult.elapsedMs + pmiResult.elapsedMs}
        };

        // Write combined statistics to file
        std::ofstream statsOut(statsFile);
        if (!statsOut.is_open()) {
            throw std::runtime_error("Could not open stats file for writing: " + statsFile);
        }
        statsOut << allStats.dump(2) << std::endl;
        statsOut.close();

        // Step 4: Analyze statistics
        std::cout << "\nAnalyzing statistics..." << std::endl;

        // Calculate duplicate rate
        double duplicateRate = static_cast<double>(normResult.duplicates) / normResult.rows * 100.0;
        std::cout << "Duplicate rate: " << std::fixed << std::setprecision(2) << duplicateRate << "%" << std::endl;

        // Calculate average processing speed
        double avgSpeed = (normResult.mbPerSec + pmiResult.mbPerSec) / 2.0;
        std::cout << "Average processing speed: " << std::fixed << std::setprecision(2) << avgSpeed << " MB/s" << std::endl;

        // Calculate n-gram density
        double ngramDensity = static_cast<double>(pmiResult.distinctNgrams) / normResult.uniques;
        std::cout << "N-gram density: " << std::fixed << std::setprecision(2) << ngramDensity
                  << " distinct n-grams per unique line" << std::endl;

        std::cout << "\nResults saved to:" << std::endl;
        std::cout << "  - Normalized text: " << normalizedFile << std::endl;
        std::cout << "  - PMI results: " << pmiFile << std::endl;
        std::cout << "  - Statistics: " << statsFile << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
