/**
 * @file suzume_feedmill.h
 * @brief Main header file for the Suzume Feedmill library
 *
 * This header provides the public API for the Suzume Feedmill library,
 * a high-performance corpus preprocessing engine for n-gram and PMI extraction.
 */

#ifndef SUZUME_FEEDMILL_H_
#define SUZUME_FEEDMILL_H_

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace suzume {

/**
 * @brief Normalization form for Unicode text
 */
enum class NormalizationForm {
  NFKC, ///< Normalization Form KC (Compatibility Composition)
  NFC   ///< Normalization Form C (Canonical Composition)
};

/**
 * @brief Progress information structure
 */
struct ProgressInfo {
  /**
   * @brief Processing phase
   */
  enum class Phase {
    Reading,    ///< Reading input file
    Processing, ///< Processing data
    Calculating, ///< Calculating results
    Writing,    ///< Writing output file
    Complete    ///< Operation complete
  };

  Phase phase = Phase::Reading;      ///< Current processing phase
  double phaseRatio = 0.0;           ///< Progress ratio within the current phase (0.0-1.0)
  double overallRatio = 0.0;         ///< Overall progress ratio (0.0-1.0)

  // Additional metadata
  uint64_t processedBytes = 0;       ///< Processed bytes
  uint64_t totalBytes = 0;           ///< Total bytes
  double processingSpeed = 0.0;      ///< Processing speed (MB/s)
  double estimatedTimeLeft = 0.0;    ///< Estimated time left (seconds)
};

/**
 * @brief Progress output format
 */
enum class ProgressFormat {
  TTY,  ///< Terminal output with progress bar
  JSON, ///< JSON format for machine parsing
  NONE  ///< No progress output
};

// 重複した定義を削除

/**
 * @brief Options for text normalization
 */
struct NormalizeOptions {
  NormalizationForm form = NormalizationForm::NFKC; ///< Unicode normalization form
  double bloomFalsePositiveRate = 0.000001;         ///< Bloom filter false positive rate (reduced to minimize false positives)
  uint32_t threads = 0;                             ///< Number of threads (0 = auto)
  ProgressFormat progressFormat = ProgressFormat::TTY; ///< Progress output format
  double progressStep = 0.05;                       ///< Progress reporting granularity (0.0-1.0)

  /**
   * @brief Callback function for progress updates
   * @param ratio Progress ratio from 0.0 to 1.0
   */
  std::function<void(double ratio)> progressCallback = nullptr;

  /**
   * @brief Structured callback function for detailed progress updates
   * @param info Detailed progress information
   */
  std::function<void(const ProgressInfo& info)> structuredProgressCallback = nullptr;
};

/**
 * @brief Options for PMI calculation
 */
struct PmiOptions {
  uint32_t n = 2;                                  ///< N-gram size (1, 2, or 3)
  uint32_t topK = 2500;                            ///< Number of top PMI results to return
  uint32_t minFreq = 3;                            ///< Minimum frequency threshold
  uint32_t threads = 0;                            ///< Number of threads (0 = auto)
  ProgressFormat progressFormat = ProgressFormat::TTY; ///< Progress output format
  double progressStep = 0.05;                      ///< Progress reporting granularity (0.0-1.0)
  bool verbose = false;                            ///< Enable verbose logging to stderr

  /**
   * @brief Callback function for progress updates
   * @param ratio Progress ratio from 0.0 to 1.0
   */
  std::function<void(double ratio)> progressCallback = nullptr;

  /**
   * @brief Structured callback function for detailed progress updates
   * @param info Detailed progress information
   */
  std::function<void(const ProgressInfo& info)> structuredProgressCallback = nullptr;
};

/**
 * @brief Result of normalization operation
 */
struct NormalizeResult {
  uint64_t rows = 0;         ///< Total number of rows processed
  uint64_t uniques = 0;      ///< Number of unique rows after deduplication
  uint64_t duplicates = 0;   ///< Number of duplicate rows removed
  uint64_t elapsedMs = 0;    ///< Processing time in milliseconds
  double mbPerSec = 0.0;     ///< Processing speed in MB/sec
};

/**
 * @brief Result of PMI calculation
 */
struct PmiResult {
  uint64_t grams = 0;            ///< Total number of n-grams processed
  uint64_t distinctNgrams = 0;   ///< Number of distinct n-grams found
  uint64_t elapsedMs = 0;        ///< Processing time in milliseconds
  double mbPerSec = 0.0;         ///< Processing speed in MB/sec
};

/**
 * @brief Options for word extraction
 */
struct WordExtractionOptions {
  // 候補生成オプション
  double minPmiScore = 1.0;              ///< Minimum PMI score
  uint32_t maxCandidateLength = 20;      ///< Maximum candidate length
  uint32_t maxCandidates = 100000;       ///< Maximum number of candidates

  // 検証オプション
  bool verifyInOriginalText = true;      ///< Verify in original text
  bool useContextualAnalysis = true;     ///< Use contextual analysis
  bool useStatisticalValidation = true;  ///< Use statistical validation
  bool useDictionaryLookup = false;      ///< Use dictionary lookup
  std::string dictionaryPath = "";       ///< Dictionary path

  // フィルタリングオプション
  uint32_t minLength = 2;                ///< Minimum length
  uint32_t maxLength = 20;               ///< Maximum length
  double minScore = 0.5;                 ///< Minimum score
  bool removeSubstrings = true;          ///< Remove substrings
  bool removeOverlapping = true;         ///< Remove overlapping
  std::string languageCode = "ja";       ///< Language code
  bool useLanguageSpecificRules = true;  ///< Use language-specific rules

  // ランキングオプション
  uint32_t topK = 1000;                  ///< Number of top results
  std::string rankingModel = "combined"; ///< Ranking model
  double pmiWeight = 0.4;                ///< PMI weight
  double lengthWeight = 0.2;             ///< Length weight
  double contextWeight = 0.2;            ///< Context weight
  double statisticalWeight = 0.2;        ///< Statistical weight

  // 並列処理オプション
  bool useParallelProcessing = true;     ///< Use parallel processing
  uint32_t threads = 0;                  ///< Number of threads (0 = auto)

  // 進捗報告オプション
  ProgressFormat progressFormat = ProgressFormat::TTY; ///< Progress output format
  double progressStep = 0.05;            ///< Progress reporting granularity (0.0-1.0)

  /**
   * @brief Callback function for progress updates
   * @param ratio Progress ratio from 0.0 to 1.0
   */
  std::function<void(double ratio)> progressCallback = nullptr;

  /**
   * @brief Structured callback function for detailed progress updates
   * @param info Detailed progress information
   */
  std::function<void(const ProgressInfo& info)> structuredProgressCallback = nullptr;
};

/**
 * @brief Result of word extraction operation
 */
struct WordExtractionResult {
  std::vector<std::string> words;      ///< Extracted unknown words
  std::vector<double> scores;          ///< Scores
  std::vector<uint32_t> frequencies;   ///< Frequencies
  std::vector<std::string> contexts;   ///< Contexts (optional)
  uint64_t processingTimeMs = 0;       ///< Processing time in milliseconds
  uint64_t memoryUsageBytes = 0;       ///< Memory usage in bytes
};

/**
 * @brief Normalize text data
 *
 * @param inputPath Path to input TSV file
 * @param outputPath Path to output TSV file
 * @param options Normalization options
 * @return NormalizeResult Results of the normalization operation
 */
NormalizeResult normalize(
  const std::string& inputPath,
  const std::string& outputPath,
  const NormalizeOptions& options = NormalizeOptions()
);

/**
 * @brief Calculate PMI (Pointwise Mutual Information)
 *
 * @param inputPath Path to input text file
 * @param outputPath Path to output TSV file
 * @param options PMI calculation options
 * @return PmiResult Results of the PMI calculation
 */
PmiResult calculatePmi(
  const std::string& inputPath,
  const std::string& outputPath,
  const PmiOptions& options = PmiOptions()
);

/**
 * @brief Extract unknown words from PMI results
 *
 * @param pmiResultsPath Path to PMI results file
 * @param originalTextPath Path to original text file
 * @param options Word extraction options
 * @return WordExtractionResult Results of the word extraction operation
 */
WordExtractionResult extractWords(
  const std::string& pmiResultsPath,
  const std::string& originalTextPath,
  const WordExtractionOptions& options = WordExtractionOptions()
);

} // namespace suzume

#endif // SUZUME_FEEDMILL_H_
