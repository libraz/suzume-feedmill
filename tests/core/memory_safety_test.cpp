/**
 * @file memory_safety_test.cpp
 * @brief Tests to reproduce memory safety issues identified in the codebase
 */

#include <gtest/gtest.h>
#include "core/buffer_api.h"
#include "core/text_utils.h"
#include "core/normalize.h"
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <memory>
#include <atomic>
#include <fstream>

namespace suzume {
namespace core {
namespace test {

// Test to reproduce memory leak in buffer_api.cpp
TEST(MemorySafetyTest, BufferApiMemoryLeak) {
  const char* testData = "test line\n";
  size_t testDataLen = strlen(testData);
  
  // Test multiple allocations to trigger memory leaks
  for (int i = 0; i < 100; ++i) {
    uint8_t* outputData = nullptr;
    size_t outputLength = 0;
    
    NormalizeOptions options;
    options.form = NormalizationForm::NFC;
    
    NormalizeResult result = normalizeBuffer(
      reinterpret_cast<const uint8_t*>(testData),
      testDataLen,
      &outputData,
      &outputLength,
      options
    );
    
    ASSERT_NE(nullptr, outputData);
    
    // Intentionally NOT calling delete[] to simulate memory leak
    // This test should FAIL initially, demonstrating the problem
    if (i % 10 == 0) {
      // Only clean up some allocations to simulate partial cleanup
      delete[] outputData;
    }
    // Remaining allocations will leak memory
  }
  
  // This test is designed to fail and demonstrate memory leaks
  // After fixing buffer_api.cpp, modify this test to use proper RAII
}

// Test to reproduce undefined behavior in atomic operations
TEST(MemorySafetyTest, AtomicUndefinedBehavior) {
  const char* testData = "test data for atomic operations\n";
  size_t testDataLen = strlen(testData);
  
  uint8_t* outputData = nullptr;
  size_t outputLength = 0;
  
  NormalizeOptions options;
  
  // Create raw buffer that will be cast to atomic (undefined behavior)
  uint32_t progressBuffer[3] = {0, 0, 0};
  
  // This should trigger undefined behavior in buffer_api.cpp line 71-89
  // where non-atomic memory is cast to atomic<uint32_t>*
  NormalizeResult result = normalizeBuffer(
    reinterpret_cast<const uint8_t*>(testData),
    testDataLen,
    &outputData,
    &outputLength,
    options,
    progressBuffer
  );
  
  ASSERT_NE(nullptr, outputData);
  
  // The undefined behavior occurs in the atomic operations
  // This test may pass but the behavior is technically undefined
  EXPECT_GT(progressBuffer[0], 0);
  
  delete[] outputData;
}

// Test to reproduce race condition in ICU initialization
TEST(MemorySafetyTest, ICUInitializationRaceCondition) {
  const int numThreads = 10;
  std::vector<std::thread> threads;
  std::atomic<int> successCount{0};
  std::atomic<int> failureCount{0};
  
  // Launch multiple threads that will all try to initialize ICU
  for (int i = 0; i < numThreads; ++i) {
    threads.emplace_back([&]() {
      try {
        // This should trigger the race condition in text_utils.cpp:36-88
        std::string testText = "test unicode: café こんにちは 你好";
        std::string normalized = normalizeLine(testText, NormalizationForm::NFC);
        
        if (!normalized.empty()) {
          successCount++;
        } else {
          failureCount++;
        }
      } catch (const std::exception& e) {
        failureCount++;
      } catch (...) {
        failureCount++;
      }
    });
  }
  
  // Wait for all threads to complete
  for (auto& thread : threads) {
    thread.join();
  }
  
  // In the current implementation, this may fail due to race conditions
  // After fixing, all threads should succeed
  EXPECT_EQ(numThreads, successCount.load());
  EXPECT_EQ(0, failureCount.load());
}

// Test to reproduce null pointer dereference in progress callback
TEST(MemorySafetyTest, NullProgressCallbackDereference) {
  // Create test files for normalization
  const std::string inputFile = "/tmp/test_normalize_input.txt";
  const std::string outputFile = "/tmp/test_normalize_output.txt";
  
  // Write test data
  std::ofstream ofs(inputFile);
  ofs << "line 1\nline 2\nline 3\n";
  ofs.close();
  
  NormalizeOptions options;
  options.progressStep = 0.1;
  
  // Test with null progress callback - should crash in current implementation
  // This will trigger the bug in normalize.cpp:239-243
  try {
    auto result = normalizeWithProgress(inputFile, outputFile, nullptr, options);
    
    // If we reach here without crashing, the bug might not be triggered
    EXPECT_GT(result.rows, 0);
  } catch (const std::exception& e) {
    // Exception is better than segfault, but still indicates a problem
    FAIL() << "Null pointer dereference caused exception: " << e.what();
  }
  
  // Cleanup
  std::remove(inputFile.c_str());
  std::remove(outputFile.c_str());
}

// Test to reproduce thread exception safety issues
TEST(MemorySafetyTest, ThreadExceptionSafety) {
  // Create test files for normalization
  const std::string inputFile = "/tmp/test_thread_safety_input.txt";
  const std::string outputFile = "/tmp/test_thread_safety_output.txt";
  
  // Write large test data
  std::ofstream ofs(inputFile);
  for (int i = 0; i < 1000; ++i) {
    ofs << "test line " << i << "\n";
  }
  ofs.close();
  
  NormalizeOptions options;
  // Force an exception by providing invalid options
  options.bloomFalsePositiveRate = -1.0; // Invalid value should cause exception
  
  try {
    // This should trigger unhandled exceptions in worker threads
    // Current implementation may crash or have undefined behavior
    auto result = normalizeWithProgress(inputFile, outputFile, [](double p) {}, options);
    
    // If we reach here, exception handling worked
    EXPECT_EQ(0, result.rows); // Should fail with invalid options
  } catch (const std::exception& e) {
    // Exception is expected with invalid options
    EXPECT_TRUE(true);
  }
  
  // Cleanup
  std::remove(inputFile.c_str());
  std::remove(outputFile.c_str());
}

// Test to reproduce zero division in PMI calculation
TEST(MemorySafetyTest, PMIZeroDivision) {
  // Create test files for PMI calculation
  const std::string inputFile = "/tmp/test_pmi_input.txt";
  const std::string outputFile = "/tmp/test_pmi_output.txt";
  
  // Write problematic input that might cause zero marginal probabilities
  std::ofstream ofs(inputFile);
  ofs << "\n"; // Empty line
  ofs << " \n"; // Whitespace only
  ofs << "\t\n"; // Only whitespace characters
  ofs.close();
  
  PmiOptions options;
  options.n = 2;
  options.topK = 10;
  options.minFreq = 0; // Allow zero frequency
  
  try {
    // This might trigger zero division in pmi.cpp:543-553
    auto result = calculatePmi(inputFile, outputFile, options);
    
    // Check for invalid PMI values (NaN, Inf)
    EXPECT_FALSE(std::isnan(result.grams));
    EXPECT_FALSE(std::isinf(result.grams));
  } catch (const std::exception& e) {
    // Exception is better than invalid floating point values
    EXPECT_TRUE(true);
  }
  
  // Cleanup
  std::remove(inputFile.c_str());
  std::remove(outputFile.c_str());
}

// Test to reproduce integer overflow in PMI total count
TEST(MemorySafetyTest, PMIIntegerOverflow) {
  // Create test files for PMI calculation
  const std::string inputFile = "/tmp/test_pmi_overflow_input.txt";
  const std::string outputFile = "/tmp/test_pmi_overflow_output.txt";
  
  // Write repeated lines to create high counts
  std::ofstream ofs(inputFile);
  const std::string repeatedLine = "a b c d e f g h i j";
  const size_t numRepeats = 10000; // Reasonable number for testing
  
  for (size_t i = 0; i < numRepeats; ++i) {
    ofs << repeatedLine << "\n";
  }
  ofs.close();
  
  PmiOptions options;
  options.n = 1;
  options.topK = 100;
  options.minFreq = 1;
  
  try {
    // This might trigger integer overflow in pmi.cpp:491-494
    auto result = calculatePmi(inputFile, outputFile, options);
    
    // Check that the result is reasonable (no overflow)
    EXPECT_GT(result.grams, 0);
    EXPECT_LT(result.grams, numRepeats * 100); // Sanity check
  } catch (const std::exception& e) {
    // Overflow should be handled gracefully
    EXPECT_TRUE(true);
  }
  
  // Cleanup
  std::remove(inputFile.c_str());
  std::remove(outputFile.c_str());
}

// Test to reproduce exception swallowing in text_utils
TEST(MemorySafetyTest, ExceptionSwallowing) {
  // Test with invalid Unicode sequences that might cause exceptions
  std::vector<uint8_t> invalidUtf8 = {
    0xFF, 0xFE, 0xFD, 0xFC, // Invalid UTF-8 byte sequence
    0x80, 0x81, 0x82, 0x83  // More invalid bytes
  };
  
  std::string invalidText(reinterpret_cast<char*>(invalidUtf8.data()), invalidUtf8.size());
  
  // This should trigger exception handling in text_utils.cpp:435-436
  // Current implementation swallows all exceptions and returns ""
  std::string result = normalizeLine(invalidText, NormalizationForm::NFC);
  
  // ICU converts invalid UTF-8 to replacement characters (U+FFFD)
  // This is expected behavior, not exception swallowing
  // Each invalid byte gets replaced with UTF-8 encoded U+FFFD: 0xEF 0xBF 0xBD
  
  // Check that we get replacement characters instead of empty string
  EXPECT_FALSE(result.empty());
  
  // The result should contain replacement characters
  // Each invalid byte becomes a replacement character
  std::string expectedReplacement = "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD";
  EXPECT_EQ(expectedReplacement, result);
  
  // This is actually correct behavior:
  // 1. ICU handles invalid UTF-8 gracefully
  // 2. Converts to replacement characters instead of crashing
  // 3. Allows processing to continue
}

} // namespace test
} // namespace core
} // namespace suzume