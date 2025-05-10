/**
 * @file file_io_test.cpp
 * @brief Tests for file I/O utilities
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "io/file_io.h"

namespace suzume {
namespace io {
namespace test {

class FileIOTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory if it doesn't exist
        std::filesystem::create_directories("test_data");

        // Create test input file
        std::ofstream inputFile("test_data/file_io_test_input.txt");
        inputFile << "Line 1\n";
        inputFile << "Line 2\n";
        inputFile << "Line 3\n";
        inputFile.close();
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all("test_data");
    }
};

// Test reading all lines
TEST_F(FileIOTest, ReadAllLines) {
    // Read all lines
    auto lines = TextFileReader::readAllLines("test_data/file_io_test_input.txt");

    // Check result
    ASSERT_EQ(3, lines.size());
    EXPECT_EQ("Line 1", lines[0]);
    EXPECT_EQ("Line 2", lines[1]);
    EXPECT_EQ("Line 3", lines[2]);
}

// Test reading all lines with progress callback
TEST_F(FileIOTest, ReadAllLinesWithProgress) {
    // Progress tracking
    bool callbackCalled = false;
    double lastProgress = 0.0;

    // Read all lines with progress callback
    auto lines = TextFileReader::readAllLines(
        "test_data/file_io_test_input.txt",
        [&callbackCalled, &lastProgress](double progress) {
            callbackCalled = true;
            lastProgress = progress;
        }
    );

    // Check callback was called
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(1.0, lastProgress);

    // Check result
    ASSERT_EQ(3, lines.size());
}

// Test processing line by line
TEST_F(FileIOTest, ProcessLineByLine) {
    // Collected lines
    std::vector<std::string> lines;

    // Process line by line
    TextFileReader::processLineByLine(
        "test_data/file_io_test_input.txt",
        [&lines](const std::string& line) {
            lines.push_back(line);
        }
    );

    // Check result
    ASSERT_EQ(3, lines.size());
    EXPECT_EQ("Line 1", lines[0]);
    EXPECT_EQ("Line 2", lines[1]);
    EXPECT_EQ("Line 3", lines[2]);
}

// Test processing line by line with progress callback
TEST_F(FileIOTest, ProcessLineByLineWithProgress) {
    // Collected lines
    std::vector<std::string> lines;

    // Progress tracking
    bool callbackCalled = false;
    double lastProgress = 0.0;

    // Process line by line with progress callback
    TextFileReader::processLineByLine(
        "test_data/file_io_test_input.txt",
        [&lines](const std::string& line) {
            lines.push_back(line);
        },
        [&callbackCalled, &lastProgress](double progress) {
            callbackCalled = true;
            lastProgress = progress;
        }
    );

    // Check callback was called
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(1.0, lastProgress);

    // Check result
    ASSERT_EQ(3, lines.size());
}

// Test reading file content
TEST_F(FileIOTest, ReadFileContent) {
    // Read file content
    std::string content = TextFileReader::readFileContent("test_data/file_io_test_input.txt");

    // Check result
    EXPECT_EQ("Line 1\nLine 2\nLine 3\n", content);
}

// Test writing lines
TEST_F(FileIOTest, WriteLines) {
    // Lines to write
    std::vector<std::string> lines = {
        "Output Line 1",
        "Output Line 2",
        "Output Line 3"
    };

    // Write lines
    TextFileWriter::writeLines("test_data/file_io_test_output.txt", lines);

    // Check file exists
    EXPECT_TRUE(std::filesystem::exists("test_data/file_io_test_output.txt"));

    // Read back the file
    auto readLines = TextFileReader::readAllLines("test_data/file_io_test_output.txt");

    // Check result
    ASSERT_EQ(3, readLines.size());
    EXPECT_EQ("Output Line 1", readLines[0]);
    EXPECT_EQ("Output Line 2", readLines[1]);
    EXPECT_EQ("Output Line 3", readLines[2]);
}

// Test writing lines with progress callback
TEST_F(FileIOTest, WriteLinesWithProgress) {
    // Lines to write
    std::vector<std::string> lines = {
        "Output Line 1",
        "Output Line 2",
        "Output Line 3"
    };

    // Progress tracking
    bool callbackCalled = false;
    double lastProgress = 0.0;

    // Write lines with progress callback
    TextFileWriter::writeLines(
        "test_data/file_io_test_output.txt",
        lines,
        [&callbackCalled, &lastProgress](double progress) {
            callbackCalled = true;
            lastProgress = progress;
        }
    );

    // Check callback was called
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(1.0, lastProgress);

    // Check file exists
    EXPECT_TRUE(std::filesystem::exists("test_data/file_io_test_output.txt"));
}

// Test writing content
TEST_F(FileIOTest, WriteContent) {
    // Content to write
    std::string content = "Output content\nwith multiple lines\n";

    // Write content
    TextFileWriter::writeContent("test_data/file_io_test_output.txt", content);

    // Check file exists
    EXPECT_TRUE(std::filesystem::exists("test_data/file_io_test_output.txt"));

    // Read back the file
    std::string readContent = TextFileReader::readFileContent("test_data/file_io_test_output.txt");

    // Check result
    EXPECT_EQ(content, readContent);
}

} // namespace test
} // namespace io
} // namespace suzume
