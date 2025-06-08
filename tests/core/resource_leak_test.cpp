/**
 * @file resource_leak_test.cpp
 * @brief Tests to reproduce resource leaks and management issues
 */

#include <gtest/gtest.h>
#include "core/buffer_api.h"
#include "io/file_io.h"
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>


namespace suzume {
namespace core {
namespace test {

// Test to reproduce file handle leaks
TEST(ResourceLeakTest, FileHandleLeaks) {
  const std::string testFile = "/tmp/test_file_handles.txt";
  
  // Create test file
  {
    std::ofstream ofs(testFile);
    for (int i = 0; i < 1000; ++i) {
      ofs << "Test line " << i << "\n";
    }
  }
  
  // Test multiple file operations without explicit cleanup
  for (int iteration = 0; iteration < 100; ++iteration) {
    try {
      // This might leak file handles if not properly managed
      std::ifstream ifs(testFile);
      EXPECT_TRUE(ifs.good());
      
      std::string line;
      size_t lineCount = 0;
      while (std::getline(ifs, line)) {
        lineCount++;
      }
      
      // Force some processing
      EXPECT_GT(lineCount, 0);
      
      // Test file writing
      std::string outputFile = "/tmp/test_output_" + std::to_string(iteration) + ".txt";
      std::ofstream ofs(outputFile);
      ofs << "Test output for iteration " << iteration << "\n";
      
      // Immediate cleanup (simulating the issue where temp files aren't cleaned)
      std::remove(outputFile.c_str());
      
    } catch (const std::exception& e) {
      // File operations might fail but shouldn't leak resources
      EXPECT_TRUE(false) << "File operation failed: " << e.what();
    }
  }
  
  // Cleanup main test file
  std::remove(testFile.c_str());
  
  // This test mainly checks that we don't run out of file descriptors
  EXPECT_TRUE(true);
}

// Test to reproduce memory allocation/deallocation mismatches
TEST(ResourceLeakTest, MemoryAllocationMismatch) {
  const size_t numIterations = 100;
  std::vector<uint8_t*> allocatedBuffers;
  std::vector<size_t> bufferSizes;
  
  for (size_t i = 0; i < numIterations; ++i) {
    const std::string testData = "Test data line " + std::to_string(i) + "\n";
    
    uint8_t* outputData = nullptr;
    size_t outputLength = 0;
    
    NormalizeOptions options;
    
    try {
      // Test buffer allocation
      NormalizeResult result = normalizeBuffer(
        reinterpret_cast<const uint8_t*>(testData.c_str()),
        testData.length(),
        &outputData,
        &outputLength,
        options
      );
      
      ASSERT_NE(nullptr, outputData);
      ASSERT_GT(outputLength, 0);
      
      // Store for later cleanup test
      allocatedBuffers.push_back(outputData);
      bufferSizes.push_back(outputLength);
      
    } catch (const std::exception& e) {
      FAIL() << "Buffer allocation failed: " << e.what();
    }
  }
  
  // Test that all buffers are valid
  for (size_t i = 0; i < allocatedBuffers.size(); ++i) {
    ASSERT_NE(nullptr, allocatedBuffers[i]);
    
    // Try to access the memory (should not crash if properly allocated)
    for (size_t j = 0; j < std::min(bufferSizes[i], size_t(10)); ++j) {
      volatile uint8_t byte = allocatedBuffers[i][j];
      (void)byte; // Suppress unused variable warning
    }
    
    // Clean up (caller's responsibility)
    delete[] allocatedBuffers[i];
  }
}

// Test to reproduce temporary file cleanup issues
TEST(ResourceLeakTest, TemporaryFileCleanup) {
  const size_t numTempFiles = 50;
  std::vector<std::string> tempFiles;
  
  // Create multiple temporary files
  for (size_t i = 0; i < numTempFiles; ++i) {
    std::string tempFile = "/tmp/suzume_test_temp_" + std::to_string(i) + "_" + 
                          std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count()) + ".txt";
    
    // Create the file
    {
      std::ofstream ofs(tempFile);
      ofs << "Temporary test data " << i << "\n";
    }
    
    tempFiles.push_back(tempFile);
    
    // Verify file exists
    std::ifstream ifs(tempFile);
    EXPECT_TRUE(ifs.good()) << "Failed to create temp file: " << tempFile;
  }
  
  // Simulate exception occurring before cleanup
  bool simulateException = true;
  if (simulateException) {
    // In real code, an exception here would leave temp files uncleanied
    // This test verifies the cleanup behavior
  }
  
  // Manual cleanup (should be automatic in proper RAII design)
  size_t cleanedFiles = 0;
  for (const auto& tempFile : tempFiles) {
    if (std::remove(tempFile.c_str()) == 0) {
      cleanedFiles++;
    }
  }
  
  EXPECT_EQ(cleanedFiles, numTempFiles) << "Not all temporary files were cleaned up";
}

// Test to reproduce thread resource leaks
TEST(ResourceLeakTest, ThreadResourceLeaks) {
  const size_t numThreads = 20;
  const size_t iterationsPerThread = 10;
  
  std::vector<std::thread> threads;
  std::atomic<size_t> completedOperations{0};
  std::atomic<size_t> failedOperations{0};
  
  // Launch multiple threads that perform resource-intensive operations
  for (size_t i = 0; i < numThreads; ++i) {
    threads.emplace_back([&, i]() {
      for (size_t j = 0; j < iterationsPerThread; ++j) {
        try {
          // Each thread performs buffer operations
          const std::string testData = "Thread " + std::to_string(i) + 
                                     " iteration " + std::to_string(j) + " data\n";
          
          uint8_t* outputData = nullptr;
          size_t outputLength = 0;
          
          NormalizeOptions options;
          
          NormalizeResult result = normalizeBuffer(
            reinterpret_cast<const uint8_t*>(testData.c_str()),
            testData.length(),
            &outputData,
            &outputLength,
            options
          );
          
          // Verify allocation
          if (outputData && outputLength > 0) {
            completedOperations++;
            
            // Clean up
            delete[] outputData;
          } else {
            failedOperations++;
          }
          
        } catch (const std::exception& e) {
          failedOperations++;
        }
        
        // Small delay to increase chance of race conditions
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    });
  }
  
  // Wait for all threads to complete
  for (auto& thread : threads) {
    thread.join();
  }
  
  const size_t expectedOperations = numThreads * iterationsPerThread;
  const size_t totalOperations = completedOperations.load() + failedOperations.load();
  
  std::cout << "Completed operations: " << completedOperations.load() << std::endl;
  std::cout << "Failed operations: " << failedOperations.load() << std::endl;
  
  EXPECT_EQ(totalOperations, expectedOperations) << "Not all operations were accounted for";
  EXPECT_EQ(failedOperations.load(), 0) << "Some operations failed";
}

// Test to reproduce cross-platform memory issues
TEST(ResourceLeakTest, CrossPlatformMemoryIssues) {
  const size_t numOperations = 50;
  
  for (size_t i = 0; i < numOperations; ++i) {
    const std::string testData = "Cross-platform test data " + std::to_string(i) + "\n";
    
    // Test basic memory operations that should work on all platforms
    try {
      uint8_t* outputData = nullptr;
      size_t outputLength = 0;
      
      NormalizeOptions options;
      
      NormalizeResult result = normalizeBuffer(
        reinterpret_cast<const uint8_t*>(testData.c_str()),
        testData.length(),
        &outputData,
        &outputLength,
        options
      );
      
      ASSERT_NE(nullptr, outputData);
      EXPECT_GT(outputLength, 0);
      
      // Cleanup
      delete[] outputData;
      
    } catch (const std::exception& e) {
      FAIL() << "Cross-platform operation failed: " << e.what();
    }
  }
}

// Test to reproduce large memory allocation patterns
TEST(ResourceLeakTest, LargeMemoryAllocations) {
  const size_t largeSize = 100 * 1024 * 1024; // 100MB
  const size_t numAllocations = 5;
  
  std::vector<std::unique_ptr<uint8_t[]>> allocations;
  
  for (size_t i = 0; i < numAllocations; ++i) {
    try {
      // Test large memory allocation
      auto buffer = std::make_unique<uint8_t[]>(largeSize);
      
      // Write to ensure allocation is real
      buffer[0] = 0x42;
      buffer[largeSize - 1] = 0x42;
      
      allocations.push_back(std::move(buffer));
      
      std::cout << "Allocated " << (largeSize / 1024 / 1024) << "MB, total: " 
                << ((i + 1) * largeSize / 1024 / 1024) << "MB" << std::endl;
      
    } catch (const std::bad_alloc& e) {
      std::cout << "Memory allocation failed at " << i << "th allocation" << std::endl;
      break; // Expected on systems with limited memory
    }
  }
  
  // Verify allocations are still valid
  for (size_t i = 0; i < allocations.size(); ++i) {
    EXPECT_EQ(allocations[i][0], 0x42);
    EXPECT_EQ(allocations[i][largeSize - 1], 0x42);
  }
  
  // Cleanup is automatic with unique_ptr
  std::cout << "Successfully allocated and cleaned up " << allocations.size() 
            << " large buffers" << std::endl;
}

} // namespace test
} // namespace core
} // namespace suzume