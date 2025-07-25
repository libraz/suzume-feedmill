/**
 * @file verifier.cpp
 * @brief Implementation of candidate verifier for word extraction
 */

#include "verifier.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/normalizer2.h>

using icu::UnicodeString;

namespace suzume {
namespace core {

CandidateVerifier::CandidateVerifier(const WordExtractionOptions& options)
    : options_(options)
{
    // Load dictionary if needed
    if (options_.useDictionaryLookup && !options_.dictionaryPath.empty()) {
        std::ifstream dictFile(options_.dictionaryPath);
        if (dictFile.is_open()) {
            std::string word;
            while (std::getline(dictFile, word)) {
                dictionary_.insert(word);
            }
        }
    }
}

std::vector<VerifiedCandidate> CandidateVerifier::verifyCandidates(
    const std::vector<WordCandidate>& candidates,
    const std::string& originalTextPath,
    const std::function<void(double)>& progressCallback
) {
    std::vector<VerifiedCandidate> verifiedCandidates;

    // Create text index
    TextIndex textIndex(originalTextPath);

    // Process each candidate
    size_t total = candidates.size();
    size_t processed = 0;

    for (const auto& candidate : candidates) {
        // Verify in original text if required
        bool verified = true;
        if (options_.verifyInOriginalText) {
            verified = verifyInText(candidate, textIndex);
        }

        // Skip if not verified
        if (!verified) {
            processed++;
            if (progressCallback) {
                progressCallback(static_cast<double>(processed) / total);
            }
            continue;
        }

        // Analyze context if required
        std::string context;
        double contextScore = 0.0;
        if (options_.useContextualAnalysis) {
            auto [ctx, score] = analyzeContext(candidate, textIndex);
            context = ctx;
            contextScore = score;
        }

        // Validate statistically if required
        double statisticalScore = 0.0;
        if (options_.useStatisticalValidation) {
            statisticalScore = validateStatistically(candidate, textIndex);
        }

        // Check dictionary if required
        if (options_.useDictionaryLookup && !dictionary_.empty()) {
            if (lookupInDictionary(candidate)) {
                // Skip words that are already in the dictionary
                processed++;
                if (progressCallback) {
                    progressCallback(static_cast<double>(processed) / total);
                }
                continue;
            }
        }

        // Create verified candidate
        VerifiedCandidate verifiedCandidate;
        verifiedCandidate.text = candidate.text;
        verifiedCandidate.score = candidate.score;
        verifiedCandidate.frequency = candidate.frequency;
        verifiedCandidate.context = context;
        verifiedCandidate.contextScore = contextScore;
        verifiedCandidate.statisticalScore = statisticalScore;

        verifiedCandidates.push_back(verifiedCandidate);

        // Report progress
        processed++;
        if (progressCallback) {
            progressCallback(static_cast<double>(processed) / total);
        }
    }

    return verifiedCandidates;
}

CandidateVerifier::TextIndex::TextIndex(const std::string& textPath) {
    // Read text file
    std::ifstream file(textPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open original text file: " + textPath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    text_ = buffer.str();
}

bool CandidateVerifier::TextIndex::contains(const std::string& pattern) const {
    return text_.find(pattern) != std::string::npos;
}

std::vector<size_t> CandidateVerifier::TextIndex::findAll(const std::string& pattern) const {
    std::vector<size_t> positions;
    size_t pos = 0;

    while ((pos = text_.find(pattern, pos)) != std::string::npos) {
        positions.push_back(pos);
        pos += pattern.length();
    }

    return positions;
}

std::string CandidateVerifier::TextIndex::getContext(size_t position, size_t contextSize) const {
    // Convert to ICU UnicodeString for proper UTF-8 handling
    UnicodeString ustr = UnicodeString::fromUTF8(text_);

    // Find the code point index for the byte position
    int32_t cpIndex = 0;
    int32_t bytePos = 0;

    while (bytePos < static_cast<int32_t>(position) && cpIndex < ustr.length()) {
        UChar32 c = ustr.char32At(cpIndex);
        cpIndex += U16_LENGTH(c);

        // Calculate UTF-8 bytes for this code point
        int32_t len = U8_LENGTH(c);
        bytePos += len;
    }

    // Calculate start and end code point indices
    int32_t startCp = std::max(0, cpIndex - static_cast<int32_t>(contextSize));
    int32_t endCp = std::min(ustr.length(), cpIndex + static_cast<int32_t>(contextSize));

    // Extract the context substring
    UnicodeString contextStr = UnicodeString(ustr, startCp, endCp - startCp);

    // Convert back to UTF-8
    std::string result;
    contextStr.toUTF8String(result);

    return result;
}

bool CandidateVerifier::verifyInText(const WordCandidate& candidate, const TextIndex& textIndex) {
    return textIndex.contains(candidate.text);
}

std::pair<std::string, double> CandidateVerifier::analyzeContext(
    const WordCandidate& candidate,
    const TextIndex& textIndex
) {
    // Find all occurrences
    auto positions = textIndex.findAll(candidate.text);

    // If no occurrences, return empty context
    if (positions.empty()) {
        return {"", 0.0};
    }

    // Get context of first occurrence
    std::string context = textIndex.getContext(positions[0]);

    // Simple context score based on number of occurrences
    double contextScore = std::min(1.0, positions.size() / 10.0);

    return {context, contextScore};
}

double CandidateVerifier::validateStatistically(
    const WordCandidate& candidate,
    const TextIndex& textIndex
) {
    // Zero frequency gets zero score
    if (candidate.frequency == 0) {
        return 0.0;
    }

    // Base score from frequency (normalized)
    double frequencyScore = std::min(1.0, candidate.frequency / 20.0);
    
    // Length bonus: longer words are generally more informative
    // Bonus scales with character count (UTF-8 aware)
    size_t charCount = 0;
    const char* ptr = candidate.text.c_str();
    const char* end = ptr + candidate.text.length();
    
    // Count UTF-8 characters properly
    while (ptr < end) {
        unsigned char c = *ptr;
        if (c < 0x80) {
            // ASCII character
            ptr++;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte UTF-8 character
            ptr += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte UTF-8 character (common for Japanese)
            ptr += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte UTF-8 character
            ptr += 4;
        } else {
            // Invalid UTF-8, skip
            ptr++;
        }
        charCount++;
    }
    
    // Length bonus: diminishing returns after 4 characters
    double lengthBonus = 1.0 + std::min(0.3, (charCount - 1) * 0.1);
    
    // Apply length bonus to frequency score
    double statisticalScore = frequencyScore * lengthBonus;
    
    // Consider context diversity if available
    auto positions = textIndex.findAll(candidate.text);
    if (positions.size() > 1) {
        // Multiple occurrences in different contexts boost the score
        double contextDiversityBonus = 1.0 + std::min(0.2, (positions.size() - 1) * 0.05);
        statisticalScore *= contextDiversityBonus;
    }
    
    // Cap at 1.0
    return std::min(1.0, statisticalScore);
}

bool CandidateVerifier::lookupInDictionary(const WordCandidate& candidate) {
    return dictionary_.find(candidate.text) != dictionary_.end();
}

} // namespace core
} // namespace suzume
