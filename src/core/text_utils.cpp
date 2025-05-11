/**
 * @file text_utils.cpp
 * @brief Implementation of text processing utilities
 */

#include "core/text_utils.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <unordered_set>
#include <vector>
#include <cstring>
#include <cerrno>
#include <random>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/normalizer2.h>
#include <unicode/uniset.h>
#ifndef EMSCRIPTEN
#include <unicode/ustream.h>
#include <unicode/ustdio.h>
#endif
#include <unicode/uclean.h>
#include "xxhash.h"

using icu::UnicodeString;
using icu::Normalizer2;

namespace suzume {
namespace core {

namespace {
    std::mutex g_mutex;
    bool g_initialized = false;

    // Code point range for emoji detection
    struct CodePointRange {
        char32_t start;
        char32_t end;
    };

    // Emoji range definitions
    // Based on Unicode 13.0 emoji ranges
    std::vector<CodePointRange> g_emojiRanges = {
        {0x1F000, 0x1F02F},  // Mahjong Tiles
        {0x1F030, 0x1F09F},  // Domino Tiles
        {0x1F0A0, 0x1F0FF},  // Playing Cards
        {0x1F100, 0x1F1FF},  // Enclosed Alphanumeric Supplement
        {0x1F200, 0x1F2FF},  // Enclosed Ideographic Supplement
        {0x1F300, 0x1F5FF},  // Miscellaneous Symbols and Pictographs
        {0x1F600, 0x1F64F},  // Emoticons
        {0x1F650, 0x1F67F},  // Ornamental Dingbats
        {0x1F680, 0x1F6FF},  // Transport and Map Symbols
        {0x1F700, 0x1F77F},  // Alchemical Symbols
        {0x1F780, 0x1F7FF},  // Geometric Shapes Extended
        {0x1F800, 0x1F8FF},  // Supplemental Arrows-C
        {0x1F900, 0x1F9FF},  // Supplemental Symbols and Pictographs
        {0x1FA00, 0x1FA6F},  // Chess Symbols
        {0x1FA70, 0x1FAFF}   // Symbols and Pictographs Extended-A
    };

    // Special character set (used in emoji sequences)
    std::unordered_set<char32_t> g_specialChars = {
        0x200D,  // Zero Width Joiner
        0xFE0F,  // Variation Selector-16 (emoji style)
        0x20E3   // Combining Enclosing Keycap
    };

    // Initialization function
    void ensureInitialized() {
        std::lock_guard<std::mutex> lock(g_mutex);

        if (g_initialized) {
            return;
        }

        // Initialize ICU if needed
        UErrorCode status = U_ZERO_ERROR;
        u_init(&status);
        if (U_FAILURE(status)) {
            std::cerr << "Failed to initialize ICU: " << u_errorName(status) << std::endl;
        }

        g_initialized = true;
    }

    // Check if a code point is within the specified ranges
    bool isInRanges(char32_t codepoint, const std::vector<CodePointRange>& ranges) {
        for (const auto& range : ranges) {
            if (codepoint >= range.start && codepoint <= range.end) {
                return true;
            }
        }
        return false;
    }

    // Check if a character is whitespace or punctuation
    bool isWhitespaceOrPunct(char32_t c) {
        // ASCII whitespace
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') {
            return true;
        }

        // ASCII punctuation
        if ((c >= 0x21 && c <= 0x2F) || (c >= 0x3A && c <= 0x40) ||
            (c >= 0x5B && c <= 0x60) || (c >= 0x7B && c <= 0x7E)) {
            return true;
        }

        // Unicode whitespace
        if (c == 0x00A0 || c == 0x1680 || (c >= 0x2000 && c <= 0x200A) ||
            c == 0x2028 || c == 0x2029 || c == 0x202F || c == 0x205F || c == 0x3000) {
            return true;
        }

        // Unicode punctuation (partial implementation)
        if ((c >= 0x2010 && c <= 0x2027) || (c >= 0x2030 && c <= 0x205E) ||
            (c >= 0x2190 && c <= 0x2BFF) || (c >= 0x3001 && c <= 0x303F)) {
            return true;
        }

        return false;
    }

    // Check if a string is emoji-only
    bool isEmojiOnly(const std::string& text) {
        if (text.empty()) {
            return false;
        }

        try {
            // Convert to ICU UnicodeString
            UnicodeString ustr = UnicodeString::fromUTF8(text);
            if (ustr.isBogus()) {
                return false; // Invalid UTF-8
            }

            bool hasEmoji = false;
            bool hasNonEmoji = false;

            // Iterate through each code point
            for (int32_t i = 0; i < ustr.length(); /* increment in loop */) {
                UChar32 c = ustr.char32At(i);
                i += U16_LENGTH(c); // Move to next code point

                // Skip whitespace and punctuation
                if (isWhitespaceOrPunct(c)) {
                    continue;
                }

                // Check if it's an emoji using ICU
                if (u_hasBinaryProperty(c, UCHAR_EMOJI) ||
                    isInRanges(c, g_emojiRanges) ||
                    g_specialChars.count(c) > 0) {
                    hasEmoji = true;
                } else {
                    hasNonEmoji = true;
                }

                // Early exit if we found both emoji and non-emoji
                if (hasEmoji && hasNonEmoji) {
                    break;
                }
            }

            return hasEmoji && !hasNonEmoji;
        } catch (const std::exception& e) {
            std::cerr << "Error in isEmojiOnly: " << e.what() << std::endl;
            return false;
        } catch (...) {
            std::cerr << "Unknown error in isEmojiOnly" << std::endl;
            return false;
        }
    }

    // Split a string by delimiter
    std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;

        if (str.empty()) {
            // For empty strings, return an array containing an empty string
            tokens.push_back("");
            return tokens;
        }

        size_t start = 0;
        size_t end = 0;

        // Find each occurrence of the delimiter
        while ((end = str.find(delimiter, start)) != std::string::npos) {
            // Add the substring from start to end
            tokens.push_back(str.substr(start, end - start));
            start = end + 1;
        }

        // Add the last token (or empty string if the delimiter was the last character)
        tokens.push_back(str.substr(start));

        return tokens;
    }

    // Strip control and format characters
    static std::string stripControl(const std::string& in) {
        UnicodeString u = UnicodeString::fromUTF8(in);
        UnicodeString out;
        for (int32_t i = 0; i < u.length();) {
            UChar32 cp = u.char32At(i);
            // Skip Cc(Other,Control) and Cf(Other,Format) characters
            if (u_charType(cp) != U_CONTROL_CHAR &&
                u_charType(cp) != U_FORMAT_CHAR) {
                out.append(cp);
            }
            i += U16_LENGTH(cp);
        }
        std::string utf8;
        out.toUTF8String(utf8);
        return utf8;
    }

    // Unicode normalization implementation
    std::string normalizeText(const std::string& text, suzume::NormalizationForm form) {
#ifdef EMSCRIPTEN
        // Simplified implementation for WebAssembly
        if (text.empty()) {
            return text;
        }

        try {
            // Convert to ICU UnicodeString
            UnicodeString ustr = UnicodeString::fromUTF8(text);
            if (ustr.isBogus()) {
                return text; // Return original text if invalid UTF-8
            }

            // Get the appropriate normalizer
            UErrorCode status = U_ZERO_ERROR;
            const Normalizer2* normalizer = nullptr;

            if (form == suzume::NormalizationForm::NFKC) {
                normalizer = Normalizer2::getNFKCInstance(status);
            } else if (form == suzume::NormalizationForm::NFC) {
                normalizer = Normalizer2::getNFCInstance(status);
            }

            if (U_FAILURE(status) || normalizer == nullptr) {
                return text;
            }

            // Perform normalization
            UnicodeString normalized = normalizer->normalize(ustr, status);
            if (U_FAILURE(status)) {
                return text;
            }

            // Convert back to UTF-8
            std::string result;
            normalized.toUTF8String(result);
            return result;
        } catch (...) {
            return text;
        }
#else
        // Full implementation for native platforms
        if (text.empty()) {
            return text;
        }

        try {
            // Ensure initialization
            ensureInitialized();

            // Convert to ICU UnicodeString
            UnicodeString ustr = UnicodeString::fromUTF8(text);
            if (ustr.isBogus()) {
                return text; // Return original text if invalid UTF-8
            }

            // Get the appropriate normalizer
            UErrorCode status = U_ZERO_ERROR;
            const Normalizer2* normalizer = nullptr;

            if (form == suzume::NormalizationForm::NFKC) {
                normalizer = Normalizer2::getNFKCInstance(status);
            } else if (form == suzume::NormalizationForm::NFC) {
                normalizer = Normalizer2::getNFCInstance(status);
            }

            if (U_FAILURE(status) || normalizer == nullptr) {
                std::cerr << "Failed to get normalizer: " << u_errorName(status) << std::endl;
                return text;
            }

            // Perform normalization
            UnicodeString normalized = normalizer->normalize(ustr, status);
            if (U_FAILURE(status)) {
                std::cerr << "Normalization failed: " << u_errorName(status) << std::endl;
                return text;
            }

            // Convert back to UTF-8
            std::string result;
            normalized.toUTF8String(result);
            return result;
        } catch (const std::exception& e) {
            std::cerr << "Error in normalizeText: " << e.what() << std::endl;
            return text;
        } catch (...) {
            std::cerr << "Unknown error in normalizeText" << std::endl;
            return text;
        }
#endif
    }

    // Convert text to lowercase
    std::string toLower(const std::string& text) {
        if (text.empty()) {
            return text;
        }

        try {
            // Convert to ICU UnicodeString
            UnicodeString ustr = UnicodeString::fromUTF8(text);
            if (ustr.isBogus()) {
                return text; // Return original text if invalid UTF-8
            }

            // Perform lowercase conversion
            ustr.toLower();

            // Convert back to UTF-8
            std::string result;
            ustr.toUTF8String(result);
            return result;
        } catch (const std::exception& e) {
            std::cerr << "Error in toLower: " << e.what() << std::endl;
            return text; // Return original text on error
        } catch (...) {
            std::cerr << "Unknown error in toLower" << std::endl;
            return text; // Return original text on error
        }
    }
}

std::string normalizeLine(const std::string& line, suzume::NormalizationForm form) {
    try {

        // Processing based on specification:
        // 1. Exclude whitespace-only lines
        bool isWhitespaceOnly = true;
        for (char c : line) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                isWhitespaceOnly = false;
                break;
            }
        }

        if (isWhitespaceOnly) {
            return ""; // Exclude whitespace-only lines
        }

        // 2. Exclude lines with length ≤1 (count Unicode code points, not bytes)
        UnicodeString ustr = UnicodeString::fromUTF8(line);

        // Skip empty or single-character lines
        if (ustr.countChar32() <= 1) {
            return ""; // Skip empty or single-character lines
        }

        // Comment exclusion removed to handle hashtags properly

        // 4. Exclude emoji-only lines
        if (line.find('\t') == std::string::npos) {
            if (line.length() >= 4 && (unsigned char)line[0] >= 0xF0) {
                // 4-byte UTF-8 sequence (likely emoji)
                if (isEmojiOnly(line)) {
                    return ""; // Skip emoji-only lines
                }
            }
        }

        // Split by tab to preserve TSV structure
        std::vector<std::string> fields;
        try {
            fields = split(line, '\t');
        } catch (const std::exception& e) {
            return "";
        } catch (...) {
            return "";
        }

        if (fields.empty()) {
            return ""; // Skip empty fields
        }

        // Process each field separately
        for (auto& field : fields) {
            // Unicode normalization using ICU (both NFC and NFKC)
            field = normalizeText(field, form);

            // Skip if normalization failed
            if (field.empty() && !line.empty()) {
                return "";
            }

            // Strip control characters using ICU (safe for all Unicode)
            field = stripControl(field);

            // Skip lowercase conversion for fields that contain numbers or when using NFC form
            if (form != suzume::NormalizationForm::NFC &&
                std::none_of(field.begin(), field.end(), ::isdigit)) {
                // Use toLower for proper UTF-8 lowercase conversion
                field = toLower(field);
            }
        }

        // Reconstruct the line with tab separators
        std::string normalizedLine;
        normalizedLine.reserve(line.length() * 2);  // Reserve space for safety

        for (size_t i = 0; i < fields.size(); ++i) {
            normalizedLine += fields[i];
            if (i < fields.size() - 1) {
                normalizedLine += '\t';
            }
        }

        return normalizedLine;
    } catch (const std::exception& e) {
        return "";
    } catch (...) {
        return "";
    }
}

bool shouldExcludeLine(const std::string& line, uint32_t minLength, uint32_t maxLength) {
    // Skip empty lines
    if (line.empty()) {
        return true;
    }

    // Skip lines with length <= 1
    if (line.length() <= 1) {
        return true;
    }

    // Comment exclusion removed to handle hashtags properly

    // Check for emoji-only lines
    if (line.find('\t') == std::string::npos) {
        if (line.length() >= 4 && (unsigned char)line[0] >= 0xF0) {
            // This is likely a 4-byte UTF-8 sequence (emoji)
            if (isEmojiOnly(line)) {
                return true;
            }
        }
    }

    // Apply min/max length filters if specified
    if (minLength > 0 && line.length() < minLength) {
        return true;
    }

    if (maxLength > 0 && line.length() > maxLength) {
        return true;
    }

    return false;
}

// Backward compatibility wrapper
bool shouldExcludeLine(const std::string& line) {
    return shouldExcludeLine(line, 0, 0);
}

std::vector<std::string> generateNgrams(const std::string& text, int n) {
    std::vector<std::string> ngrams;

    if (text.empty() || n <= 0) {
        return ngrams;
    }

    try {
        // Convert to ICU UnicodeString
        UnicodeString ustr = UnicodeString::fromUTF8(text);
        if (ustr.isBogus()) {
            return ngrams; // Return empty vector if invalid UTF-8
        }

        // Extract code points
        std::vector<UChar32> codepoints;
        for (int32_t i = 0; i < ustr.length(); /* increment in loop */) {
            UChar32 c = ustr.char32At(i);
            codepoints.push_back(c);
            i += U16_LENGTH(c); // Move to next code point
        }

        // If text is shorter than n, return empty vector
        if (codepoints.size() < static_cast<size_t>(n)) {
            return ngrams;
        }

        // Generate n-grams
        for (size_t i = 0; i <= codepoints.size() - n; i++) {
            // Create a substring for this n-gram
            UnicodeString ngram_ustr;
            for (size_t j = 0; j < static_cast<size_t>(n); j++) {
                ngram_ustr.append(codepoints[i + j]);
            }

            // Convert to UTF-8
            std::string ngram;
            ngram_ustr.toUTF8String(ngram);
            ngrams.push_back(ngram);
        }

        return ngrams;
    } catch (const std::exception& e) {
        std::cerr << "Error in generateNgrams: " << e.what() << std::endl;
        return ngrams;
    } catch (...) {
        std::cerr << "Unknown error in generateNgrams" << std::endl;
        return ngrams;
    }
}

uint64_t calculateHash(const std::string& str) {
    return XXH64(str.data(), str.size(), 0);
}

bool isDuplicate(const std::string& str,
                std::unordered_set<std::string>& uniqueSet,
                double bloomFalsePositiveRate) {
    // Simple implementation without Bloom filter for now
    // In a real implementation, we would use a Bloom filter for initial check
    // with the specified false positive rate
    (void)bloomFalsePositiveRate; // Suppress unused parameter warning

    // For empty strings, always consider as duplicate
    if (str.empty()) {
        return true;
    }

    // Check if already in the set
    if (uniqueSet.find(str) != uniqueSet.end()) {
        return true;
    }

    // Not a duplicate, add to set
    uniqueSet.insert(str);
    return false;
}

std::vector<std::string> sampleLines(
    const std::string& inputPath,
    size_t sampleSize,
    unsigned int seed
) {
    // Initialize result vector
    std::vector<std::string> result;

    // Check if sample size is valid
    if (sampleSize == 0) {
        return result;
    }

    // Check if file exists before attempting to open
    if (!std::filesystem::exists(inputPath)) {
        throw std::runtime_error("File does not exist: " + inputPath);
    }

    try {
        // Initialize random number generator
        std::mt19937 gen;
        if (seed == 0) {
            // Use time-based seed if not specified
            std::random_device rd;
            gen.seed(rd());
        } else {
            gen.seed(seed);
        }

        // Open input file
        std::ifstream file(inputPath);
        if (!file.is_open()) {
            // Provide more detailed error message based on errno
            std::string errorMsg;
            if (errno == EACCES || errno == EPERM) {
                errorMsg = "Permission denied: Cannot read " + inputPath;
            } else if (errno == ENOENT) {
                errorMsg = "File does not exist: " + inputPath;
            } else {
                errorMsg = "Failed to open file: " + inputPath + " (errno: " + std::to_string(errno) + ")";
            }
            throw std::runtime_error(errorMsg);
        }

        // Read lines using Reservoir sampling algorithm
        std::string line;
        size_t lineCount = 0;

        // Fill the reservoir with the first sampleSize lines
        while (lineCount < sampleSize && std::getline(file, line)) {
            result.push_back(line);
            lineCount++;
        }

        // Process the rest of the lines using Reservoir sampling
        while (std::getline(file, line)) {
            lineCount++;

            // Generate a random index in [0, lineCount)
            std::uniform_int_distribution<size_t> dist(0, lineCount - 1);
            size_t j = dist(gen);

            // Replace element at index j with probability sampleSize/lineCount
            if (j < sampleSize) {
                result[j] = line;
            }
        }

        return result;
    } catch (const std::exception& e) {
        std::cerr << "Error in sampleLines: " << e.what() << std::endl;
        return result;
    } catch (...) {
        std::cerr << "Unknown error in sampleLines" << std::endl;
        return result;
    }
}

// Overload for sampling from a vector of strings
std::vector<std::string> sampleLines(
    const std::vector<std::string>& lines,
    size_t sampleSize,
    unsigned int seed
) {
    // Initialize result vector
    std::vector<std::string> result;

    // Check if sample size is valid
    if (sampleSize == 0) {
        return result;
    }

    // Check if there are enough lines
    if (lines.empty()) {
        return result;
    }

    try {
        // Initialize random number generator
        std::mt19937 gen;
        if (seed == 0) {
            // Use time-based seed if not specified
            std::random_device rd;
            gen.seed(rd());
        } else {
            gen.seed(seed);
        }

        // Use Reservoir sampling algorithm
        size_t lineCount = lines.size();

        // If sample size is greater than or equal to the number of lines, return all lines
        if (sampleSize >= lineCount) {
            return lines;
        }

        // Fill the reservoir with the first sampleSize lines
        result.assign(lines.begin(), lines.begin() + sampleSize);

        // Process the rest of the lines using Reservoir sampling
        for (size_t i = sampleSize; i < lineCount; i++) {
            // Generate a random index in [0, i)
            std::uniform_int_distribution<size_t> dist(0, i);
            size_t j = dist(gen);

            // Replace element at index j with probability sampleSize/i
            if (j < sampleSize) {
                result[j] = lines[i];
            }
        }

        return result;
    } catch (const std::exception& e) {
        std::cerr << "Error in sampleLines: " << e.what() << std::endl;
        return result;
    } catch (...) {
        std::cerr << "Unknown error in sampleLines" << std::endl;
        return result;
    }
}

} // namespace core
} // namespace suzume
