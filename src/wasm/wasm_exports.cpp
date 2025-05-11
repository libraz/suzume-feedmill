/**
 * @file wasm_exports.cpp
 * @brief C-style exports for WebAssembly
 */

#include <suzume_feedmill.h>
#include <emscripten.h>
#include <string>

// C-style exports for WebAssembly
extern "C" {

// Export normalize function
EMSCRIPTEN_KEEPALIVE
const char* normalize(const char* inputPath, const char* outputPath) {
    static std::string result;
    try {
        suzume::NormalizeOptions options;
        suzume::NormalizeResult normalizeResult = suzume::normalize(inputPath, outputPath, options);

        // Format result as JSON
        result = "{\"rows\":" + std::to_string(normalizeResult.rows) +
                 ",\"uniques\":" + std::to_string(normalizeResult.uniques) +
                 ",\"duplicates\":" + std::to_string(normalizeResult.duplicates) +
                 ",\"elapsedMs\":" + std::to_string(normalizeResult.elapsedMs) +
                 ",\"mbPerSec\":" + std::to_string(normalizeResult.mbPerSec) + "}";

        return result.c_str();
    } catch (const std::exception& e) {
        result = "{\"error\":\"" + std::string(e.what()) + "\"}";
        return result.c_str();
    } catch (...) {
        result = "{\"error\":\"Unknown error\"}";
        return result.c_str();
    }
}

// Export calculatePmi function
EMSCRIPTEN_KEEPALIVE
const char* calculatePmi(const char* inputPath, const char* outputPath) {
    static std::string result;
    try {
        suzume::PmiOptions options;
        suzume::PmiResult pmiResult = suzume::calculatePmi(inputPath, outputPath, options);

        // Format result as JSON
        result = "{\"grams\":" + std::to_string(pmiResult.grams) +
                 ",\"distinctNgrams\":" + std::to_string(pmiResult.distinctNgrams) +
                 ",\"elapsedMs\":" + std::to_string(pmiResult.elapsedMs) +
                 ",\"mbPerSec\":" + std::to_string(pmiResult.mbPerSec) + "}";

        return result.c_str();
    } catch (const std::exception& e) {
        result = "{\"error\":\"" + std::string(e.what()) + "\"}";
        return result.c_str();
    } catch (...) {
        result = "{\"error\":\"Unknown error\"}";
        return result.c_str();
    }
}

// Export extractWords function
EMSCRIPTEN_KEEPALIVE
const char* extractWords(const char* pmiResultsPath, const char* originalTextPath) {
    static std::string result;
    try {
        suzume::WordExtractionOptions options;
        suzume::WordExtractionResult extractResult = suzume::extractWords(pmiResultsPath, originalTextPath, options);

        // Format result as JSON (simplified - just return count)
        result = "{\"wordCount\":" + std::to_string(extractResult.words.size()) +
                 ",\"processingTimeMs\":" + std::to_string(extractResult.processingTimeMs) + "}";

        return result.c_str();
    } catch (const std::exception& e) {
        result = "{\"error\":\"" + std::string(e.what()) + "\"}";
        return result.c_str();
    } catch (...) {
        result = "{\"error\":\"Unknown error\"}";
        return result.c_str();
    }
}

} // extern "C"
