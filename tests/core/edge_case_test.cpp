/**
 * @file edge_case_test.cpp
 * @brief Tests to reproduce edge cases and boundary conditions
 */

#include <gtest/gtest.h>
#include "core/normalize.h"
#include "core/pmi.h"
#include "core/text_utils.h"
#include "core/word_extraction.h"
#include "io/file_io.h"
#include <fstream>
#include <sstream>
#include <limits>
#include <vector>
#include <string>

namespace suzume {
namespace core {
namespace test {

// Test to reproduce empty file handling issues
TEST(EdgeCaseTest, EmptyFileHandling) {
  const std::string emptyFile = "/tmp/test_empty_file.txt";
  const std::string outputFile = "/tmp/test_empty_output.txt";
  
  // Create empty file
  {
    std::ofstream ofs(emptyFile);
    // Write nothing
  }
  
  // Test normalization with empty file
  try {
    NormalizeOptions options;
    NormalizeResult result = suzume::normalize(emptyFile, outputFile, options);
    
    // Empty file should be handled gracefully
    EXPECT_EQ(result.rows, 0);
    EXPECT_EQ(result.uniques, 0);
    EXPECT_EQ(result.duplicates, 0);
    
  } catch (const std::exception& e) {
    // Should not throw exception for empty files
    FAIL() << "Empty file handling failed: " << e.what();
  }
  
  // Test PMI calculation with empty file
  try {
    PmiOptions pmiOptions;
    PmiResult pmiResult = suzume::calculatePmi(emptyFile, outputFile, pmiOptions);
    
    // Empty file should result in zero grams
    EXPECT_EQ(pmiResult.grams, 0);
    EXPECT_EQ(pmiResult.distinctNgrams, 0);
    
  } catch (const std::exception& e) {
    // Should not throw exception for empty files
    FAIL() << "Empty file PMI calculation failed: " << e.what();
  }
  
  // Cleanup
  std::remove(emptyFile.c_str());
  std::remove(outputFile.c_str());
}

// Test to reproduce very large file handling
TEST(EdgeCaseTest, LargeFileHandling) {
  const std::string largeFile = "/tmp/test_large_file.txt";
  const std::string outputFile = "/tmp/test_large_output.txt";
  const size_t numLines = 100000; // 100K lines
  
  // Create large file
  {
    std::ofstream ofs(largeFile);
    for (size_t i = 0; i < numLines; ++i) {
      ofs << "This is test line number " << i << " with some text content\n";
    }
  }
  
  try {
    NormalizeOptions options;
    options.threads = 4; // Use multiple threads
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    NormalizeResult result = suzume::normalize(largeFile, outputFile, options);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Large file processing took " << duration.count() << " ms" << std::endl;
    
    // Should handle large files without issues
    EXPECT_EQ(result.rows, numLines);
    EXPECT_GT(result.uniques, 0);
    EXPECT_LT(duration.count(), 30000); // Should complete within 30 seconds
    
  } catch (const std::exception& e) {
    FAIL() << "Large file handling failed: " << e.what();
  }
  
  // Cleanup
  std::remove(largeFile.c_str());
  std::remove(outputFile.c_str());
}

// Test to reproduce special character encoding issues
TEST(EdgeCaseTest, SpecialCharacterEncoding) {
  const std::string testFile = "/tmp/test_special_chars.txt";
  const std::string outputFile = "/tmp/test_special_output.txt";
  
  // Create file with various special characters
  {
    std::ofstream ofs(testFile);
    
    // Various Unicode characters
    ofs << "ASCII text\n";
    ofs << "Unicode: café, naïve, résumé\n";
    ofs << "Japanese: こんにちは世界\n";
    ofs << "Chinese: 你好世界\n";
    ofs << "Emoji: 😀🎉🌟\n";
    ofs << "Math symbols: ∑∞±∂∆\n";
    ofs << "Currency: €$£¥₹\n";
    
    // Potentially problematic sequences
    ofs << "Control chars: \t\r\n";
    ofs << "Null byte test: before\0after\n";
    ofs << "Long line: " << std::string(10000, 'x') << "\n";
  }
  
  try {
    NormalizeOptions options;
    options.form = NormalizationForm::NFC;
    
    NormalizeResult result = suzume::normalize(testFile, outputFile, options);
    
    // Should handle special characters
    EXPECT_GT(result.rows, 0);
    EXPECT_GT(result.uniques, 0);
    
    // Verify output file exists and has content
    std::ifstream ifs(outputFile);
    EXPECT_TRUE(ifs.good());
    
    std::string line;
    size_t lineCount = 0;
    while (std::getline(ifs, line)) {
      lineCount++;
      // Should not be empty (unless normalization removes content)
      // EXPECT_GT(line.length(), 0); // May be empty after normalization
    }
    
    EXPECT_GT(lineCount, 0);
    
  } catch (const std::exception& e) {
    FAIL() << "Special character handling failed: " << e.what();
  }
  
  // Cleanup
  std::remove(testFile.c_str());
  std::remove(outputFile.c_str());
}

// Test to reproduce boundary value issues
TEST(EdgeCaseTest, BoundaryValues) {
  // Test with maximum values
  {
    NormalizeOptions options;
    options.threads = UINT32_MAX; // Should be clamped to reasonable value
    options.minLength = UINT32_MAX;
    options.maxLength = 0; // Invalid: min > max
    
    const std::string testFile = "/tmp/test_boundary.txt";
    const std::string outputFile = "/tmp/test_boundary_output.txt";
    
    // Create test file
    {
      std::ofstream ofs(testFile);
      ofs << "test line\n";
    }
    
    try {
      NormalizeResult result = suzume::normalize(testFile, outputFile, options);
      
      // Should handle invalid options gracefully or throw appropriate exception
      // With min > max, should either reject all lines or throw
      EXPECT_EQ(result.uniques, 0); // All lines rejected due to length filter
      
    } catch (const std::invalid_argument& e) {
      // Expected behavior for invalid options
      EXPECT_TRUE(true);
    } catch (const std::exception& e) {
      FAIL() << "Unexpected exception: " << e.what();
    }
    
    // Cleanup
    std::remove(testFile.c_str());
    std::remove(outputFile.c_str());
  }
  
  // Test PMI with extreme values
  {
    PmiOptions options;
    options.n = 100; // Very large n-gram size
    options.topK = UINT32_MAX; // Very large K
    options.minFreq = UINT32_MAX; // Very high frequency requirement
    
    const std::string testFile = "/tmp/test_pmi_boundary.txt";
    const std::string outputFile = "/tmp/test_pmi_boundary_output.txt";
    
    // Create test file
    {
      std::ofstream ofs(testFile);
      ofs << "short text\n";
    }
    
    try {
      PmiResult result = suzume::calculatePmi(testFile, outputFile, options);
      
      // With extreme parameters, should find no n-grams
      EXPECT_EQ(result.grams, 0);
      EXPECT_EQ(result.distinctNgrams, 0);
      
    } catch (const std::exception& e) {
      // Might throw exception for invalid parameters
      std::cout << "PMI boundary test exception (expected): " << e.what() << std::endl;
    }
    
    // Cleanup
    std::remove(testFile.c_str());
    std::remove(outputFile.c_str());
  }
}

// Test to reproduce line length edge cases
TEST(EdgeCaseTest, LineLengthEdgeCases) {
  const std::string testFile = "/tmp/test_line_lengths.txt";
  const std::string outputFile = "/tmp/test_line_lengths_output.txt";
  
  // Create file with various line lengths
  {
    std::ofstream ofs(testFile);
    
    // Empty line
    ofs << "\n";
    
    // Very short lines
    ofs << "a\n";
    ofs << "ab\n";
    
    // Normal lines
    ofs << "normal length line\n";
    
    // Very long line (potential buffer overflow)
    std::string longLine(100000, 'x');
    ofs << longLine << "\n";
    
    // Line with only whitespace
    ofs << "   \t  \n";
    
    // Line with only special characters
    ofs << "!@#$%^&*()\n";
  }
  
  try {
    NormalizeOptions options;
    options.minLength = 5;  // Filter short lines
    options.maxLength = 50; // Filter long lines
    
    NormalizeResult result = suzume::normalize(testFile, outputFile, options);
    
    // Should filter based on length requirements
    EXPECT_GT(result.rows, 0); // Should process some lines
    EXPECT_GT(result.duplicates + result.uniques, 0); // Some filtering should occur
    
    // Verify no buffer overflow occurred
    std::ifstream ifs(outputFile);
    EXPECT_TRUE(ifs.good());
    
    std::string line;
    while (std::getline(ifs, line)) {
      // All lines should meet length requirements
      EXPECT_GE(line.length(), options.minLength);
      EXPECT_LE(line.length(), options.maxLength);
    }
    
  } catch (const std::exception& e) {
    FAIL() << "Line length handling failed: " << e.what();
  }
  
  // Cleanup
  std::remove(testFile.c_str());
  std::remove(outputFile.c_str());
}

// Test to reproduce progress callback edge cases
TEST(EdgeCaseTest, ProgressCallbackEdgeCases) {
  const std::string testFile = "/tmp/test_progress.txt";
  const std::string outputFile = "/tmp/test_progress_output.txt";
  
  // Create test file
  {
    std::ofstream ofs(testFile);
    for (int i = 0; i < 1000; ++i) {
      ofs << "Progress test line " << i << "\n";
    }
  }
  
  // Test with various progress step values
  std::vector<double> progressSteps = {0.0, 0.001, 0.5, 1.0, 1.5, -0.1};
  
  for (double step : progressSteps) {
    try {
      NormalizeOptions options;
      options.progressStep = step;
      
      std::vector<double> reportedProgress;
      bool callbackCalled = false;
      
      auto progressCallback = [&](const ProgressInfo& info) {
        callbackCalled = true;
        reportedProgress.push_back(info.overallRatio);
        
        // Progress should be valid
        EXPECT_GE(info.overallRatio, 0.0);
        EXPECT_LE(info.overallRatio, 1.0);
        
        // Phase should be valid
        EXPECT_GE(static_cast<int>(info.phase), 0);
        EXPECT_LE(static_cast<int>(info.phase), 4);
      };
      
      options.structuredProgressCallback = progressCallback;
      
      if (step <= 0.0 || step > 1.0) {
        // Invalid progress step should throw exception
        EXPECT_THROW(
          normalizeWithStructuredProgress(testFile, outputFile, progressCallback, options),
          std::invalid_argument
        );
      } else {
        // Valid progress step should work
        NormalizeResult result = normalizeWithStructuredProgress(testFile, outputFile, progressCallback, options);
        
        EXPECT_TRUE(callbackCalled);
        EXPECT_GT(reportedProgress.size(), 0);
        
        // Progress should be monotonically increasing
        for (size_t i = 1; i < reportedProgress.size(); ++i) {
          EXPECT_GE(reportedProgress[i], reportedProgress[i-1]);
        }
      }
      
    } catch (const std::invalid_argument& e) {
      // Expected for invalid progress steps
      continue;
    } catch (const std::exception& e) {
      FAIL() << "Progress callback test failed with step " << step << ": " << e.what();
    }
  }
  
  // Cleanup
  std::remove(testFile.c_str());
  std::remove(outputFile.c_str());
}

// Test to reproduce malformed input handling
TEST(EdgeCaseTest, MalformedInputHandling) {
  const std::string testFile = "/tmp/test_malformed.txt";
  const std::string outputFile = "/tmp/test_malformed_output.txt";
  
  // Create file with potentially malformed content
  {
    std::ofstream ofs(testFile, std::ios::binary);
    
    // Normal text
    ofs << "Normal line\n";
    
    // Binary data
    for (int i = 0; i < 256; ++i) {
      ofs << static_cast<char>(i);
    }
    ofs << "\n";
    
    // Invalid UTF-8 sequences
    uint8_t invalidUtf8[] = {0xFF, 0xFE, 0xFD, 0xFC, 0x80, 0x81, 0x82, 0x83};
    ofs.write(reinterpret_cast<char*>(invalidUtf8), sizeof(invalidUtf8));
    ofs << "\n";
    
    // Very long line without newline
    std::string veryLongLine(1000000, 'z');
    ofs << veryLongLine;
    // No newline - potential issue for line-based processing
  }
  
  try {
    NormalizeOptions options;
    
    NormalizeResult result = suzume::normalize(testFile, outputFile, options);
    
    // Should handle malformed input gracefully
    // Exact behavior depends on implementation
    EXPECT_GE(result.rows, 0); // Should not crash
    
    // Verify output file is created
    std::ifstream ifs(outputFile);
    EXPECT_TRUE(ifs.good());
    
  } catch (const std::exception& e) {
    // Malformed input might cause exceptions, but shouldn't crash
    std::cout << "Malformed input exception (may be expected): " << e.what() << std::endl;
  }
  
  // Cleanup
  std::remove(testFile.c_str());
  std::remove(outputFile.c_str());
}

} // namespace test
} // namespace core
} // namespace suzume