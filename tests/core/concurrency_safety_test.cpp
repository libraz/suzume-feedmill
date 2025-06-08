/**
 * @file concurrency_safety_test.cpp  
 * @brief Tests to reproduce concurrency safety issues
 */

#include <gtest/gtest.h>
#include "core/text_utils.h"
#include "parallel/executor.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <future>

namespace suzume {
namespace core {
namespace test {

// Test to reproduce ICU initialization race condition
TEST(ConcurrencySafetyTest, ICUInitializationRace) {
  const int numThreads = 20;
  std::vector<std::future<bool>> futures;
  std::atomic<int> successCount{0};
  std::atomic<int> failureCount{0};
  
  // Launch many threads simultaneously to trigger race condition
  for (int i = 0; i < numThreads; ++i) {
    futures.push_back(std::async(std::launch::async, [&, i]() {
      try {
        // Multiple threads calling this simultaneously should trigger race
        // in text_utils.cpp g_initialized flag
        std::string testText = "Thread " + std::to_string(i) + ": café こんにちは";
        std::string result = normalizeLine(testText, NormalizationForm::NFC);
        
        if (!result.empty()) {
          successCount++;
          return true;
        } else {
          failureCount++;
          return false;
        }
      } catch (...) {
        failureCount++;
        return false;
      }
    }));
  }
  
  // Wait for all tasks and collect results
  bool allSucceeded = true;
  for (auto& future : futures) {
    if (!future.get()) {
      allSucceeded = false;
    }
  }
  
  // With race condition, some threads may fail
  EXPECT_TRUE(allSucceeded) << "Some threads failed due to race condition. "
                            << "Success: " << successCount.load() 
                            << ", Failures: " << failureCount.load();
}

// Test to reproduce double-checked locking issues
TEST(ConcurrencySafetyTest, DoubleCheckedLockingRace) {
  const int numIterations = 100;
  std::atomic<int> raceConditionDetected{0};
  
  for (int iteration = 0; iteration < numIterations; ++iteration) {
    const int numThreads = 10;
    std::vector<std::thread> threads;
    std::atomic<bool> startFlag{false};
    
    // All threads will wait for start signal then call normalizeLine
    for (int i = 0; i < numThreads; ++i) {
      threads.emplace_back([&, i]() {
        // Wait for start signal to maximize chance of race condition
        while (!startFlag.load()) {
          std::this_thread::yield();
        }
        
        try {
          std::string testText = "Race test " + std::to_string(i);
          std::string result = normalizeLine(testText, NormalizationForm::NFC);
          
          if (result.empty()) {
            raceConditionDetected++;
          }
        } catch (...) {
          raceConditionDetected++;
        }
      });
    }
    
    // Start all threads simultaneously
    startFlag.store(true);
    
    // Wait for all threads
    for (auto& thread : threads) {
      thread.join();
    }
  }
  
  // If there are race conditions, some calls might fail
  EXPECT_EQ(0, raceConditionDetected.load()) 
    << "Race conditions detected in " << raceConditionDetected.load() << " cases";
}

// Test to reproduce executor exception safety issues
TEST(ConcurrencySafetyTest, ExecutorExceptionSafety) {
  std::vector<int> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  
  // Mapper function that throws exception on specific values
  std::function<int(const int&)> faultyMapper = [](const int& x) -> int {
    if (x == 5) {
      throw std::runtime_error("Intentional exception for testing");
    }
    return x * 2;
  };
  
  try {
    // This should trigger exception handling issues in executor.h:76-80
    auto result = parallel::ParallelExecutor::parallelMap(input, faultyMapper, 4);
    
    // If we reach here, exception wasn't properly propagated
    FAIL() << "Exception should have been thrown but wasn't";
  } catch (const std::runtime_error& e) {
    // Expected exception - good
    EXPECT_EQ(std::string("Intentional exception for testing"), e.what());
  } catch (...) {
    // Wrong exception type or unhandled exception
    FAIL() << "Unexpected exception type or unhandled exception";
  }
}

// Test for memory ordering issues with progress buffer
TEST(ConcurrencySafetyTest, ProgressBufferMemoryOrdering) {
  const int numIterations = 1000;
  std::atomic<int> inconsistentReads{0};
  
  for (int i = 0; i < numIterations; ++i) {
    uint32_t progressBuffer[3] = {0, 0, 0};
    
    // Writer thread
    std::thread writer([&progressBuffer]() {
      for (uint32_t j = 1; j <= 100; ++j) {
        // Simulate the problematic atomic operations from buffer_api.cpp
        // This is the problematic code pattern we're testing
        std::atomic_store_explicit(
          reinterpret_cast<std::atomic<uint32_t>*>(&progressBuffer[0]),
          1, // phase
          std::memory_order_release
        );
        std::atomic_store_explicit(
          reinterpret_cast<std::atomic<uint32_t>*>(&progressBuffer[1]), 
          j, // current
          std::memory_order_release
        );
        std::atomic_store_explicit(
          reinterpret_cast<std::atomic<uint32_t>*>(&progressBuffer[2]),
          100, // total
          std::memory_order_release
        );
        
        std::this_thread::sleep_for(std::chrono::microseconds(1));
      }
    });
    
    // Reader thread
    std::thread reader([&progressBuffer, &inconsistentReads]() {
      for (int k = 0; k < 200; ++k) {
        uint32_t phase = std::atomic_load_explicit(
          reinterpret_cast<std::atomic<uint32_t>*>(&progressBuffer[0]),
          std::memory_order_acquire
        );
        uint32_t current = std::atomic_load_explicit(
          reinterpret_cast<std::atomic<uint32_t>*>(&progressBuffer[1]),
          std::memory_order_acquire  
        );
        uint32_t total = std::atomic_load_explicit(
          reinterpret_cast<std::atomic<uint32_t>*>(&progressBuffer[2]),
          std::memory_order_acquire
        );
        
        // Check for inconsistent state
        if (current > total || (phase == 1 && current == 0 && total > 0)) {
          inconsistentReads++;
        }
        
        std::this_thread::sleep_for(std::chrono::microseconds(1));
      }
    });
    
    writer.join();
    reader.join();
  }
  
  // Memory ordering issues may cause inconsistent reads
  EXPECT_EQ(0, inconsistentReads.load()) 
    << "Inconsistent reads detected: " << inconsistentReads.load();
}

// Test for data race in global state access
TEST(ConcurrencySafetyTest, GlobalStateDataRace) {
  const int numThreads = 20;
  const int numCalls = 100;
  std::vector<std::thread> threads;
  std::atomic<int> totalCalls{0};
  std::atomic<int> failedCalls{0};
  
  // Launch multiple threads that access global state
  for (int i = 0; i < numThreads; ++i) {
    threads.emplace_back([&, i]() {
      for (int j = 0; j < numCalls; ++j) {
        try {
          std::string testText = "Global state test " + std::to_string(i) + "_" + std::to_string(j);
          
          // Multiple calls to functions that modify global state
          std::string result1 = normalizeLine(testText, NormalizationForm::NFC);
          std::string result2 = normalizeLine(testText, NormalizationForm::NFKC);
          
          totalCalls += 2;
          
          if (result1.empty() || result2.empty()) {
            failedCalls++;
          }
        } catch (...) {
          failedCalls++;
        }
      }
    });
  }
  
  // Wait for all threads
  for (auto& thread : threads) {
    thread.join();
  }
  
  // Data races may cause some calls to fail
  int expectedCalls = numThreads * numCalls * 2;
  EXPECT_EQ(expectedCalls, totalCalls.load()) 
    << "Expected " << expectedCalls << " calls, got " << totalCalls.load();
  EXPECT_EQ(0, failedCalls.load())
    << "Failed calls due to data races: " << failedCalls.load();
}

} // namespace test  
} // namespace core
} // namespace suzume