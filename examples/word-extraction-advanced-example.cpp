#include <suzume_feedmill.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <map>
#include <set>
#include <chrono>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * Advanced example of using suzume-feedmill for word extraction
 *
 * This example demonstrates:
 * 1. Advanced text normalization with custom options
 * 2. PMI calculation with optimized parameters
 * 3. Sophisticated word extraction with context analysis
 * 4. Post-processing of extracted words
 * 5. Visualization and analysis of results
 */

// Helper function to read lines from a file
std::vector<std::string> readLines(const std::string& filename) {
    std::vector<std::string> lines;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    while (std::getline(file, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    return lines;
}

// Helper function to write lines to a file
void writeLines(const std::string& filename, const std::vector<std::string>& lines) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + filename);
    }

    for (const auto& line : lines) {
        file << line << std::endl;
    }
}

// Helper function to parse TSV file with extracted words
std::vector<std::tuple<std::string, double, int, bool>> parseWordsTsv(const std::string& filename) {
    std::vector<std::tuple<std::string, double, int, bool>> words;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::string word;
        double score = 0.0;
        int components = 0;
        bool verified = false;

        // Parse TSV line
        size_t pos1 = line.find('\t');
        if (pos1 != std::string::npos) {
            word = line.substr(0, pos1);

            size_t pos2 = line.find('\t', pos1 + 1);
            if (pos2 != std::string::npos) {
                score = std::stod(line.substr(pos1 + 1, pos2 - pos1 - 1));

                size_t pos3 = line.find('\t', pos2 + 1);
                if (pos3 != std::string::npos) {
                    components = std::stoi(line.substr(pos2 + 1, pos3 - pos2 - 1));
                    verified = line.substr(pos3 + 1) == "✓";
                }
            }
        }

        words.emplace_back(word, score, components, verified);
    }

    return words;
}

// Post-processing function to filter and categorize words
void postProcessWords(const std::string& inputFile, const std::string& outputFile) {
    auto words = parseWordsTsv(inputFile);

    // Sort by score
    std::sort(words.begin(), words.end(), [](const auto& a, const auto& b) {
        return std::get<1>(a) > std::get<1>(b);
    });

    // Categorize words by length
    std::map<int, std::vector<std::tuple<std::string, double, int, bool>>> wordsByLength;
    for (const auto& word : words) {
        int length = std::get<0>(word).length();
        wordsByLength[length].push_back(word);
    }

    // Write categorized results
    std::ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + outputFile);
    }

    outFile << "# Advanced Word Extraction Results" << std::endl;
    outFile << "# ---------------------------------" << std::endl;
    outFile << "# Total words extracted: " << words.size() << std::endl;
    outFile << std::endl;

    // Write summary by length
    outFile << "## Summary by Length" << std::endl;
    outFile << "| Length | Count | Avg Score | Examples |" << std::endl;
    outFile << "|--------|-------|-----------|----------|" << std::endl;

    for (const auto& [length, lengthWords] : wordsByLength) {
        // Calculate average score
        double totalScore = 0.0;
        for (const auto& word : lengthWords) {
            totalScore += std::get<1>(word);
        }
        double avgScore = totalScore / lengthWords.size();

        // Get up to 3 examples
        std::string examples;
        for (size_t i = 0; i < std::min(size_t(3), lengthWords.size()); i++) {
            if (i > 0) examples += ", ";
            examples += std::get<0>(lengthWords[i]);
        }

        outFile << "| " << length << " | " << lengthWords.size() << " | "
                << std::fixed << std::setprecision(2) << avgScore << " | "
                << examples << " |" << std::endl;
    }

    outFile << std::endl;

    // Write top words by score
    outFile << "## Top Words by Score" << std::endl;
    outFile << "| Word | Score | Components | Verified |" << std::endl;
    outFile << "|------|-------|------------|----------|" << std::endl;

    size_t topCount = std::min(size_t(50), words.size());
    for (size_t i = 0; i < topCount; i++) {
        const auto& [word, score, components, verified] = words[i];
        outFile << "| " << word << " | "
                << std::fixed << std::setprecision(2) << score << " | "
                << components << " | "
                << (verified ? "✓" : "") << " |" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Check arguments
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_text_file> [output_directory]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputDir = (argc > 2) ? argv[2] : "./output";

    // Create output directory if it doesn't exist
    fs::create_directories(outputDir);

    // Define output files
    std::string normalizedFile = outputDir + "/advanced-normalized.tsv";
    std::string pmiFile = outputDir + "/advanced-ngrams.tsv";
    std::string wordsFile = outputDir + "/advanced-words.tsv";
    std::string analysisFile = outputDir + "/advanced-analysis.md";
    std::string sampleFile = outputDir + "/advanced-sample.tsv";

    try {
        // Create a sample file if input is too large
        std::ifstream inFile(inputFile, std::ios::binary | std::ios::ate);
        std::streamsize size = inFile.tellg();
        inFile.close();

        if (size > 10 * 1024 * 1024) { // 10 MB
            std::cout << "Input file is large (" << (size / (1024 * 1024)) << " MB), creating a sample..." << std::endl;

            auto lines = readLines(inputFile);
            size_t sampleSize = std::min(size_t(10000), lines.size());

            // Randomly select lines
            std::vector<std::string> sampleLines;
            std::sample(lines.begin(), lines.end(), std::back_inserter(sampleLines),
                       sampleSize, std::mt19937{std::random_device{}()});

            writeLines(sampleFile, sampleLines);
            inputFile = sampleFile;
            std::cout << "Created sample with " << sampleLines.size() << " lines" << std::endl;
        }

        // Step 1: Normalize text with advanced options
        std::cout << "Normalizing text with advanced options..." << std::endl;
        suzume::NormalizeOptions normOpt;
        normOpt.form = suzume::NormalizationForm::NFKC;
        normOpt.bloomFalsePositiveRate = 0.0001; // Higher precision
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

        // Step 2: Calculate PMI with optimized parameters
        std::cout << "\nCalculating PMI with optimized parameters..." << std::endl;
        suzume::PmiOptions pmiOpt;
        pmiOpt.n = 2;  // bigrams
        pmiOpt.topK = 5000; // More candidates
        pmiOpt.minFreq = 2; // Lower threshold
        pmiOpt.threads = 8;
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

        // Step 3: Extract words with context analysis
        std::cout << "\nExtracting words with context analysis..." << std::endl;
        suzume::WordExtractionOptions wordOpt;
        wordOpt.minPmiScore = 2.5; // Lower threshold for more candidates
        wordOpt.minLength = 2;
        wordOpt.maxLength = 15; // Allow longer words
        wordOpt.topK = 500; // More results
        wordOpt.verifyInOriginalText = true;
        wordOpt.useContextualAnalysis = true; // Enable context analysis
        wordOpt.threads = 8;
        wordOpt.progressFormat = suzume::ProgressFormat::TTY;
        wordOpt.progressCallback = [](double ratio) {
            std::cout << "\rWord extraction progress: " << std::fixed << std::setprecision(1)
                      << (ratio * 100.0) << "%" << std::flush;
        };

        auto wordResult = suzume::extractWords(pmiFile, normalizedFile, wordsFile, wordOpt);

        std::cout << "\nWord extraction complete!" << std::endl;
        std::cout << "Extracted " << wordResult.words.size() << " potential words" << std::endl;
        std::cout << "Processing time: " << wordResult.processingTimeMs << " ms" << std::endl;

        // Step 4: Post-process and analyze results
        std::cout << "\nPost-processing and analyzing results..." << std::endl;
        postProcessWords(wordsFile, analysisFile);

        // Display top words
        std::cout << "\nTop extracted words:" << std::endl;
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
        std::cout << "  - Analysis report: " << analysisFile << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
