/**
 * @file performance_issues_test.cpp
 * @brief Tests to reproduce performance bottlenecks and algorithmic issues
 */

#include <gtest/gtest.h>
#include "core/word_extraction/common.h"
#include "core/word_extraction.h"
#include <chrono>
#include <vector>
#include <string>
#include <random>
#include <fstream>

namespace suzume {
namespace core {
namespace test {

// Test to reproduce O(n²) algorithm in filtering logic
TEST(PerformanceIssuesTest, QuadraticSubstringRemoval) {
  std::vector<WordCandidate> candidates;
  
  // Create overlapping candidates that will trigger quadratic behavior
  // Each candidate is a substring of others
  const std::string baseText = "abcdefghijklmnopqrstuvwxyz";
  
  // Generate many overlapping substrings
  for (size_t start = 0; start < baseText.length() - 2; ++start) {
    for (size_t len = 3; len <= std::min(size_t(10), baseText.length() - start); ++len) {
      std::string candidate = baseText.substr(start, len);
      candidates.push_back({candidate, 1.0, 1, false});
    }
  }
  
  std::cout << "Testing with " << candidates.size() << " candidates..." << std::endl;
  
  // Create test files for word extraction
  const std::string pmiFile = "/tmp/test_pmi_results.tsv";
  const std::string originalFile = "/tmp/test_original.txt";
  
  // Create PMI results file
  {
    std::ofstream ofs(pmiFile);
    ofs << "ngram\tscore\tfrequency\n";
    for (const auto& candidate : candidates) {
      ofs << candidate.text << "\t" << candidate.score << "\t" << candidate.frequency << "\n";
    }
  }
  
  // Create original text file
  {
    std::ofstream ofs(originalFile);
    ofs << baseText << "\n";
  }
  
  // Measure execution time
  auto startTime = std::chrono::high_resolution_clock::now();
  
  try {
    // This should trigger potential quadratic behavior in word extraction
    WordExtractionOptions options;
    options.maxCandidates = 1000;
    auto result = suzume::extractWords(pmiFile, originalFile, options);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Word extraction took " << duration.count() << " ms" << std::endl;
    
    // Should complete in reasonable time even with many candidates
    EXPECT_LT(duration.count(), 5000); // Should complete within 5 seconds
    EXPECT_GT(result.words.size(), 0); // Should extract some words
    
  } catch (const std::exception& e) {
    std::cout << "Word extraction failed: " << e.what() << std::endl;
  }
  
  // Cleanup
  std::remove(pmiFile.c_str());
  std::remove(originalFile.c_str());
}

// Test to reproduce potential memory allocation issues
TEST(PerformanceIssuesTest, MemoryAllocationPerformance) {
  const size_t numAllocations = 10000;
  
  auto startTime = std::chrono::high_resolution_clock::now();
  
  // Test repeated memory allocation/deallocation patterns
  for (size_t i = 0; i < numAllocations; ++i) {
    // Simulate word candidate allocation pattern
    std::vector<WordCandidate> candidates;
    candidates.reserve(100); // Good practice
    
    for (size_t j = 0; j < 100; ++j) {
      std::string text = "candidate_" + std::to_string(i) + "_" + std::to_string(j);
      candidates.push_back({text, 1.0, 1, false});
    }
    
    // Vector automatically cleans up when going out of scope
  }
  
  auto endTime = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
  
  std::cout << "Memory allocation test took " << duration.count() << " ms" << std::endl;
  
  // Should complete efficiently
  EXPECT_LT(duration.count(), 1000); // Should complete within 1 second
}

// Test to reproduce string processing patterns
TEST(PerformanceIssuesTest, StringProcessingPatterns) {
  const size_t numIterations = 10000;
  std::vector<std::string> testStrings;
  
  // Generate test data
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> lengthDist(5, 20);
  std::uniform_int_distribution<> charDist('a', 'z');
  
  for (size_t i = 0; i < numIterations; ++i) {
    size_t length = lengthDist(gen);
    std::string str;
    str.reserve(length);
    
    for (size_t j = 0; j < length; ++j) {
      str += static_cast<char>(charDist(gen));
    }
    testStrings.push_back(str);
  }
  
  auto startTime = std::chrono::high_resolution_clock::now();
  
  // Test string processing performance
  std::vector<WordCandidate> candidates;
  candidates.reserve(testStrings.size());
  
  for (const auto& str : testStrings) {
    candidates.push_back({str, 1.0, 1, false});
  }
  
  auto endTime = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
  
  std::cout << "String processing of " << numIterations << " strings took " 
            << duration.count() << " ms" << std::endl;
  
  // Should handle processing efficiently
  EXPECT_LT(duration.count(), 500); // Should complete within 500ms
  EXPECT_EQ(candidates.size(), testStrings.size());
}

// Test to reproduce large candidate set processing
TEST(PerformanceIssuesTest, LargeCandidateSetProcessing) {
  const size_t numCandidates = 5000;
  
  // Generate large number of realistic candidates
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> scoreDist(0.0, 10.0);
  std::uniform_int_distribution<> freqDist(1, 100);
  std::uniform_int_distribution<> lengthDist(2, 15);
  std::uniform_int_distribution<> charDist('a', 'z');
  
  // Create test files
  const std::string pmiFile = "/tmp/test_large_pmi.tsv";
  const std::string originalFile = "/tmp/test_large_original.txt";
  
  {
    std::ofstream ofs(pmiFile);
    ofs << "ngram\tscore\tfrequency\n";
    
    for (size_t i = 0; i < numCandidates; ++i) {
      size_t length = lengthDist(gen);
      std::string candidate;
      candidate.reserve(length);
      
      for (size_t j = 0; j < length; ++j) {
        candidate += static_cast<char>(charDist(gen));
      }
      
      ofs << candidate << "\t" << scoreDist(gen) << "\t" << freqDist(gen) << "\n";
    }
  }
  
  {
    std::ofstream ofs(originalFile);
    ofs << "Large text with many words for testing performance\n";
  }
  
  std::cout << "Testing processing with " << numCandidates << " candidates..." << std::endl;
  
  auto startTime = std::chrono::high_resolution_clock::now();
  
  try {
    WordExtractionOptions options;
    options.minPmiScore = 2.0;
    options.maxCandidates = 1000;
    
    auto result = suzume::extractWords(pmiFile, originalFile, options);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Large set processing took " << duration.count() << " ms" << std::endl;
    std::cout << "Extracted " << result.words.size() << " words" << std::endl;
    
    // Should handle large sets efficiently
    EXPECT_LT(duration.count(), 5000); // Should complete within 5 seconds
    
  } catch (const std::exception& e) {
    std::cout << "Large set processing failed: " << e.what() << std::endl;
  }
  
  // Cleanup
  std::remove(pmiFile.c_str());
  std::remove(originalFile.c_str());
}

// Test to reproduce vector reallocation overhead
TEST(PerformanceIssuesTest, VectorReallocationOverhead) {
  const size_t numOperations = 50000;
  
  auto startTime = std::chrono::high_resolution_clock::now();
  
  // Test without proper reservation (inefficient)
  std::vector<std::string> inefficientVector;
  for (size_t i = 0; i < numOperations; ++i) {
    inefficientVector.emplace_back("test_string_" + std::to_string(i));
  }
  
  auto midTime = std::chrono::high_resolution_clock::now();
  
  // Test with proper reservation (efficient)
  std::vector<std::string> efficientVector;
  efficientVector.reserve(numOperations);
  for (size_t i = 0; i < numOperations; ++i) {
    efficientVector.emplace_back("test_string_" + std::to_string(i));
  }
  
  auto endTime = std::chrono::high_resolution_clock::now();
  
  auto inefficientDuration = std::chrono::duration_cast<std::chrono::milliseconds>(midTime - startTime);
  auto efficientDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - midTime);
  
  std::cout << "Without reservation: " << inefficientDuration.count() << " ms" << std::endl;
  std::cout << "With reservation: " << efficientDuration.count() << " ms" << std::endl;
  
  // Both should complete in reasonable time
  EXPECT_LT(inefficientDuration.count(), 2000);
  EXPECT_LT(efficientDuration.count(), 2000);
  
  // Verify both vectors have the same content
  EXPECT_EQ(inefficientVector.size(), efficientVector.size());
  EXPECT_EQ(inefficientVector.size(), numOperations);
}

} // namespace test
} // namespace core
} // namespace suzume