/**
 * @file buffer_api_test.cpp
 * @brief Tests for buffer-based API
 */

#include <gtest/gtest.h>
#include "core/buffer_api.h"
#include <cstring>
#include <string>
#include <vector>

namespace suzume {
namespace core {
namespace test {

// Test normalizeBuffer with simple input
TEST(BufferApiTest, NormalizeBufferSimple) {
  // Create test input
  const char* testData = "line1\nline2\nline1\nline3\n";
  size_t testDataLen = strlen(testData);

  // Create output variables
  uint8_t* outputData = nullptr;
  size_t outputLength = 0;

  // Create options
  NormalizeOptions options;
  options.form = NormalizationForm::NFC;
  options.bloomFalsePositiveRate = 0.01;

  // Call normalizeBuffer
  NormalizeResult result = normalizeBuffer(
    reinterpret_cast<const uint8_t*>(testData),
    testDataLen,
    &outputData,
    &outputLength,
    options
  );

  // Check result
  ASSERT_NE(nullptr, outputData);
  ASSERT_GT(outputLength, 0);
  EXPECT_EQ(4, result.rows);
  EXPECT_EQ(3, result.uniques);
  EXPECT_EQ(1, result.duplicates);

  // Convert output to string for easier checking
  std::string output(reinterpret_cast<char*>(outputData), outputLength);

  // Check output content (should contain unique lines)
  EXPECT_NE(std::string::npos, output.find("line1"));
  EXPECT_NE(std::string::npos, output.find("line2"));
  EXPECT_NE(std::string::npos, output.find("line3"));

  // Clean up
  delete[] outputData;
}

// Test normalizeBuffer with empty input
TEST(BufferApiTest, NormalizeBufferEmpty) {
  // Create empty input
  const char* testData = "";
  size_t testDataLen = 0;

  // Create output variables
  uint8_t* outputData = nullptr;
  size_t outputLength = 0;

  // Create options
  NormalizeOptions options;

  // Call normalizeBuffer
  NormalizeResult result = normalizeBuffer(
    reinterpret_cast<const uint8_t*>(testData),
    testDataLen,
    &outputData,
    &outputLength,
    options
  );

  // Check result
  EXPECT_EQ(0, result.rows);
  EXPECT_EQ(0, result.uniques);
  EXPECT_EQ(0, result.duplicates);
  EXPECT_EQ(0, outputLength);

  // Clean up if needed
  if (outputData) {
    delete[] outputData;
  }
}

// Test normalizeBuffer with Unicode input
TEST(BufferApiTest, NormalizeBufferUnicode) {
  // Create test input with Unicode characters
  const char* testData = "café\nこんにちは\n你好\n";
  size_t testDataLen = strlen(testData);

  // Create output variables
  uint8_t* outputData = nullptr;
  size_t outputLength = 0;

  // Create options
  NormalizeOptions options;
  options.form = NormalizationForm::NFC;

  // Call normalizeBuffer
  NormalizeResult result = normalizeBuffer(
    reinterpret_cast<const uint8_t*>(testData),
    testDataLen,
    &outputData,
    &outputLength,
    options
  );

  // Check result
  ASSERT_NE(nullptr, outputData);
  ASSERT_GT(outputLength, 0);
  EXPECT_EQ(3, result.rows);
  EXPECT_EQ(3, result.uniques);

  // Convert output to string for easier checking
  std::string output(reinterpret_cast<char*>(outputData), outputLength);

  // Check output content
  EXPECT_NE(std::string::npos, output.find("café"));
  EXPECT_NE(std::string::npos, output.find("こんにちは"));
  EXPECT_NE(std::string::npos, output.find("你好"));

  // Clean up
  delete[] outputData;
}

// Test calculatePmiFromBuffer with simple input
TEST(BufferApiTest, CalculatePmiFromBufferSimple) {
  // Create test input
  const char* testData = "this is a test\nthis is another test\nyet another test\n";
  size_t testDataLen = strlen(testData);

  // Create output variables
  uint8_t* outputData = nullptr;
  size_t outputLength = 0;

  // Create options
  PmiOptions options;
  options.n = 2;
  options.topK = 10;
  options.minFreq = 1;

  // Call calculatePmiFromBuffer
  PmiResult result = calculatePmiFromBuffer(
    reinterpret_cast<const uint8_t*>(testData),
    testDataLen,
    &outputData,
    &outputLength,
    options
  );

  // Check result
  ASSERT_NE(nullptr, outputData);
  ASSERT_GT(outputLength, 0);
  EXPECT_GT(result.grams, 0);

  // Convert output to string for easier checking
  std::string output(reinterpret_cast<char*>(outputData), outputLength);

  // Output should be in TSV format with n-grams, scores, and frequencies
  // Just check that we have some output, not specific n-grams
  // since the exact output depends on the PMI calculation
  EXPECT_GT(outputLength, 0);
  EXPECT_GT(output.length(), 0);

  // Clean up
  delete[] outputData;
}

// Test progress reporting
TEST(BufferApiTest, ProgressReporting) {
  // Create test input
  const char* testData = "line1\nline2\nline3\nline4\nline5\n";
  size_t testDataLen = strlen(testData);

  // Create output variables
  uint8_t* outputData = nullptr;
  size_t outputLength = 0;

  // Create options
  NormalizeOptions options;

  // Create progress buffer
  uint32_t progressBuffer[3] = {0, 0, 0};

  // Call normalizeBuffer with progress buffer
  normalizeBuffer(
    reinterpret_cast<const uint8_t*>(testData),
    testDataLen,
    &outputData,
    &outputLength,
    options,
    progressBuffer
  );

  // Check result
  ASSERT_NE(nullptr, outputData);

  // Check progress buffer
  EXPECT_EQ(4, progressBuffer[0]); // Phase should be Complete (4)
  EXPECT_GT(progressBuffer[1], 0); // Current progress should be > 0
  EXPECT_GT(progressBuffer[2], 0); // Total should be > 0

  // Clean up
  delete[] outputData;
}

} // namespace test
} // namespace core
} // namespace suzume
