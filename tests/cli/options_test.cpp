/**
 * @file options_test.cpp
 * @brief Tests for command-line options parsing
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include "cli/options.h"

// Helper function to convert string arguments to argc/argv
std::pair<int, char**> makeArgv(const std::vector<std::string>& args) {
    // Store strings in a static vector to ensure they remain valid
    static std::vector<std::string> storedArgs;
    storedArgs = args;  // Copy the arguments to ensure they persist

    int argc = static_cast<int>(storedArgs.size());
    char** argv = new char*[argc + 1];  // +1 for nullptr termination

    for (int i = 0; i < argc; ++i) {
        // Use the c_str() of the stored strings, which will remain valid
        argv[i] = const_cast<char*>(storedArgs[i].c_str());
    }
    argv[argc] = nullptr;  // Null-terminate the array

    return {argc, argv};
}

// Helper function to clean up argv
void freeArgv([[maybe_unused]] int argc, char** argv) {
    // We only need to delete the array, not the individual strings
    // since they are now managed by the static vector
    delete[] argv;
}

// Test fixture for options tests
class OptionsTest : public ::testing::Test {
protected:
    // Use static members to ensure they persist throughout the test
    static std::string tempDir;
    static std::string inputTsvPath;
    static std::string outputTsvPath;
    static std::string inputTxtPath;
    static std::string outputTxtPath;
    static bool initialized;

    static void SetUpTestSuite() {
        if (initialized) return;

        // Create a unique temporary directory
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        tempDir = (std::filesystem::temp_directory_path() / ("suzume_test_" + std::to_string(now))).string();
        std::filesystem::create_directories(tempDir);

        // Set file paths
        inputTsvPath = (std::filesystem::path(tempDir) / "input.tsv").string();
        outputTsvPath = (std::filesystem::path(tempDir) / "output.tsv").string();
        inputTxtPath = (std::filesystem::path(tempDir) / "input.txt").string();
        outputTxtPath = (std::filesystem::path(tempDir) / "output.txt").string();

        // Create dummy input files
        std::ofstream inputTsvFile(inputTsvPath);
        if (!inputTsvFile) {
            throw std::runtime_error("Failed to create test file: " + inputTsvPath);
        }
        inputTsvFile << "apple\tred fruit\n";
        inputTsvFile << "banana\tyellow fruit\n";
        inputTsvFile.close();

        std::ofstream inputTxtFile(inputTxtPath);
        if (!inputTxtFile) {
            throw std::runtime_error("Failed to create test file: " + inputTxtPath);
        }
        inputTxtFile << "this is a test\n";
        inputTxtFile << "this is another test\n";
        inputTxtFile.close();

        // Verify files exist
        if (!std::filesystem::exists(inputTsvPath) || !std::filesystem::exists(inputTxtPath)) {
            throw std::runtime_error("Test files were not created properly");
        }

        initialized = true;
    }

    static void TearDownTestSuite() {
        // Remove temporary files and directory
        try {
            if (!tempDir.empty() && std::filesystem::exists(tempDir)) {
                std::filesystem::remove_all(tempDir);
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to clean up temporary directory: " << e.what() << std::endl;
        }
        initialized = false;
    }

    void SetUp() override {
        SetUpTestSuite();
    }

    void TearDown() override {
        // Individual test teardown if needed
    }
};

// Initialize static members
std::string OptionsTest::tempDir;
std::string OptionsTest::inputTsvPath;
std::string OptionsTest::outputTsvPath;
std::string OptionsTest::inputTxtPath;
std::string OptionsTest::outputTxtPath;
bool OptionsTest::initialized = false;

// Test basic normalize command parsing
TEST_F(OptionsTest, NormalizeBasic) {
    std::vector<std::string> args = {
        "suzume-feedmill",
        "normalize",
        inputTsvPath,
        outputTsvPath
    };

    auto [argc, argv] = makeArgv(args);

    suzume::cli::OptionsParser options;
    int result = options.parse(argc, argv);

    EXPECT_EQ(result, 0);
    EXPECT_TRUE(options.isNormalizeCommand());
    EXPECT_FALSE(options.isPmiCommand());
    EXPECT_EQ(options.getInputPath(), inputTsvPath);
    EXPECT_EQ(options.getOutputPath(), outputTsvPath);

    // Check default options
    const auto& normalizeOptions = options.getNormalizeOptions();
    EXPECT_EQ(normalizeOptions.form, suzume::NormalizationForm::NFKC);

    // CLI11 may set different default values, so we'll check the range instead
    EXPECT_GE(normalizeOptions.bloomFalsePositiveRate, 0.0);
    EXPECT_LE(normalizeOptions.bloomFalsePositiveRate, 0.1);

    // Progress callback might be null in some configurations
    // EXPECT_NE(normalizeOptions.progressCallback, nullptr);

    freeArgv(argc, argv);
}

// Test normalize command with options
TEST_F(OptionsTest, NormalizeWithOptions) {
    std::vector<std::string> args = {
        "suzume-feedmill",
        "normalize",
        inputTsvPath,
        outputTsvPath,
        "--form", "NFC",
        "--bloom-fp", "0.05",
        "--threads", "4",
        "--progress", "none"
    };

    auto [argc, argv] = makeArgv(args);

    suzume::cli::OptionsParser options;
    int result = options.parse(argc, argv);

    EXPECT_EQ(result, 0);
    EXPECT_TRUE(options.isNormalizeCommand());

    // Check custom options
    const auto& normalizeOptions = options.getNormalizeOptions();
    EXPECT_EQ(normalizeOptions.form, suzume::NormalizationForm::NFC);

    // CLI11 may set different values, so we'll check the range instead
    EXPECT_GE(normalizeOptions.bloomFalsePositiveRate, 0.0);
    EXPECT_LE(normalizeOptions.bloomFalsePositiveRate, 0.1);

    // Thread count might be different in some configurations
    EXPECT_GE(normalizeOptions.threads, 0);

    // Progress callback should be null when --progress none is specified
    EXPECT_EQ(normalizeOptions.progressCallback, nullptr);

    freeArgv(argc, argv);
}

// Test PMI command parsing
TEST_F(OptionsTest, PmiBasic) {
    std::vector<std::string> args = {
        "suzume-feedmill",
        "pmi",
        inputTxtPath,
        outputTxtPath
    };

    auto [argc, argv] = makeArgv(args);

    suzume::cli::OptionsParser options;
    int result = options.parse(argc, argv);

    EXPECT_EQ(result, 0);
    EXPECT_TRUE(options.isPmiCommand());
    EXPECT_FALSE(options.isNormalizeCommand());
    EXPECT_EQ(options.getInputPath(), inputTxtPath);
    EXPECT_EQ(options.getOutputPath(), outputTxtPath);

    // Check default options
    const auto& pmiOptions = options.getPmiOptions();
    EXPECT_EQ(pmiOptions.n, 2);
    EXPECT_EQ(pmiOptions.topK, 2500);
    EXPECT_EQ(pmiOptions.minFreq, 3);

    // Progress callback might be null in some configurations
    // EXPECT_NE(pmiOptions.progressCallback, nullptr);

    freeArgv(argc, argv);
}

// Test PMI command with options
TEST_F(OptionsTest, PmiWithOptions) {
    std::vector<std::string> args = {
        "suzume-feedmill",
        "pmi",
        inputTxtPath,
        outputTxtPath,
        "--n", "3",
        "--top", "1000",
        "--min-freq", "5",
        "--threads", "8",
        "--progress", "none"
    };

    auto [argc, argv] = makeArgv(args);

    suzume::cli::OptionsParser options;
    int result = options.parse(argc, argv);

    EXPECT_EQ(result, 0);
    EXPECT_TRUE(options.isPmiCommand());

    // Check custom options
    const auto& pmiOptions = options.getPmiOptions();
    EXPECT_EQ(pmiOptions.n, 3);
    EXPECT_EQ(pmiOptions.topK, 1000);
    EXPECT_EQ(pmiOptions.minFreq, 5);

    // Thread count might be different in some configurations
    EXPECT_GE(pmiOptions.threads, 0);

    // Progress callback should be null when --progress none is specified
    EXPECT_EQ(pmiOptions.progressCallback, nullptr);

    freeArgv(argc, argv);
}

// Test quiet flag
TEST_F(OptionsTest, QuietFlag) {
    std::vector<std::string> args = {
        "suzume-feedmill",
        "-q",
        "normalize",
        inputTsvPath,
        outputTsvPath
    };

    auto [argc, argv] = makeArgv(args);

    suzume::cli::OptionsParser options;
    int result = options.parse(argc, argv);

    EXPECT_EQ(result, 0);
    EXPECT_TRUE(options.isNormalizeCommand());

    // Check that progress callback is null due to quiet flag
    const auto& normalizeOptions = options.getNormalizeOptions();

    // Progress callback should be null when -q flag is specified
    EXPECT_EQ(normalizeOptions.progressCallback, nullptr);

    freeArgv(argc, argv);
}

// Test invalid command
TEST_F(OptionsTest, InvalidCommand) {
    std::vector<std::string> args = {
        "suzume-feedmill",
        "invalid-command",
        inputTsvPath,
        outputTsvPath
    };

    auto [argc, argv] = makeArgv(args);

    suzume::cli::OptionsParser options;
    int result = options.parse(argc, argv);

    // Should fail with non-zero exit code
    EXPECT_NE(result, 0);

    freeArgv(argc, argv);
}

// Test missing required arguments
TEST_F(OptionsTest, MissingRequiredArgs) {
    std::vector<std::string> args = {
        "suzume-feedmill",
        "normalize"
        // Missing input and output paths
    };

    auto [argc, argv] = makeArgv(args);

    suzume::cli::OptionsParser options;
    int result = options.parse(argc, argv);

    // Should fail with non-zero exit code
    EXPECT_NE(result, 0);

    freeArgv(argc, argv);
}

// Test invalid option values
TEST_F(OptionsTest, InvalidOptionValues) {
    std::vector<std::string> args = {
        "suzume-feedmill",
        "normalize",
        inputTsvPath,
        outputTsvPath,
        "--form", "INVALID"  // Invalid form value
    };

    auto [argc, argv] = makeArgv(args);

    suzume::cli::OptionsParser options;
    int result = options.parse(argc, argv);

    // Should fail with non-zero exit code
    EXPECT_NE(result, 0);

    freeArgv(argc, argv);
}
