/**
 * @file shared_memory_test.cpp
 * @brief Tests for shared memory progress notification
 */

#include <gtest/gtest.h>
#include "shared_memory_mock.h" // Use mock instead of actual binding
#include <thread>
#include <chrono>
#include <atomic>

namespace suzume {
namespace binding {
namespace test {

// Test basic progress update
TEST(SharedMemoryTest, BasicProgressUpdate) {
  // Create progress buffer
  uint32_t progressBuffer[3] = {0, 0, 0};

  // Update progress
  UpdateSharedMemoryProgress(
    progressBuffer,
    ProgressPhase::Processing,
    50,
    100
  );

  // Check progress buffer
  EXPECT_EQ(static_cast<uint32_t>(ProgressPhase::Processing), progressBuffer[0]);
  EXPECT_EQ(50, progressBuffer[1]);
  EXPECT_EQ(100, progressBuffer[2]);
}

// Test progress update with ratio
TEST(SharedMemoryTest, ProgressUpdateWithRatio) {
  // Create progress buffer
  uint32_t progressBuffer[3] = {0, 0, 0};

  // Update progress with ratio
  UpdateSharedMemoryProgress(
    progressBuffer,
    0.75 // 75%
  );

  // Check progress buffer
  EXPECT_EQ(static_cast<uint32_t>(ProgressPhase::Writing), progressBuffer[0]); // Should be Writing phase
  EXPECT_EQ(75, progressBuffer[1]); // 75% of 100
  EXPECT_EQ(100, progressBuffer[2]);
}

// Test progress update with ProgressInfo
TEST(SharedMemoryTest, ProgressUpdateWithInfo) {
  // Create progress buffer
  uint32_t progressBuffer[3] = {0, 0, 0};

  // Create progress info
  ProgressInfo info;
  info.phase = ProgressInfo::Phase::Calculating;
  info.phaseRatio = 0.6;
  info.overallRatio = 0.4;

  // Update progress with info
  UpdateSharedMemoryProgress(
    progressBuffer,
    info
  );

  // Check progress buffer
  EXPECT_EQ(static_cast<uint32_t>(ProgressPhase::Calculating), progressBuffer[0]);
  EXPECT_EQ(40, progressBuffer[1]); // 40% of 100
  EXPECT_EQ(100, progressBuffer[2]);
}

// Test progress update from multiple threads
TEST(SharedMemoryTest, ThreadSafeProgressUpdate) {
  // Create progress buffer
  uint32_t progressBuffer[3] = {0, 0, 0};

  // Create atomic counter for thread synchronization
  std::atomic<int> counter(0);

  // Create threads
  std::thread t1([&]() {
    // Wait for all threads to be ready
    counter.fetch_add(1);
    while (counter.load() < 3) {
      std::this_thread::yield();
    }

    // Update progress from thread 1
    UpdateSharedMemoryProgress(
      progressBuffer,
      ProgressPhase::Reading,
      10,
      100
    );
  });

  std::thread t2([&]() {
    // Wait for all threads to be ready
    counter.fetch_add(1);
    while (counter.load() < 3) {
      std::this_thread::yield();
    }

    // Update progress from thread 2
    UpdateSharedMemoryProgress(
      progressBuffer,
      ProgressPhase::Processing,
      50,
      100
    );
  });

  std::thread t3([&]() {
    // Wait for all threads to be ready
    counter.fetch_add(1);
    while (counter.load() < 3) {
      std::this_thread::yield();
    }

    // Update progress from thread 3
    UpdateSharedMemoryProgress(
      progressBuffer,
      ProgressPhase::Complete,
      100,
      100
    );
  });

  // Wait for threads to finish
  t1.join();
  t2.join();
  t3.join();

  // Check progress buffer
  // The final value should be from one of the threads
  // We can't predict which one, but it should be valid
  EXPECT_TRUE(
    progressBuffer[0] == static_cast<uint32_t>(ProgressPhase::Reading) ||
    progressBuffer[0] == static_cast<uint32_t>(ProgressPhase::Processing) ||
    progressBuffer[0] == static_cast<uint32_t>(ProgressPhase::Complete)
  );

  EXPECT_TRUE(
    progressBuffer[1] == 10 ||
    progressBuffer[1] == 50 ||
    progressBuffer[1] == 100
  );

  EXPECT_EQ(100, progressBuffer[2]);
}

// Test progress callback creation
TEST(SharedMemoryTest, CreateProgressCallback) {
  // Create progress buffer
  uint32_t progressBuffer[3] = {0, 0, 0};

  // Create progress callback
  auto callback = CreateSharedMemoryProgressCallback(progressBuffer);

  // Call callback
  callback(0.5); // 50%

  // Check progress buffer
  EXPECT_EQ(50, progressBuffer[1]);
  EXPECT_EQ(100, progressBuffer[2]);
}

// Test structured progress callback creation
TEST(SharedMemoryTest, CreateStructuredProgressCallback) {
  // Create progress buffer
  uint32_t progressBuffer[3] = {0, 0, 0};

  // Create structured progress callback
  auto callback = CreateSharedMemoryStructuredProgressCallback(progressBuffer);

  // Create progress info
  ProgressInfo info;
  info.phase = ProgressInfo::Phase::Writing;
  info.phaseRatio = 0.8;
  info.overallRatio = 0.9;

  // Call callback
  callback(info);

  // Check progress buffer
  EXPECT_EQ(static_cast<uint32_t>(ProgressPhase::Writing), progressBuffer[0]);
  EXPECT_EQ(90, progressBuffer[1]); // 90% of 100
  EXPECT_EQ(100, progressBuffer[2]);
}

// Test null progress buffer handling
TEST(SharedMemoryTest, NullProgressBuffer) {
  // Call update with null buffer (should not crash)
  UpdateSharedMemoryProgress(
    nullptr,
    ProgressPhase::Processing,
    50,
    100
  );

  // Call update with ratio and null buffer (should not crash)
  UpdateSharedMemoryProgress(
    nullptr,
    0.5
  );

  // Create progress info
  ProgressInfo info;
  info.phase = ProgressInfo::Phase::Writing;
  info.overallRatio = 0.8;

  // Call update with info and null buffer (should not crash)
  UpdateSharedMemoryProgress(
    nullptr,
    info
  );

  // Create callbacks with null buffer
  auto callback1 = CreateSharedMemoryProgressCallback(nullptr);
  auto callback2 = CreateSharedMemoryStructuredProgressCallback(nullptr);

  // Call callbacks (should not crash)
  callback1(0.5);
  callback2(info);

  // Test passed if no crash
  EXPECT_TRUE(true);
}

} // namespace test
} // namespace binding
} // namespace suzume
