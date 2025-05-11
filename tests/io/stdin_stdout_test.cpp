/**
 * @file stdin_stdout_test.cpp
 * @brief Tests for stdin/stdout functionality
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "io/file_io.h"

// Mock classes for stdin/stdout testing
class StdinRedirector {
private:
    std::streambuf* originalCin;
    std::istringstream mockInput;

public:
    StdinRedirector(const std::string& input) : mockInput(input) {
        originalCin = std::cin.rdbuf();
        std::cin.rdbuf(mockInput.rdbuf());
    }

    ~StdinRedirector() {
        std::cin.rdbuf(originalCin);
    }
};

class StdoutRedirector {
private:
    std::streambuf* originalCout;
    std::ostringstream mockOutput;

public:
    StdoutRedirector() {
        originalCout = std::cout.rdbuf();
        std::cout.rdbuf(mockOutput.rdbuf());
    }

    ~StdoutRedirector() {
        std::cout.rdbuf(originalCout);
    }

    std::string getOutput() const {
        return mockOutput.str();
    }
};

namespace suzume {
namespace io {
namespace test {

class StdinStdoutTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory if it doesn't exist
        std::filesystem::create_directories("test_data");

        // Create test input file
        std::ofstream inputFile("test_data/stdin_test_input.txt");
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

// Test reading from stdin
TEST_F(StdinStdoutTest, ReadFromStdin) {
    // Setup mock stdin
    StdinRedirector redirector("Line 1\nLine 2\nLine 3\n");

    // Read all lines from stdin
    auto lines = TextFileReader::readAllLines("-");

    // Check result
    ASSERT_EQ(3, lines.size());
    EXPECT_EQ("Line 1", lines[0]);
    EXPECT_EQ("Line 2", lines[1]);
    EXPECT_EQ("Line 3", lines[2]);
}

// Test reading from stdin with progress callback
TEST_F(StdinStdoutTest, ReadFromStdinWithProgress) {
    // Setup mock stdin
    StdinRedirector redirector("Line 1\nLine 2\nLine 3\n");

    // Progress tracking
    bool callbackCalled = false;
    double lastProgress = 0.0;

    // Read all lines from stdin with progress callback
    auto lines = TextFileReader::readAllLines(
        "-",
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

// Test processing stdin line by line
TEST_F(StdinStdoutTest, ProcessStdinLineByLine) {
    // Setup mock stdin
    StdinRedirector redirector("Line 1\nLine 2\nLine 3\n");

    // Collected lines
    std::vector<std::string> lines;

    // Process line by line from stdin
    TextFileReader::processLineByLine(
        "-",
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

// Test reading file content from stdin
TEST_F(StdinStdoutTest, ReadFileContentFromStdin) {
    // Setup mock stdin
    StdinRedirector redirector("Line 1\nLine 2\nLine 3\n");

    // Read file content from stdin
    std::string content = TextFileReader::readFileContent("-");

    // Check result
    EXPECT_EQ("Line 1\nLine 2\nLine 3\n", content);
}

// Test writing to stdout
TEST_F(StdinStdoutTest, WriteToStdout) {
    // Setup mock stdout
    StdoutRedirector redirector;

    // Lines to write
    std::vector<std::string> lines = {
        "Output Line 1",
        "Output Line 2",
        "Output Line 3"
    };

    // Write lines to stdout
    TextFileWriter::writeLines("-", lines);

    // Check result
    std::string output = redirector.getOutput();
    EXPECT_EQ("Output Line 1\nOutput Line 2\nOutput Line 3\n", output);
}

// Test writing to stdout with progress callback
TEST_F(StdinStdoutTest, WriteToStdoutWithProgress) {
    // Setup mock stdout
    StdoutRedirector redirector;

    // Lines to write
    std::vector<std::string> lines = {
        "Output Line 1",
        "Output Line 2",
        "Output Line 3"
    };

    // Progress tracking
    bool callbackCalled = false;
    double lastProgress = 0.0;

    // Write lines to stdout with progress callback
    TextFileWriter::writeLines(
        "-",
        lines,
        [&callbackCalled, &lastProgress](double progress) {
            callbackCalled = true;
            lastProgress = progress;
        }
    );

    // Check callback was called
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(1.0, lastProgress);

    // Check result
    std::string output = redirector.getOutput();
    EXPECT_EQ("Output Line 1\nOutput Line 2\nOutput Line 3\n", output);
}

// Test writing content to stdout
TEST_F(StdinStdoutTest, WriteContentToStdout) {
    // Setup mock stdout
    StdoutRedirector redirector;

    // Content to write
    std::string content = "Output content\nwith multiple lines\n";

    // Write content to stdout
    TextFileWriter::writeContent("-", content);

    // Check result
    std::string output = redirector.getOutput();
    EXPECT_EQ(content, output);
}

} // namespace test
} // namespace io
} // namespace suzume
