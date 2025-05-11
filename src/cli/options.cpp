/**
 * @file options.cpp
 * @brief Implementation of command-line options handling for suzume-feedmill
 */

#include "options.h"
#include <iostream>
#include <chrono>
#include <sstream>

namespace suzume {
namespace cli {

// Static progress callback functions
void OptionsParser::ttyProgressCallback(double ratio) {
    static int lastPercent = -1;
    int percent = static_cast<int>(ratio * 100);

    if (percent != lastPercent) {
        std::cerr << "\rProgress: " << percent << "%" << std::flush;
        lastPercent = percent;

        if (percent >= 100) {
            std::cerr << std::endl;
        }
    }
}

void OptionsParser::jsonProgressCallback(double ratio) {
    static int lastPercent = -1;
    int percent = static_cast<int>(ratio * 100);

    if (percent != lastPercent) {
        std::cerr << "{\"progress\":" << percent << "}" << std::endl;
        lastPercent = percent;
    }
}

// ETA calculation helper
double OptionsParser::calculateEta(double ratio) {
    static auto startTime = std::chrono::steady_clock::now();
    static double lastRatio = 0.0;
    static double lastEta = 0.0;

    // Skip if ratio is 0 (just starting) or 1 (already finished)
    if (ratio <= 0.0 || ratio >= 1.0) {
        lastRatio = ratio;
        return 0.0;
    }

    // Calculate elapsed time
    auto now = std::chrono::steady_clock::now();
    double elapsedSeconds = std::chrono::duration<double>(now - startTime).count();

    // Calculate ETA
    double eta = 0.0;
    if (ratio > 0.0) {
        // Estimate total time based on current progress
        double totalEstimatedTime = elapsedSeconds / ratio;
        // Calculate remaining time
        eta = totalEstimatedTime - elapsedSeconds;
    }

    // Smooth ETA to avoid jumps
    if (lastRatio > 0.0 && lastEta > 0.0) {
        // Apply simple exponential smoothing
        double alpha = 0.2; // Smoothing factor
        eta = alpha * eta + (1 - alpha) * lastEta;
    }

    // Update last values
    lastRatio = ratio;
    lastEta = eta;

    return eta;
}

// Progress callback with ETA
void OptionsParser::ttyProgressCallbackWithEta(double ratio) {
    static int lastPercent = -1;
    int percent = static_cast<int>(ratio * 100);

    if (percent != lastPercent) {
        // Calculate ETA
        double eta = calculateEta(ratio);

        // Format ETA
        std::string etaStr;
        if (ratio <= 0.0 || ratio >= 1.0) {
            etaStr = "";
        } else {
            int etaMinutes = static_cast<int>(eta) / 60;
            int etaSeconds = static_cast<int>(eta) % 60;

            std::stringstream ss;
            ss << " ETA: ";
            if (etaMinutes > 0) {
                ss << etaMinutes << "m ";
            }
            ss << etaSeconds << "s";
            etaStr = ss.str();
        }

        // Display progress with ETA
        std::cerr << "\rProgress: " << percent << "%" << etaStr << std::flush;
        lastPercent = percent;

        if (percent >= 100) {
            std::cerr << std::endl;
        }
    }
}

void OptionsParser::jsonProgressCallbackWithEta(double ratio) {
    static int lastPercent = -1;
    int percent = static_cast<int>(ratio * 100);

    if (percent != lastPercent) {
        // Calculate ETA
        double eta = calculateEta(ratio);

        // Output JSON with ETA
        std::cerr << "{\"progress\":" << percent << ", \"eta\":" << eta << "}" << std::endl;
        lastPercent = percent;
    }
}

OptionsParser::OptionsParser() {
    // Setup commands and options
    setupNormalizeCommand();
    setupPmiCommand();
    setupWordExtractCommand();
    setupGlobalOptions();
}

void OptionsParser::setupNormalizeCommand() {
    // Add normalize command
    normalizeCommand = app.add_subcommand("normalize", "Normalize and deduplicate text data");

    // Add input/output options
    normalizeCommand->add_option("input", inputPath, "Input file path (use - for stdin)")
        ->required()
        ->check([](const std::string& path) {
            // Allow "-" for stdin
            if (path == "-") return std::string();
            // Otherwise check if file exists
            return CLI::ExistingFile(path);
        });

    normalizeCommand->add_option("output", outputPath, "Output file path (use - for stdout)")
        ->required();

    // Add normalize-specific options
    // Store form as an enum directly instead of string
    normalizeOptions.form = suzume::NormalizationForm::NFKC; // Default value
    std::vector<std::pair<std::string, suzume::NormalizationForm>> form_map = {
        {"NFKC", suzume::NormalizationForm::NFKC},
        {"NFC", suzume::NormalizationForm::NFC}
    };
    normalizeCommand->add_option("--form", normalizeOptions.form, "Normalization form (NFKC or NFC)")
        ->transform(CLI::CheckedTransformer(form_map, CLI::ignore_case));

    normalizeCommand->add_option("--bloom-fp", normalizeOptions.bloomFalsePositiveRate,
                               "Bloom filter false positive rate")
        ->check(CLI::Range(0.0001, 0.1));

    normalizeCommand->add_option("--threads", normalizeOptions.threads,
                               "Number of threads (0 = auto)");

    // Add sample option
    normalizeCommand->add_option("--sample", sampleSize,
                               "Sample N lines randomly from input")
        ->check(CLI::PositiveNumber);

    // Add line length filter options
    normalizeCommand->add_option("--min-length", normalizeOptions.minLength,
                               "Minimum line length (0 = no minimum)")
        ->check(CLI::NonNegativeNumber);

    normalizeCommand->add_option("--max-length", normalizeOptions.maxLength,
                               "Maximum line length (0 = no maximum)")
        ->check(CLI::NonNegativeNumber);

    // Store progress format as an enum directly
    normalizeProgressFormat = ProgressFormat::TTY; // Default value
    std::vector<std::pair<std::string, ProgressFormat>> progress_map = {
        {"tty", ProgressFormat::TTY},
        {"json", ProgressFormat::JSON},
        {"none", ProgressFormat::NONE}
    };
    normalizeCommand->add_option("--progress", normalizeProgressFormat,
                               "Progress display mode (tty, json, or none)")
        ->transform(CLI::CheckedTransformer(progress_map, CLI::ignore_case));

    // Add callbacks for post-processing - using direct enum values
    normalizeCommand->callback([this]() {
        // Set progress callback based on format
        switch (normalizeProgressFormat) {
            case ProgressFormat::TTY:
                normalizeOptions.progressCallback = ttyProgressCallbackWithEta;
                break;
            case ProgressFormat::JSON:
                normalizeOptions.progressCallback = jsonProgressCallbackWithEta;
                break;
            case ProgressFormat::NONE:
                normalizeOptions.progressCallback = nullptr;
                break;
        }
    });
}

void OptionsParser::setupPmiCommand() {
    // Add PMI command
    pmiCommand = app.add_subcommand("pmi", "Calculate PMI (Pointwise Mutual Information)");

    // Add input/output options
    pmiCommand->add_option("input", inputPath, "Input file path (use - for stdin)")
        ->required()
        ->check([](const std::string& path) {
            // Allow "-" for stdin
            if (path == "-") return std::string();
            // Otherwise check if file exists
            return CLI::ExistingFile(path);
        });

    pmiCommand->add_option("output", outputPath, "Output file path (use - for stdout)")
        ->required();

    // Add PMI-specific options
    pmiCommand->add_option("--n", pmiOptions.n, "N-gram size (1, 2, or 3)")
        ->check(CLI::Range(1, 3));

    pmiCommand->add_option("--top", pmiOptions.topK, "Number of top results")
        ->check(CLI::Range(1, 100000));

    pmiCommand->add_option("--min-freq", pmiOptions.minFreq, "Minimum frequency threshold")
        ->check(CLI::Range(1, 1000));

    pmiCommand->add_option("--threads", pmiOptions.threads, "Number of threads (0 = auto)");

    // Store progress format as an enum directly
    pmiProgressFormat = ProgressFormat::TTY; // Default value
    std::vector<std::pair<std::string, ProgressFormat>> progress_map = {
        {"tty", ProgressFormat::TTY},
        {"json", ProgressFormat::JSON},
        {"none", ProgressFormat::NONE}
    };
    pmiCommand->add_option("--progress", pmiProgressFormat,
                          "Progress display mode (tty, json, or none)")
        ->transform(CLI::CheckedTransformer(progress_map, CLI::ignore_case));

    // Add callbacks for post-processing - using direct enum values
    pmiCommand->callback([this]() {
        // Set progress callback based on format
        switch (pmiProgressFormat) {
            case ProgressFormat::TTY:
                pmiOptions.progressCallback = ttyProgressCallbackWithEta;
                break;
            case ProgressFormat::JSON:
                pmiOptions.progressCallback = jsonProgressCallbackWithEta;
                break;
            case ProgressFormat::NONE:
                pmiOptions.progressCallback = nullptr;
                break;
        }
    });
}

void OptionsParser::setupWordExtractCommand() {
    // Add word-extract command
    wordExtractCommand = app.add_subcommand("word-extract", "Extract unknown words from PMI results");

    // Add input/output options
    wordExtractCommand->add_option("pmi-results", inputPath, "PMI results file path")
        ->required()
        ->check(CLI::ExistingFile);

    wordExtractCommand->add_option("original-text", originalTextPath, "Original text file path")
        ->required()
        ->check(CLI::ExistingFile);

    wordExtractCommand->add_option("output", outputPath, "Output file path")
        ->required();

    // Add word extraction specific options
    wordExtractCommand->add_option("--min-pmi", wordExtractionOptions.minPmiScore,
                                  "Minimum PMI score")
        ->check(CLI::Range(0.0, 100.0));

    wordExtractCommand->add_option("--max-length", wordExtractionOptions.maxCandidateLength,
                                  "Maximum candidate length")
        ->check(CLI::Range(1, 100));

    wordExtractCommand->add_option("--min-length", wordExtractionOptions.minLength,
                                  "Minimum candidate length")
        ->check(CLI::Range(1, 100));

    wordExtractCommand->add_option("--top", wordExtractionOptions.topK,
                                  "Number of top results")
        ->check(CLI::Range(1, 100000));

    wordExtractCommand->add_option("--language", wordExtractionOptions.languageCode,
                                  "Language code (e.g., ja, en, zh)");

    // Use callback_t for flags
    auto noVerifyCallback = [this](int count) {
        if (count > 0) wordExtractionOptions.verifyInOriginalText = false;
    };
    wordExtractCommand->add_flag("--no-verify", noVerifyCallback, "Disable verification in original text");

    auto noContextCallback = [this](int count) {
        if (count > 0) wordExtractionOptions.useContextualAnalysis = false;
    };
    wordExtractCommand->add_flag("--no-context", noContextCallback, "Disable contextual analysis");

    auto noStatisticalCallback = [this](int count) {
        if (count > 0) wordExtractionOptions.useStatisticalValidation = false;
    };
    wordExtractCommand->add_flag("--no-statistical", noStatisticalCallback, "Disable statistical validation");

    auto useDictionaryCallback = [this](int count) {
        if (count > 0) wordExtractionOptions.useDictionaryLookup = true;
    };
    wordExtractCommand->add_flag("--use-dictionary", useDictionaryCallback, "Enable dictionary lookup");

    wordExtractCommand->add_option("--dictionary", wordExtractionOptions.dictionaryPath,
                                  "Dictionary file path");

    auto noSubstringsCallback = [this](int count) {
        if (count > 0) wordExtractionOptions.removeSubstrings = false;
    };
    wordExtractCommand->add_flag("--no-substrings", noSubstringsCallback, "Disable substring removal");

    auto noOverlappingCallback = [this](int count) {
        if (count > 0) wordExtractionOptions.removeOverlapping = false;
    };
    wordExtractCommand->add_flag("--no-overlapping", noOverlappingCallback, "Disable overlapping removal");

    auto noLanguageRulesCallback = [this](int count) {
        if (count > 0) wordExtractionOptions.useLanguageSpecificRules = false;
    };
    wordExtractCommand->add_flag("--no-language-rules", noLanguageRulesCallback, "Disable language-specific rules");

    wordExtractCommand->add_option("--threads", wordExtractionOptions.threads,
                                  "Number of threads (0 = auto)");

    // Store progress format as an enum directly
    wordExtractProgressFormat = ProgressFormat::TTY; // Default value
    std::vector<std::pair<std::string, ProgressFormat>> progress_map = {
        {"tty", ProgressFormat::TTY},
        {"json", ProgressFormat::JSON},
        {"none", ProgressFormat::NONE}
    };
    wordExtractCommand->add_option("--progress", wordExtractProgressFormat,
                                  "Progress display mode (tty, json, or none)")
        ->transform(CLI::CheckedTransformer(progress_map, CLI::ignore_case));

    // Add callbacks for post-processing - using direct enum values
    wordExtractCommand->callback([this]() {
        // Set progress callback based on format
        switch (wordExtractProgressFormat) {
            case ProgressFormat::TTY:
                wordExtractionOptions.progressCallback = ttyProgressCallbackWithEta;
                break;
            case ProgressFormat::JSON:
                wordExtractionOptions.progressCallback = jsonProgressCallbackWithEta;
                break;
            case ProgressFormat::NONE:
                wordExtractionOptions.progressCallback = nullptr;
                break;
        }
    });
}

void OptionsParser::setupGlobalOptions() {
    // Set custom exit callback to handle help properly
    app.set_help_flag("-h,--help", "Print this help message and exit");
    app.set_help_all_flag("--help-all", "Print help message for all subcommands");

    // Add version flag
    app.add_flag_callback("-v,--version", []() {
        std::cout << "suzume-feedmill " << getVersion() << std::endl;
        exit(0);
    }, "Show version information");

    // Add quiet flag (global)
    app.add_flag_callback("-q,--quiet", [this]() {
        normalizeOptions.progressCallback = nullptr;
        pmiOptions.progressCallback = nullptr;
        wordExtractionOptions.progressCallback = nullptr;
        normalizeProgressFormat = ProgressFormat::NONE;
        pmiProgressFormat = ProgressFormat::NONE;
        wordExtractProgressFormat = ProgressFormat::NONE;
    }, "Suppress all output (same as --progress none)");

    // Add stats-json flag to each subcommand instead of as a global option
    // This ensures the option is recognized by each subcommand
    normalizeCommand->add_flag("--stats-json", statsJson, "Output statistics as JSON to stdout");
    pmiCommand->add_flag("--stats-json", statsJson, "Output statistics as JSON to stdout");
    wordExtractCommand->add_flag("--stats-json", statsJson, "Output statistics as JSON to stdout");

    // Also add it as a global option for help display
    app.add_flag("--stats-json", statsJson, "Output statistics as JSON to stdout");

    // Allow 0 or 1 subcommand (0 for help/version, 1 for normal operation)
    app.require_subcommand(0, 1);
}

int OptionsParser::parse(int argc, char* argv[]) {
    try {
        app.parse(argc, argv);
        return 0;
    } catch (const CLI::CallForHelp &e) {
        // Help was requested, display help and exit with success code
        app.exit(e);
        exit(0); // Exit immediately with success code
    } catch (const CLI::CallForAllHelp &e) {
        // Help-all was requested, display help and exit with success code
        app.exit(e);
        exit(0); // Exit immediately with success code
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }
}

const std::string& OptionsParser::getInputPath() const {
    return inputPath;
}

const std::string& OptionsParser::getOutputPath() const {
    return outputPath;
}

const suzume::NormalizeOptions& OptionsParser::getNormalizeOptions() const {
    return normalizeOptions;
}

size_t OptionsParser::getSampleSize() const {
    return sampleSize;
}

const suzume::PmiOptions& OptionsParser::getPmiOptions() const {
    return pmiOptions;
}

bool OptionsParser::isNormalizeCommand() const {
    return normalizeCommand && normalizeCommand->parsed();
}

bool OptionsParser::isPmiCommand() const {
    return pmiCommand && pmiCommand->parsed();
}

std::string OptionsParser::getVersion() {
    return "v0.1.0";
}

const suzume::WordExtractionOptions& OptionsParser::getWordExtractionOptions() const {
    return wordExtractionOptions;
}

bool OptionsParser::isWordExtractCommand() const {
    return wordExtractCommand && wordExtractCommand->parsed();
}

bool OptionsParser::isStatsJsonEnabled() const {
    return statsJson;
}

const std::string& OptionsParser::getOriginalTextPath() const {
    return originalTextPath;
}

} // namespace cli
} // namespace suzume
