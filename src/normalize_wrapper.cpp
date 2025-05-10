/**
 * @file normalize_wrapper.cpp
 * @brief Implementation of wrapper functions for the public API
 */

#include "suzume_feedmill.h"
#include "core/normalize.h"
#include "core/pmi.h"
#include "core/word_extraction.h"

namespace suzume {

NormalizeResult normalize(
    const std::string& inputPath,
    const std::string& outputPath,
    const NormalizeOptions& options
) {
    return core::normalize(inputPath, outputPath, options);
}

PmiResult calculatePmi(
    const std::string& inputPath,
    const std::string& outputPath,
    const PmiOptions& options
) {
    return core::calculatePmi(inputPath, outputPath, options);
}

WordExtractionResult extractWords(
    const std::string& pmiResultsPath,
    const std::string& originalTextPath,
    const WordExtractionOptions& options
) {
    return core::extractWords(pmiResultsPath, originalTextPath, options);
}

} // namespace suzume
