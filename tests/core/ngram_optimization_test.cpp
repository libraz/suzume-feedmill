/**
 * @file ngram_optimization_test.cpp
 * @brief Tests for N-gram processing optimizations
 */

#include <gtest/gtest.h>
#include "core/ngram_cache.h"
#include "core/streaming_processor.h"
#include "core/pmi.h"
#include <chrono>
#include <vector>
#include <string>
#include <fstream>
#include <random>
#include <thread>

namespace suzume {
namespace core {
namespace test {

// Test to verify N-gram cache functionality
TEST(NGramOptimizationTest, NGramCacheBasicFunctionality) {
    NGramCache cache(100, 5); // 100 entries, 5 minutes TTL
    
    // Test cache miss
    auto result = cache.get("test_ngram");
    EXPECT_FALSE(result.has_value());
    
    // Test cache put and hit
    cache.put("test_ngram", 2.5, 10);
    result = cache.get("test_ngram");
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(2.5, result->score);
    EXPECT_EQ(10u, result->frequency);
    
    // Test statistics
    auto [hits, misses, size] = cache.getStats();
    EXPECT_EQ(1u, hits);
    EXPECT_EQ(1u, misses); // Initial miss
    EXPECT_EQ(1u, size);
    
    // Test hit rate
    double hitRate = cache.getHitRate();
    EXPECT_DOUBLE_EQ(0.5, hitRate); // 1 hit out of 2 accesses
}

// Test to verify cache performance improvements
TEST(NGramOptimizationTest, CachePerformanceImprovement) {
    const size_t numOperations = 1000;
    const size_t numUniqueNgrams = 100;
    
    NGramCache cache(numUniqueNgrams * 2, 10);
    
    // Generate test data
    std::vector<std::string> ngrams;
    for (size_t i = 0; i < numUniqueNgrams; ++i) {
        ngrams.push_back("ngram_" + std::to_string(i));
    }
    
    // Populate cache
    for (size_t i = 0; i < numUniqueNgrams; ++i) {
        cache.put(ngrams[i], static_cast<double>(i), static_cast<uint32_t>(i));
    }
    
    // Test cache performance with repeated access
    auto startTime = std::chrono::high_resolution_clock::now();
    
    size_t hitCount = 0;
    for (size_t i = 0; i < numOperations; ++i) {
        size_t index = i % numUniqueNgrams;
        auto result = cache.get(ngrams[index]);
        if (result.has_value()) {
            hitCount++;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    std::cout << "Cache lookup time for " << numOperations << " operations: " 
              << duration.count() << " μs" << std::endl;
    std::cout << "Hit rate: " << (static_cast<double>(hitCount) / numOperations * 100) << "%" << std::endl;
    
    // Should have very high hit rate for repeated accesses
    EXPECT_GT(hitCount, numOperations * 0.9); // At least 90% hit rate
    EXPECT_LT(duration.count(), 10000); // Should complete within 10ms
}

// Test to verify streaming processor basic functionality
TEST(NGramOptimizationTest, StreamingProcessorBasicTest) {
    const std::string testFile = "/tmp/streaming_test_input.txt";
    const std::string outputFile = "/tmp/streaming_test_output.txt";
    
    // Create test file
    {
        std::ofstream ofs(testFile);
        for (int i = 0; i < 1000; ++i) {
            ofs << "Line " << i << " with some test content" << std::endl;
        }
    }
    
    StreamingLineProcessor processor;
    
    // Simple line processor that adds prefix
    auto lineProcessor = [](const std::string& line) -> std::string {
        return "Processed: " + line;
    };
    
    bool progressCalled = false;
    auto progressCallback = [&progressCalled](double progress) {
        progressCalled = true;
        EXPECT_GE(progress, 0.0);
        EXPECT_LE(progress, 1.0);
    };
    
    size_t linesProcessed = processor.processFile(testFile, outputFile, lineProcessor, progressCallback);
    
    EXPECT_EQ(1000u, linesProcessed);
    EXPECT_TRUE(progressCalled);
    
    // Verify output
    std::ifstream ifs(outputFile);
    EXPECT_TRUE(ifs.good());
    
    std::string line;
    size_t lineCount = 0;
    while (std::getline(ifs, line)) {
        EXPECT_TRUE(line.find("Processed:") == 0);
        lineCount++;
    }
    EXPECT_EQ(1000u, lineCount);
    
    // Get statistics
    auto [bytesRead, bytesWritten, timeMs] = processor.getStats();
    EXPECT_GT(bytesRead, 0u);
    EXPECT_GT(bytesWritten, 0u);
    EXPECT_GT(timeMs, 0u);
    
    std::cout << "Streaming processing stats:" << std::endl;
    std::cout << "  Bytes read: " << bytesRead << std::endl;
    std::cout << "  Bytes written: " << bytesWritten << std::endl;
    std::cout << "  Time: " << timeMs << " ms" << std::endl;
    
    // Cleanup
    std::remove(testFile.c_str());
    std::remove(outputFile.c_str());
}

// Test to verify batch processing performance
TEST(NGramOptimizationTest, BatchProcessingPerformance) {
    const std::string testFile = "/tmp/batch_test_input.txt";
    const std::string outputFile = "/tmp/batch_test_output.txt";
    const size_t numLines = 10000;
    
    // Create larger test file
    {
        std::ofstream ofs(testFile);
        for (size_t i = 0; i < numLines; ++i) {
            ofs << "Test line " << i << " with varying content length: " 
                << std::string(i % 50, 'x') << std::endl;
        }
    }
    
    StreamingConfig config;
    config.batchSize = 500; // Process in larger batches
    config.bufferSize = 128 * 1024; // 128KB buffer
    
    StreamingLineProcessor processor(config);
    
    // Batch processor that filters and transforms lines
    auto batchProcessor = [](const std::vector<std::string>& batch) -> std::vector<std::string> {
        std::vector<std::string> result;
        result.reserve(batch.size());
        
        for (const auto& line : batch) {
            // Only keep lines with even numbers
            if (line.find(" 0 ") != std::string::npos ||
                line.find(" 2 ") != std::string::npos ||
                line.find(" 4 ") != std::string::npos ||
                line.find(" 6 ") != std::string::npos ||
                line.find(" 8 ") != std::string::npos) {
                result.push_back("Filtered: " + line);
            }
        }
        
        return result;
    };
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    size_t linesProcessed = processor.processBatch(testFile, outputFile, batchProcessor);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_EQ(numLines, linesProcessed);
    
    // Verify some output exists (filtered lines)
    std::ifstream ifs(outputFile);
    EXPECT_TRUE(ifs.good());
    
    size_t outputLines = 0;
    std::string line;
    while (std::getline(ifs, line)) {
        EXPECT_TRUE(line.find("Filtered:") == 0);
        outputLines++;
    }
    
    EXPECT_GT(outputLines, 0u);
    EXPECT_LT(outputLines, numLines); // Should be filtered
    
    std::cout << "Batch processing performance:" << std::endl;
    std::cout << "  Input lines: " << linesProcessed << std::endl;
    std::cout << "  Output lines: " << outputLines << std::endl;
    std::cout << "  Processing time: " << duration.count() << " ms" << std::endl;
    std::cout << "  Throughput: " << (linesProcessed * 1000.0 / duration.count()) << " lines/sec" << std::endl;
    
    // Performance expectation: should process at least 1000 lines/sec
    EXPECT_GT(linesProcessed * 1000.0 / duration.count(), 1000.0);
    
    // Cleanup
    std::remove(testFile.c_str());
    std::remove(outputFile.c_str());
}

// Test to verify parallel streaming performance
TEST(NGramOptimizationTest, ParallelStreamingPerformance) {
    const std::string testFile = "/tmp/parallel_test_input.txt";
    const std::string outputFile = "/tmp/parallel_test_output.txt";
    const size_t numLines = 5000;
    
    // Create test file
    {
        std::ofstream ofs(testFile);
        for (size_t i = 0; i < numLines; ++i) {
            ofs << "Parallel test line " << i << " with content for processing" << std::endl;
        }
    }
    
    StreamingConfig config;
    config.batchSize = 200;
    config.bufferSize = 64 * 1024;
    
    ParallelStreamProcessor processor(4, config); // Use 4 threads
    
    // Simulated CPU-intensive batch processor
    auto batchProcessor = [](const std::vector<std::string>& batch) -> std::vector<std::string> {
        std::vector<std::string> result;
        result.reserve(batch.size());
        
        for (const auto& line : batch) {
            // Simulate some processing work
            std::string processed = line;
            for (int i = 0; i < 10; ++i) {
                processed = "P" + processed;
            }
            result.push_back(processed);
        }
        
        return result;
    };
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    size_t linesProcessed = processor.processFile(testFile, outputFile, batchProcessor);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_EQ(numLines, linesProcessed);
    
    // Verify output
    std::ifstream ifs(outputFile);
    EXPECT_TRUE(ifs.good());
    
    size_t outputLines = 0;
    std::string line;
    while (std::getline(ifs, line)) {
        // Should have prefix from processing
        EXPECT_TRUE(line.find("PPPPPPPPPP") == 0);
        outputLines++;
    }
    EXPECT_EQ(numLines, outputLines);
    
    auto [totalLines, processingTime, threadsUsed] = processor.getStats();
    
    std::cout << "Parallel streaming performance:" << std::endl;
    std::cout << "  Lines processed: " << totalLines << std::endl;
    std::cout << "  Processing time: " << processingTime << " ms" << std::endl;
    std::cout << "  Threads used: " << threadsUsed << std::endl;
    std::cout << "  Throughput: " << (totalLines * 1000.0 / processingTime) << " lines/sec" << std::endl;
    
    EXPECT_EQ(numLines, totalLines);
    EXPECT_EQ(4u, threadsUsed);
    
    // Should be reasonably fast with parallel processing
    EXPECT_LT(duration.count(), 5000); // Should complete within 5 seconds
    
    // Cleanup
    std::remove(testFile.c_str());
    std::remove(outputFile.c_str());
}

// Test to verify cache cleanup functionality
TEST(NGramOptimizationTest, CacheCleanupTest) {
    NGramCache cache(10, 1); // 10 entries, 1 minute TTL
    
    // Add some entries
    for (int i = 0; i < 5; ++i) {
        cache.put("ngram_" + std::to_string(i), static_cast<double>(i), static_cast<uint32_t>(i));
    }
    
    auto [hits, misses, size] = cache.getStats();
    EXPECT_EQ(5u, size);
    
    // Test that entries exist and can be retrieved
    for (int i = 0; i < 5; ++i) {
        auto result = cache.get("ngram_" + std::to_string(i));
        EXPECT_TRUE(result.has_value());
    }
    
    // Cleanup expired entries (should not remove anything since TTL is 1 minute)
    size_t removedCount = cache.cleanupExpired();
    EXPECT_EQ(0u, removedCount);
    
    // Verify cache still has entries
    auto [hits2, misses2, size2] = cache.getStats();
    EXPECT_EQ(5u, size2);
}

} // namespace test
} // namespace core
} // namespace suzume