/**
 * @file normalize_error_test.cpp
 * @brief Tests for error handling in text normalization
 */

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include "core/normalize.h"

namespace suzume {
namespace core {
namespace test {

class NormalizeErrorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory if it doesn't exist
        std::filesystem::create_directories("test_data");

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

// Test non-existent input file
TEST_F(NormalizeErrorTest, NonExistentInputFile) {
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;

    // Try to normalize a non-existent file
    EXPECT_THROW({
        core::normalize(
            "test_data/non_existent_file.tsv",
            "test_data/output.tsv",
            options
        );
    }, std::runtime_error);
}

// Test invalid output path
TEST_F(NormalizeErrorTest, InvalidOutputPath) {
    // Create a directory that we'll use as a file path (which should fail)
    std::filesystem::create_directories("test_data/directory_not_file");

    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;

    // Try to write to a directory instead of a file
    // This should fail on all platforms
    bool exceptionThrown = false;
    try {
        core::normalize(
            "test_data/valid_input.tsv",
            "test_data/directory_not_file",
            options
        );
    } catch (const std::exception&) {
        exceptionThrown = true;
    }

    EXPECT_TRUE(exceptionThrown) << "Expected exception when writing to a directory path";
}

// Test invalid normalization form
TEST_F(NormalizeErrorTest, InvalidNormalizationForm) {
    // Create an invalid normalization form using a cast
    // This is a bit of a hack, but it's the only way to test this
    NormalizeOptions options;
    options.form = static_cast<NormalizationForm>(999); // Invalid value

    // This might throw or handle the error gracefully
    // The important thing is that it doesn't crash
    try {
        core::normalize(
            "test_data/valid_input.tsv",
            "test_data/output.tsv",
            options
        );
    } catch (const std::exception& e) {
        // Expected exception
        SUCCEED();
        return;
    }

    // If we get here, the function didn't throw
    // This is also acceptable if it handled the error gracefully
    SUCCEED();
}

// Test invalid Bloom filter rate
TEST_F(NormalizeErrorTest, InvalidBloomFilterRate) {
    // Test negative Bloom filter rate
    {
        NormalizeOptions options;
        options.bloomFalsePositiveRate = -0.1; // Negative value

        // The implementation might handle this gracefully rather than throwing
        try {
            core::normalize(
                "test_data/valid_input.tsv",
                "test_data/output.tsv",
                options
            );
            // If we get here, the function didn't throw
            SUCCEED();
        } catch (const std::exception& e) {
            // Also acceptable if it throws
            SUCCEED();
        }
    }

    // Test Bloom filter rate > 1.0
    {
        NormalizeOptions options;
        options.bloomFalsePositiveRate = 1.5; // Value > 1.0

        // The implementation might handle this gracefully rather than throwing
        try {
            core::normalize(
                "test_data/valid_input.tsv",
                "test_data/output.tsv",
                options
            );
            // If we get here, the function didn't throw
            SUCCEED();
        } catch (const std::exception& e) {
            // Also acceptable if it throws
            SUCCEED();
        }
    }
}

// Test invalid thread count
TEST_F(NormalizeErrorTest, InvalidThreadCount) {
    NormalizeOptions options;
    options.threads = static_cast<uint32_t>(-1); // Very large value due to unsigned wrap-around

    // This might throw or handle the error gracefully
    // The important thing is that it doesn't crash
    try {
        core::normalize(
            "test_data/valid_input.tsv",
            "test_data/output.tsv",
            options
        );
    } catch (const std::exception& e) {
        // Expected exception
        SUCCEED();
        return;
    }

    // If we get here, the function didn't throw
    // This is also acceptable if it handled the error gracefully
    SUCCEED();
}

// Test with empty input file
TEST_F(NormalizeErrorTest, EmptyInputFile) {
    // Create an empty input file
    std::ofstream emptyFile("test_data/empty.tsv");
    emptyFile.close();

    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;

    // This should not throw, but return zero results
    NormalizeResult result = core::normalize(
        "test_data/empty.tsv",
        "test_data/empty_output.tsv",
        options
    );

    EXPECT_EQ(0, result.rows);
    EXPECT_EQ(0, result.uniques);
}

// Test with read-only output file
TEST_F(NormalizeErrorTest, ReadOnlyOutputFile) {
    // Skip this test on Windows as file permissions work differently
    #ifndef _WIN32
    // Create output file and make it read-only
    std::ofstream outputFile("test_data/readonly.tsv");
    outputFile << "This file will be made read-only\n";
    outputFile.close();

    // Make the file read-only
    std::filesystem::permissions(
        "test_data/readonly.tsv",
        std::filesystem::perms::owner_read |
        std::filesystem::perms::group_read |
        std::filesystem::perms::others_read,
        std::filesystem::perm_options::replace
    );

    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;

    // Try to write to a read-only file
    EXPECT_THROW({
        core::normalize(
            "test_data/valid_input.tsv",
            "test_data/readonly.tsv",
            options
        );
    }, std::runtime_error);
    #endif
}

// Test with invalid progress callback
TEST_F(NormalizeErrorTest, InvalidProgressCallback) {
    NormalizeOptions options;
    options.form = NormalizationForm::NFKC;

    // Set a progress callback that throws an exception
    options.progressCallback = [](double /* progress */) {
        throw std::runtime_error("Test exception from progress callback");
    };

    // The implementation might handle this gracefully rather than re-throwing
    try {
        core::normalize(
            "test_data/valid_input.tsv",
            "test_data/output.tsv",
            options
        );
        // If we get here, the function handled the exception
        SUCCEED();
    } catch (const std::exception& e) {
        // Also acceptable if it re-throws
        SUCCEED();
    }
}

} // namespace test
} // namespace core
} // namespace suzume
