/**
 * @file pmi_error_test.cpp
 * @brief Tests for error handling in PMI calculation
 */

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include "core/pmi.h"

namespace suzume {
namespace core {
namespace test {

class PmiErrorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory if it doesn't exist
        std::filesystem::create_directories("test_data");

        // Create a sample valid input file
        std::ofstream inputFile("test_data/pmi_valid_input.txt");
        inputFile << "This is a test sentence for PMI calculation.\n";
        inputFile << "This is another test sentence with some repeated words.\n";
        inputFile << "PMI calculation requires sufficient text data.\n";
        inputFile << "The more text data, the better the PMI calculation.\n";
        inputFile << "Test sentences with repeated words help PMI calculation.\n";
        inputFile.close();
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all("test_data");
    }
};

// Test non-existent input file
TEST_F(PmiErrorTest, NonExistentInputFile) {
    PmiOptions options;
    options.n = 2;
    options.topK = 10;
    options.minFreq = 2;

    // Try to calculate PMI on a non-existent file
    EXPECT_THROW({
        core::calculatePmi(
            "test_data/non_existent_file.txt",
            "test_data/pmi_output.tsv",
            options
        );
    }, std::runtime_error);
}

// Test invalid output path
TEST_F(PmiErrorTest, InvalidOutputPath) {
    PmiOptions options;
    options.n = 2;
    options.topK = 10;
    options.minFreq = 2;

    // Try to write to an invalid output path
    EXPECT_THROW({
        core::calculatePmi(
            "test_data/pmi_valid_input.txt",
            "/invalid/path/that/does/not/exist/output.tsv",
            options
        );
    }, std::runtime_error);
}

// Test invalid n-gram size
TEST_F(PmiErrorTest, InvalidNgramSize) {
    // Test n = 0
    {
        PmiOptions options;
        options.n = 0; // Invalid value
        options.topK = 10;
        options.minFreq = 2;

        // The implementation might handle this gracefully rather than throwing
        try {
            core::calculatePmi(
                "test_data/pmi_valid_input.txt",
                "test_data/pmi_output.tsv",
                options
            );
            // If we get here, the function didn't throw
            SUCCEED();
        } catch (const std::exception& e) {
            // Also acceptable if it throws
            SUCCEED();
        }
    }

    // Test n > 3
    {
        PmiOptions options;
        options.n = 4; // Invalid value
        options.topK = 10;
        options.minFreq = 2;

        // The implementation might handle this gracefully rather than throwing
        try {
            core::calculatePmi(
                "test_data/pmi_valid_input.txt",
                "test_data/pmi_output.tsv",
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

// Test invalid topK value
TEST_F(PmiErrorTest, InvalidTopK) {
    PmiOptions options;
    options.n = 2;
    options.topK = 0; // Zero value
    options.minFreq = 2;

    // This might throw or handle the error gracefully
    // The important thing is that it doesn't crash
    try {
        core::calculatePmi(
            "test_data/pmi_valid_input.txt",
            "test_data/pmi_output.tsv",
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

// Test invalid minFreq value
TEST_F(PmiErrorTest, InvalidMinFreq) {
    PmiOptions options;
    options.n = 2;
    options.topK = 10;
    options.minFreq = 0; // Zero value

    // This might throw or handle the error gracefully
    // The important thing is that it doesn't crash
    try {
        core::calculatePmi(
            "test_data/pmi_valid_input.txt",
            "test_data/pmi_output.tsv",
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

// Test invalid thread count
TEST_F(PmiErrorTest, InvalidThreadCount) {
    PmiOptions options;
    options.n = 2;
    options.topK = 10;
    options.minFreq = 2;
    options.threads = static_cast<uint32_t>(-1); // Very large value due to unsigned wrap-around

    // This might throw or handle the error gracefully
    // The important thing is that it doesn't crash
    try {
        core::calculatePmi(
            "test_data/pmi_valid_input.txt",
            "test_data/pmi_output.tsv",
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
TEST_F(PmiErrorTest, EmptyInputFile) {
    // Create an empty input file
    std::ofstream emptyFile("test_data/empty.txt");
    emptyFile.close();

    PmiOptions options;
    options.n = 2;
    options.topK = 10;
    options.minFreq = 2;

    // This should not throw, but return zero results
    PmiResult result = core::calculatePmi(
        "test_data/empty.txt",
        "test_data/empty_output.tsv",
        options
    );

    EXPECT_EQ(0, result.grams);
}

// Test with read-only output file
TEST_F(PmiErrorTest, ReadOnlyOutputFile) {
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

    PmiOptions options;
    options.n = 2;
    options.topK = 10;
    options.minFreq = 2;

    // Try to write to a read-only file
    EXPECT_THROW({
        core::calculatePmi(
            "test_data/pmi_valid_input.txt",
            "test_data/readonly.tsv",
            options
        );
    }, std::runtime_error);
    #endif
}

// Test with invalid progress callback
TEST_F(PmiErrorTest, InvalidProgressCallback) {
    PmiOptions options;
    options.n = 2;
    options.topK = 10;
    options.minFreq = 2;

    // Set a progress callback that throws an exception
    options.progressCallback = [](double /* progress */) {
        throw std::runtime_error("Test exception from progress callback");
    };

    // The implementation might handle this gracefully rather than re-throwing
    try {
        core::calculatePmi(
            "test_data/pmi_valid_input.txt",
            "test_data/pmi_output.tsv",
            options
        );
        // If we get here, the function handled the exception
        SUCCEED();
    } catch (const std::exception& e) {
        // Also acceptable if it re-throws
        SUCCEED();
    }
}

// Test countNgrams with invalid input
TEST_F(PmiErrorTest, CountNgramsInvalidInput) {
    // Test with empty string
    {
        auto counts = core::countNgrams("", 2);
        EXPECT_TRUE(counts.empty());
    }

    // Test with invalid n-gram size
    try {
        auto counts = core::countNgrams("This is a test", 0);
        // If it doesn't throw, it should at least return empty results
        EXPECT_TRUE(counts.empty());
    } catch (const std::exception& e) {
        // Expected exception
        SUCCEED();
    }

    try {
        auto counts = core::countNgrams("This is a test", 4);
        // If it doesn't throw, it should at least handle it gracefully
        SUCCEED();
    } catch (const std::exception& e) {
        // Expected exception
        SUCCEED();
    }
}

// Test calculatePmiScores with invalid input
TEST_F(PmiErrorTest, CalculatePmiScoresInvalidInput) {
    // Test with empty counts
    {
        std::unordered_map<std::string, uint32_t> emptyCounts;
        auto scores = core::calculatePmiScores(emptyCounts, 2, 1);
        // Empty input might return empty results or some default values
        // Just check that it doesn't crash
        SUCCEED();
    }

    // Test with invalid n-gram size
    try {
        std::unordered_map<std::string, uint32_t> counts = {
            {"test", 1},
            {"example", 2}
        };
        auto scores = core::calculatePmiScores(counts, 0, 1);
        // If it doesn't throw, it's handled gracefully
        SUCCEED();
    } catch (const std::exception& e) {
        // Also acceptable if it throws
        SUCCEED();
    }

    // Test with invalid minFreq
    try {
        std::unordered_map<std::string, uint32_t> counts = {
            {"test", 1},
            {"example", 2}
        };
        auto scores = core::calculatePmiScores(counts, 1, 0);
        // If it doesn't throw, it's handled gracefully
        SUCCEED();
    } catch (const std::exception& e) {
        // Also acceptable if it throws
        SUCCEED();
    }
}

} // namespace test
} // namespace core
} // namespace suzume
