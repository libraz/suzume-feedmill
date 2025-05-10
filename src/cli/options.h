/**
 * @file options.h
 * @brief Command-line options handling for suzume-feedmill
 */

#pragma once

#include <string>
#include <functional>
#include <CLI/CLI.hpp>
#include "core/normalize.h"
#include "core/pmi.h"
#include "core/word_extraction.h"

namespace suzume {
namespace cli {

/**
 * @brief Progress output format
 */
enum class ProgressFormat {
    TTY,   ///< Terminal-friendly progress updates
    JSON,  ///< JSON-formatted progress updates
    NONE   ///< No progress output
};

/**
 * @brief Command-line options parser for suzume-feedmill
 */
class OptionsParser {
public:
    /**
     * @brief Constructor
     */
    OptionsParser();

    /**
     * @brief Parse command-line arguments
     *
     * @param argc Argument count
     * @param argv Argument values
     * @return int Exit code (0 for success, non-zero for error)
     */
    int parse(int argc, char* argv[]);

    /**
     * @brief Get the input file path
     *
     * @return const std::string& Input file path
     */
    const std::string& getInputPath() const;

    /**
     * @brief Get the output file path
     *
     * @return const std::string& Output file path
     */
    const std::string& getOutputPath() const;

    /**
     * @brief Get the original text path (for word extraction)
     *
     * @return const std::string& Original text path
     */
    const std::string& getOriginalTextPath() const;

    /**
     * @brief Get the normalize options
     *
     * @return const suzume::NormalizeOptions& Normalize options
     */
    const suzume::NormalizeOptions& getNormalizeOptions() const;

    /**
     * @brief Get the PMI options
     *
     * @return const suzume::PmiOptions& PMI options
     */
    const suzume::PmiOptions& getPmiOptions() const;

    /**
     * @brief Get the word extraction options
     *
     * @return const suzume::WordExtractionOptions& Word extraction options
     */
    const suzume::WordExtractionOptions& getWordExtractionOptions() const;

    /**
     * @brief Check if normalize command was selected
     *
     * @return true If normalize command was selected
     * @return false Otherwise
     */
    bool isNormalizeCommand() const;

    /**
     * @brief Check if PMI command was selected
     *
     * @return true If PMI command was selected
     * @return false Otherwise
     */
    bool isPmiCommand() const;

    /**
     * @brief Check if word-extract command was selected
     *
     * @return true If word-extract command was selected
     * @return false Otherwise
     */
    bool isWordExtractCommand() const;

    /**
     * @brief Get the version string
     *
     * @return std::string Version string
     */
    static std::string getVersion();

private:
    // CLI11 app
    CLI::App app{"suzume-feedmill - Grind the feed, sharpen the tokens."};

    // Subcommands
    CLI::App* normalizeCommand{nullptr};
    CLI::App* pmiCommand{nullptr};
    CLI::App* wordExtractCommand{nullptr};

    // Input/output paths
    std::string inputPath;
    std::string outputPath;
    std::string originalTextPath;

    // Options
    suzume::NormalizeOptions normalizeOptions;
    suzume::PmiOptions pmiOptions;
    suzume::WordExtractionOptions wordExtractionOptions;

    // Progress format
    ProgressFormat normalizeProgressFormat{ProgressFormat::TTY};
    ProgressFormat pmiProgressFormat{ProgressFormat::TTY};
    ProgressFormat wordExtractProgressFormat{ProgressFormat::TTY};

    // Setup methods
    void setupNormalizeCommand();
    void setupPmiCommand();
    void setupWordExtractCommand();
    void setupGlobalOptions();

    // Progress callback functions
    static void ttyProgressCallback(double ratio);
    static void jsonProgressCallback(double ratio);
};

} // namespace cli
} // namespace suzume
