#include <suzume_feedmill.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>

/**
 * Simple example of using suzume-feedmill for word extraction
 *
 * This example demonstrates:
 * 1. Normalizing text data
 * 2. Calculating PMI for character n-grams
 * 3. Extracting potential unknown words
 */
int main(int argc, char* argv[]) {
    // Check arguments
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_text_file> [output_directory]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputDir = (argc > 2) ? argv[2] : "./output";

    // Create output directory if it doesn't exist
    std::string mkdirCmd = "mkdir -p " + outputDir;
    system(mkdirCmd.c_str());

    // Define output files
    std::string normalizedFile = outputDir + "/simple-normalized.tsv";
    std::string pmiFile = outputDir + "/simple-pmi.tsv";
    std::string wordsFile = outputDir + "/simple-words.tsv";

    try {
        // Step 1: Normalize text
        std::cout << "Normalizing text..." << std::endl;
        suzume::NormalizeOptions normOpt;
        normOpt.form = suzume::NormalizationForm::NFKC;
        normOpt.threads = 4;
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

        // Step 2: Calculate PMI
        std::cout << "\nCalculating PMI..." << std::endl;
        suzume::PmiOptions pmiOpt;
        pmiOpt.n = 2;  // bigrams
        pmiOpt.topK = 2500;
        pmiOpt.minFreq = 3;
        pmiOpt.threads = 4;
        pmiOpt.progressFormat = suzume::ProgressFormat::TTY;
        pmiOpt.progressCallback = [](double ratio) {
            std::cout << "\rPMI calculation progress: " << std::fixed << std::setprecision(1)
                      << (ratio * 100.0) << "%" << std::flush;
        };

        auto pmiResult = suzume::calculatePmi(normalizedFile, pmiFile, pmiOpt);

        std::cout << "\nPMI calculation complete!" << std::endl;
        std::cout << "Processed " << pmiResult.grams << " n-grams, "
                  << pmiResult.distinctNgrams << " distinct n-grams" << std::endl;
        std::cout << "Processing speed: " << pmiResult.mbPerSec << " MB/s" << std::endl;

        // Step 3: Extract words
        std::cout << "\nExtracting words..." << std::endl;
        suzume::WordExtractionOptions wordOpt;
        wordOpt.minPmiScore = 3.0;
        wordOpt.minLength = 2;
        wordOpt.maxLength = 10;
        wordOpt.topK = 100;
        wordOpt.verifyInOriginalText = true;
        wordOpt.threads = 4;
        wordOpt.progressFormat = suzume::ProgressFormat::TTY;
        wordOpt.progressCallback = [](double ratio) {
            std::cout << "\rWord extraction progress: " << std::fixed << std::setprecision(1)
                      << (ratio * 100.0) << "%" << std::flush;
        };

        auto wordResult = suzume::extractWords(pmiFile, normalizedFile, wordsFile, wordOpt);

        std::cout << "\nWord extraction complete!" << std::endl;
        std::cout << "Extracted " << wordResult.words.size() << " potential words" << std::endl;
        std::cout << "Processing time: " << wordResult.processingTimeMs << " ms" << std::endl;

        // Display top 10 words
        std::cout << "\nTop 10 extracted words:" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;
        std::cout << std::left << std::setw(20) << "Word"
                  << std::setw(10) << "Score"
                  << std::setw(10) << "Frequency"
                  << "Verified" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;

        size_t displayCount = std::min(size_t(10), wordResult.words.size());
        for (size_t i = 0; i < displayCount; i++) {
            std::cout << std::left << std::setw(20) << wordResult.words[i]
                      << std::setw(10) << std::fixed << std::setprecision(2) << wordResult.scores[i]
                      << std::setw(10) << wordResult.frequencies[i]
                      << (wordResult.verified[i] ? "✓" : "") << std::endl;
        }

        std::cout << "\nResults saved to:" << std::endl;
        std::cout << "  - Normalized text: " << normalizedFile << std::endl;
        std::cout << "  - PMI results: " << pmiFile << std::endl;
        std::cout << "  - Extracted words: " << wordsFile << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
