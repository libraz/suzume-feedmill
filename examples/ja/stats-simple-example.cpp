#include <suzume_feedmill.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

/**
 * 簡単な統計出力例
 *
 * この例では以下を実演します：
 * 1. 基本的な統計情報の収集
 * 2. パフォーマンス指標の出力
 */
int main(int argc, char* argv[]) {
    // Check arguments
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputDir = "stats_example_output";
    std::string normalizedFile = outputDir + "/normalized.tsv";
    std::string pmiFile = outputDir + "/ngrams.tsv";

    try {
        // Create output directory
        // Note: In a real application, you would use std::filesystem or similar
        system(("mkdir -p " + outputDir).c_str());

        // Step 1: Normalize text and collect statistics
        std::cout << "Normalizing text and collecting statistics..." << std::endl;

        suzume::NormalizeOptions normOpt;
        normOpt.form = suzume::NormalizationForm::NFKC;
        normOpt.threads = 4;

        auto normResult = suzume::normalize(inputFile, normalizedFile, normOpt);

        std::cout << "Normalization Results:" << std::endl;
        std::cout << "  Rows processed: " << normResult.rows << std::endl;
        std::cout << "  Unique rows: " << normResult.uniques << std::endl;
        std::cout << "  Duplicates removed: " << normResult.duplicates << std::endl;
        std::cout << "  Processing time: " << normResult.elapsedMs << " ms" << std::endl;
        std::cout << "  Speed: " << normResult.mbPerSec << " MB/sec" << std::endl;

        // Step 2: Calculate PMI and collect statistics
        std::cout << "\nCalculating PMI and collecting statistics..." << std::endl;

        suzume::PmiOptions pmiOpt;
        pmiOpt.n = 2;
        pmiOpt.topK = 100;
        pmiOpt.minFreq = 2;
        pmiOpt.threads = 4;

        auto pmiResult = suzume::calculatePmi(normalizedFile, pmiFile, pmiOpt);

        std::cout << "PMI Results:" << std::endl;
        std::cout << "  N-grams processed: " << pmiResult.grams << std::endl;
        std::cout << "  Distinct n-grams: " << pmiResult.distinctNgrams << std::endl;
        std::cout << "  Processing time: " << pmiResult.elapsedMs << " ms" << std::endl;
        std::cout << "  Speed: " << pmiResult.mbPerSec << " MB/sec" << std::endl;

        // Step 3: Display summary
        std::cout << "\nSummary:" << std::endl;
        std::cout << "  Total processing time: " << (normResult.elapsedMs + pmiResult.elapsedMs) << " ms" << std::endl;
        std::cout << "  Compression ratio: " << (double)normResult.uniques / normResult.rows << std::endl;
        std::cout << "  Output files created in: " << outputDir << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}