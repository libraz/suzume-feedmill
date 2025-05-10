/**
 * @file performance_test.cpp
 * @brief Performance and memory tests for core functionality
 *
 * These tests are designed to verify the performance and memory usage
 * of the core functionality. They may take longer to run than regular tests.
 */

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <random>
#include <string>
#include <vector>
#include "core/normalize.h"
#include "core/pmi.h"

namespace suzume {
namespace core {
namespace test {

class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory if it doesn't exist
        std::filesystem::create_directories("test_data");
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all("test_data");
    }

    // Helper to generate a large test file
    void generateLargeFile(const std::string& filename, size_t lineCount, size_t wordsPerLine = 10) {
        std::ofstream file(filename);

        // List of words to use
        std::vector<std::string> words = {
            "the", "quick", "brown", "fox", "jumps", "over", "lazy", "dog",
            "hello", "world", "test", "data", "performance", "memory", "usage",
            "suzume", "feedmill", "normalize", "pmi", "calculation", "n-gram",
            "processing", "text", "analysis", "natural", "language", "corpus",
            "token", "word", "sentence", "paragraph", "document", "collection"
        };

        // Random number generator
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> wordDist(0, words.size() - 1);

        // Generate lines
        for (size_t i = 0; i < lineCount; ++i) {
            for (size_t j = 0; j < wordsPerLine; ++j) {
                file << words[wordDist(gen)];
                if (j < wordsPerLine - 1) {
                    file << " ";
                }
            }
            file << "\n";
        }

        file.close();
    }

    // Helper to measure execution time
    template<typename Func>
    double measureExecutionTime(Func func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> diff = end - start;
        return diff.count();
    }
};

// Test normalize performance with a medium-sized file
TEST_F(PerformanceTest, DISABLED_NormalizePerformance) {
    // Generate a medium-sized file (10,000 lines)
    const size_t lineCount = 10000;
    const std::string inputFile = "test_data/normalize_perf_input.tsv";
    const std::string outputFile = "test_data/normalize_perf_output.tsv";

    generateLargeFile(inputFile, lineCount);

    // Set up options
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;
    options.threads = 4; // Use a fixed number of threads for consistent testing

    // Measure execution time
    double executionTime = measureExecutionTime([&]() {
        NormalizeResult result = core::normalize(inputFile, outputFile, options);

        // Verify results
        EXPECT_EQ(lineCount, result.rows);
        EXPECT_GT(result.uniques, 0);
        EXPECT_LE(result.uniques, lineCount);
    });

    // Output performance metrics
    std::cout << "Normalize performance test:" << std::endl;
    std::cout << "  Lines processed: " << lineCount << std::endl;
    std::cout << "  Execution time: " << executionTime << " seconds" << std::endl;
    std::cout << "  Lines per second: " << lineCount / executionTime << std::endl;

    // No hard assertions on performance, as it depends on the hardware
    // This test is mainly for monitoring performance over time
}

// Test PMI performance with a medium-sized file
TEST_F(PerformanceTest, DISABLED_PmiPerformance) {
    // Generate a medium-sized file (5,000 lines)
    const size_t lineCount = 5000;
    const std::string inputFile = "test_data/pmi_perf_input.txt";
    const std::string outputFile = "test_data/pmi_perf_output.tsv";

    generateLargeFile(inputFile, lineCount);

    // Set up options
    PmiOptions options;
    options.n = 2;
    options.topK = 1000;
    options.minFreq = 2;
    options.threads = 4; // Use a fixed number of threads for consistent testing

    // Measure execution time
    double executionTime = measureExecutionTime([&]() {
        PmiResult result = core::calculatePmi(inputFile, outputFile, options);

        // Verify results
        EXPECT_GT(result.grams, 0);
    });

    // Output performance metrics
    std::cout << "PMI performance test:" << std::endl;
    std::cout << "  Lines processed: " << lineCount << std::endl;
    std::cout << "  Execution time: " << executionTime << " seconds" << std::endl;
    std::cout << "  Lines per second: " << lineCount / executionTime << std::endl;

    // Check output file
    std::ifstream outputFileStream(outputFile);
    std::string line;
    std::vector<std::string> outputLines;

    while (std::getline(outputFileStream, line)) {
        outputLines.push_back(line);
    }

    // First line should be header
    EXPECT_EQ("ngram\tpmi\tfrequency", outputLines[0]);

    // Should have at most topK + 1 lines (header + data)
    EXPECT_LE(outputLines.size(), options.topK + 1);
}

// Test memory usage with repeated calls
TEST_F(PerformanceTest, DISABLED_MemoryUsage) {
    // Generate a small file for repeated processing
    const size_t lineCount = 1000;
    const std::string inputFile = "test_data/memory_test_input.tsv";
    const std::string outputFile = "test_data/memory_test_output.tsv";

    generateLargeFile(inputFile, lineCount);

    // Normalize options
    NormalizeOptions normalizeOptions;
    normalizeOptions.form = NormalizationForm::NFKC;

    // PMI options
    PmiOptions pmiOptions;
    pmiOptions.n = 2;
    pmiOptions.topK = 100;
    pmiOptions.minFreq = 2;

    // Perform multiple iterations to check for memory leaks
    const int iterations = 10;

    // Test normalize memory usage
    for (int i = 0; i < iterations; ++i) {
        NormalizeResult result = core::normalize(
            inputFile,
            outputFile + std::to_string(i),
            normalizeOptions
        );

        EXPECT_EQ(lineCount, result.rows);
        EXPECT_GT(result.uniques, 0);
    }

    // Test PMI memory usage
    for (int i = 0; i < iterations; ++i) {
        PmiResult result = core::calculatePmi(
            inputFile,
            outputFile + "_pmi_" + std::to_string(i),
            pmiOptions
        );

        EXPECT_GT(result.grams, 0);
    }

    // No assertions on memory usage, as it's hard to measure in a portable way
    // This test is mainly for manual verification with tools like valgrind
}

// Test scaling with thread count
TEST_F(PerformanceTest, DISABLED_ThreadScaling) {
    // Generate a medium-sized file
    const size_t lineCount = 5000;
    const std::string inputFile = "test_data/thread_scaling_input.tsv";
    const std::string outputFile = "test_data/thread_scaling_output.tsv";

    generateLargeFile(inputFile, lineCount);

    // Test with different thread counts
    std::vector<uint32_t> threadCounts = {1, 2, 4, 8};
    std::vector<double> executionTimes;

    for (uint32_t threads : threadCounts) {
        // Normalize options
        NormalizeOptions options;
        options.form = NormalizationForm::NFKC;
        options.threads = threads;

        // Measure execution time
        double executionTime = measureExecutionTime([&]() {
            NormalizeResult result = core::normalize(
                inputFile,
                outputFile + "_" + std::to_string(threads),
                options
            );

            EXPECT_EQ(lineCount, result.rows);
            EXPECT_GT(result.uniques, 0);
        });

        executionTimes.push_back(executionTime);

        std::cout << "Threads: " << threads << ", Time: " << executionTime << " seconds" << std::endl;
    }

    // Check that execution time decreases with more threads
    // This is not always true due to overhead, so we don't assert it
    // Just output the results for manual verification
    std::cout << "Thread scaling results:" << std::endl;
    for (size_t i = 0; i < threadCounts.size(); ++i) {
        std::cout << "  Threads: " << threadCounts[i]
                  << ", Time: " << executionTimes[i] << " seconds"
                  << ", Speedup: " << (i > 0 ? executionTimes[0] / executionTimes[i] : 1.0)
                  << std::endl;
    }
}

// Test with very large n-gram counts
TEST_F(PerformanceTest, DISABLED_LargeNgramCounts) {
    // Generate a file with repeated words to create many identical n-grams
    std::ofstream file("test_data/large_ngram_input.txt");

    // Create 1000 lines with the same 10 words repeated
    for (int i = 0; i < 1000; ++i) {
        file << "the quick brown fox jumps over the lazy dog again";
        file << "\n";
    }

    file.close();

    // Set up options
    PmiOptions options;
    options.n = 2;
    options.topK = 100;
    options.minFreq = 1; // Include all n-grams

    // Measure execution time
    double executionTime = measureExecutionTime([&]() {
        PmiResult result = core::calculatePmi(
            "test_data/large_ngram_input.txt",
            "test_data/large_ngram_output.tsv",
            options
        );

        // Verify results
        EXPECT_GT(result.grams, 0);
    });

    // Output performance metrics
    std::cout << "Large n-gram counts test:" << std::endl;
    std::cout << "  Execution time: " << executionTime << " seconds" << std::endl;

    // Check output file
    std::ifstream outputFile("test_data/large_ngram_output.tsv");
    std::string line;
    std::vector<std::string> outputLines;

    while (std::getline(outputFile, line)) {
        outputLines.push_back(line);
    }

    // First line should be header
    EXPECT_EQ("ngram\tpmi\tfrequency", outputLines[0]);

    // Should have some n-grams with high frequency
    bool hasHighFrequency = false;

    for (size_t i = 1; i < outputLines.size(); ++i) {
        std::string line = outputLines[i];
        size_t lastTabPos = line.rfind('\t');

        if (lastTabPos != std::string::npos) {
            std::string freqStr = line.substr(lastTabPos + 1);
            int freq = std::stoi(freqStr);

            if (freq > 900) { // Should be close to 1000
                hasHighFrequency = true;
                break;
            }
        }
    }

    EXPECT_TRUE(hasHighFrequency);
}

} // namespace test
} // namespace core
} // namespace suzume
