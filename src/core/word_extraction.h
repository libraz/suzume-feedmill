/**
 * @file word_extraction.h
 * @brief Unknown word extraction functionality
 */

#ifndef SUZUME_CORE_WORD_EXTRACTION_H_
#define SUZUME_CORE_WORD_EXTRACTION_H_

#include <string>
#include <functional>
#include "suzume_feedmill.h"
#include "word_extraction/common.h"
#include "word_extraction/generator.h"
#include "word_extraction/verifier.h"
#include "word_extraction/filter.h"
#include "word_extraction/ranker.h"

namespace suzume {
namespace core {

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

/**
 * @brief Extract unknown words with progress reporting
 *
 * @param pmiResultsPath Path to PMI results file
 * @param originalTextPath Path to original text file
 * @param progressCallback Progress callback function
 * @param options Word extraction options
 * @return WordExtractionResult Results of the word extraction operation
 */
WordExtractionResult extractWordsWithProgress(
    const std::string& pmiResultsPath,
    const std::string& originalTextPath,
    const std::function<void(double)>& progressCallback,
    const WordExtractionOptions& options = WordExtractionOptions()
);

/**
 * @brief Extract unknown words with structured progress reporting
 *
 * @param pmiResultsPath Path to PMI results file
 * @param originalTextPath Path to original text file
 * @param progressCallback Structured progress callback function
 * @param options Word extraction options
 * @return WordExtractionResult Results of the word extraction operation
 */
WordExtractionResult extractWordsWithStructuredProgress(
    const std::string& pmiResultsPath,
    const std::string& originalTextPath,
    const std::function<void(const ProgressInfo&)>& progressCallback,
    const WordExtractionOptions& options = WordExtractionOptions()
);

} // namespace core
} // namespace suzume

#endif // SUZUME_CORE_WORD_EXTRACTION_H_
