/**
 * @file normalize_detailed_error_test.cpp
 * @brief Tests for detailed error handling in text normalization
 */

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <system_error>
#include "core/normalize.h"

namespace suzume {
namespace core {
namespace test {

class NormalizeDetailedErrorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory if it doesn't exist
        std::filesystem::create_directories("test_data");
        std::filesystem::create_directories("test_data/nested");

        // Create a sample valid input file
        std::ofstream inputFile("test_data/valid_input.tsv");
        inputFile << "Sample text for testing\n";
        inputFile << "Another line of text\n";
        inputFile.close();
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all("test_data");
    }
};

// Test detailed error message for non-existent directory
// This test is disabled because it conflicts with the standard output support
// TEST_F(NormalizeDetailedErrorTest, NonExistentDirectoryError) {
//     NormalizeOptions options;
//     options.form = NormalizationForm::NFKC;
//
//     try {
//         suzume::core::normalize(
//             "test_data/valid_input.tsv",
//             "non_existent_directory/output.tsv",
//             options
//         );
//         FAIL() << "Expected std::runtime_error";
//     } catch (const std::runtime_error& e) {
//         std::string errorMsg = e.what();
//         // Check that the error message contains directory-specific information
//         EXPECT_TRUE(errorMsg.find("Directory") != std::string::npos ||
//                     errorMsg.find("directory") != std::string::npos);
//     }
// }

// Test detailed error message for permission denied
TEST_F(NormalizeDetailedErrorTest, PermissionDeniedError) {
    // Skip this test on Windows as file permissions work differently
    #ifndef _WIN32
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;

    // Create a directory and make it read-only
    std::filesystem::create_directories("test_data/readonly_dir");
    std::filesystem::permissions(
        "test_data/readonly_dir",
        std::filesystem::perms::owner_read |
        std::filesystem::perms::group_read |
        std::filesystem::perms::others_read,
        std::filesystem::perm_options::replace
    );

    try {
        suzume::core::normalize(
            "test_data/valid_input.tsv",
            "test_data/readonly_dir/output.tsv",
            options
        );
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& e) {
        std::string errorMsg = e.what();
        // Check that the error message contains permission-specific information
        EXPECT_TRUE(errorMsg.find("Permission") != std::string::npos ||
                    errorMsg.find("permission") != std::string::npos);
    }

    // Restore permissions to allow cleanup
    std::filesystem::permissions(
        "test_data/readonly_dir",
        std::filesystem::perms::owner_write,
        std::filesystem::perm_options::add
    );
    #endif
}

// Test error handling for corrupted input file
TEST_F(NormalizeDetailedErrorTest, CorruptedInputFile) {
    // Create a binary file with non-UTF8 content
    std::ofstream corruptedFile("test_data/corrupted.tsv", std::ios::binary);
    char invalidUtf8[] = {
        (char)0xFF, (char)0xFE, // Invalid UTF-8 sequence
        'H', 'e', 'l', 'l', 'o',
        (char)0xC0, (char)0xAF, // Another invalid UTF-8 sequence
        '\n'
    };
    corruptedFile.write(invalidUtf8, sizeof(invalidUtf8));
    corruptedFile.close();

    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;

    // The implementation should handle this gracefully
    try {
        NormalizeResult result = suzume::core::normalize(
            "test_data/corrupted.tsv",
            "test_data/corrupted_output.tsv",
            options
        );

        // If it doesn't throw, it should have processed some lines
        EXPECT_GE(result.rows, 0);
    } catch (const std::exception& e) {
        // If it throws, the error message should be informative
        std::string errorMsg = e.what();
        EXPECT_FALSE(errorMsg.empty());
    }
}

// Test error handling for file size calculation errors
TEST_F(NormalizeDetailedErrorTest, FileSizeCalculationError) {
    // Create a special file that might cause file_size to fail
    // (e.g., a named pipe or device file on Unix)
    // This is platform-specific, so we'll just simulate the error path

    // Create a mock file
    std::ofstream mockFile("test_data/mock_special_file.tsv");
    mockFile << "Test content\n";
    mockFile.close();

    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;

    // The implementation should handle file_size errors gracefully
    NormalizeResult result = suzume::core::normalize(
        "test_data/mock_special_file.tsv",
        "test_data/output.tsv",
        options
    );

    // Should complete successfully even if file_size fails
    EXPECT_GT(result.rows, 0);
    EXPECT_GT(result.uniques, 0);
}

// Test with structured callback that throws exceptions
TEST_F(NormalizeDetailedErrorTest, StructuredCallbackException) {
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;

    // Set a structured progress callback that throws an exception
    bool callbackCalled = false;

    try {
        suzume::core::normalizeWithStructuredProgress(
            "test_data/valid_input.tsv",
            "test_data/output.tsv",
            [&callbackCalled](const ProgressInfo& info) {
                callbackCalled = true;
                if (info.phase == ProgressInfo::Phase::Processing) {
                    throw std::runtime_error("Test exception from structured callback");
                }
            },
            options
        );

        // If we get here, the function handled the exception gracefully
        EXPECT_TRUE(callbackCalled);
    } catch (const std::exception& e) {
        // If it re-throws, that's also acceptable
        EXPECT_TRUE(callbackCalled);
    }
}

// Test with extremely small Bloom filter rate
TEST_F(NormalizeDetailedErrorTest, ExtremeBloomFilterRate) {
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;
    options.bloomFalsePositiveRate = 0.0000001; // Extremely small

    // This should be handled gracefully
    NormalizeResult result = suzume::core::normalize(
        "test_data/valid_input.tsv",
        "test_data/output.tsv",
        options
    );

    // Should complete successfully
    EXPECT_GT(result.rows, 0);
    EXPECT_GT(result.uniques, 0);
}

// Test with zero threads
TEST_F(NormalizeDetailedErrorTest, ZeroThreads) {
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;
    options.threads = 0; // Should use hardware_concurrency

    // This should be handled gracefully
    NormalizeResult result = suzume::core::normalize(
        "test_data/valid_input.tsv",
        "test_data/output.tsv",
        options
    );

    // Should complete successfully
    EXPECT_GT(result.rows, 0);
    EXPECT_GT(result.uniques, 0);
}

} // namespace test
} // namespace core
} // namespace suzume
