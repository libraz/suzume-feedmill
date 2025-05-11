/**
 * @file wasm_main.cpp
 * @brief WebAssembly entry point for Suzume Feedmill
 */

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <fstream>
#include "suzume_feedmill.h"
#include "core/buffer_api.h"

using namespace emscripten;

/**
 * Normalize text using Suzume Feedmill
 */
val normalize(const std::string& text, val options) {
    try {
        // Parse options
        suzume::NormalizeOptions normOpt;

        // Form option
        if (options.hasOwnProperty("form")) {
            std::string form = options["form"].as<std::string>();
            if (form == "NFC") {
                normOpt.form = suzume::NormalizationForm::NFC;
            } else {
                normOpt.form = suzume::NormalizationForm::NFKC;
            }
        }

        // Threads option
        if (options.hasOwnProperty("threads")) {
            normOpt.threads = options["threads"].as<uint32_t>();
        }

        // Bloom filter false positive rate
        if (options.hasOwnProperty("bloomFp")) {
            normOpt.bloomFalsePositiveRate = options["bloomFp"].as<double>();
        }

        // Convert input string to buffer
        const uint8_t* inputData = reinterpret_cast<const uint8_t*>(text.c_str());
        size_t inputLength = text.length();

        // Prepare output buffer
        uint8_t* outputData = nullptr;
        size_t outputLength = 0;

        // Use buffer API for normalization
        auto result = suzume::core::normalizeBuffer(
            inputData,
            inputLength,
            &outputData,
            &outputLength,
            normOpt
        );

        // Convert output buffer to string
        std::string outputText;
        if (outputData && outputLength > 0) {
            outputText = std::string(reinterpret_cast<char*>(outputData), outputLength);
            // Free the buffer allocated by normalizeBuffer
            free(outputData);
        }

        // Create return object
        val returnVal = val::object();
        returnVal.set("text", outputText);
        returnVal.set("rows", result.rows);
        returnVal.set("uniques", result.uniques);
        returnVal.set("duplicates", result.duplicates);
        returnVal.set("elapsedMs", result.elapsedMs);
        returnVal.set("mbPerSec", result.mbPerSec);

        return returnVal;
    } catch (const std::exception& e) {
        val returnVal = val::object();
        returnVal.set("error", e.what());
        return returnVal;
    }
}

/**
 * Calculate PMI using Suzume Feedmill
 */
val calculatePmi(const std::string& text, val options) {
    try {
        // Parse options
        suzume::PmiOptions pmiOpt;

        // N-gram size
        if (options.hasOwnProperty("n")) {
            pmiOpt.n = options["n"].as<uint32_t>();
        }

        // Top K
        if (options.hasOwnProperty("topK")) {
            pmiOpt.topK = options["topK"].as<uint32_t>();
        }

        // Min frequency
        if (options.hasOwnProperty("minFreq")) {
            pmiOpt.minFreq = options["minFreq"].as<uint32_t>();
        }

        // Threads
        if (options.hasOwnProperty("threads")) {
            pmiOpt.threads = options["threads"].as<uint32_t>();
        }

        // Convert input string to buffer
        const uint8_t* inputData = reinterpret_cast<const uint8_t*>(text.c_str());
        size_t inputLength = text.length();

        // Prepare output buffer
        uint8_t* outputData = nullptr;
        size_t outputLength = 0;

        // Use buffer API for PMI calculation
        auto result = suzume::core::calculatePmiFromBuffer(
            inputData,
            inputLength,
            &outputData,
            &outputLength,
            pmiOpt
        );

        // Convert output buffer to string
        std::string outputText;
        if (outputData && outputLength > 0) {
            outputText = std::string(reinterpret_cast<char*>(outputData), outputLength);
            // Free the buffer allocated by calculatePmiFromBuffer
            free(outputData);
        }

        // Parse the output to create a JavaScript array of objects
        std::istringstream resultStream(outputText);
        std::string line;
        val results = val::array();

        while (std::getline(resultStream, line)) {
            std::istringstream lineStream(line);
            std::string ngram;
            double score;
            uint32_t frequency;

            if (lineStream >> ngram >> score >> frequency) {
                val item = val::object();
                item.set("ngram", ngram);
                item.set("score", score);
                item.set("frequency", frequency);
                results.call<void>("push", item);
            }
        }

        // Create return object
        val returnVal = val::object();
        returnVal.set("results", results);
        returnVal.set("grams", result.grams);
        returnVal.set("distinctNgrams", result.distinctNgrams);
        returnVal.set("elapsedMs", result.elapsedMs);
        returnVal.set("mbPerSec", result.mbPerSec);

        return returnVal;
    } catch (const std::exception& e) {
        val returnVal = val::object();
        returnVal.set("error", e.what());
        return returnVal;
    }
}

/**
 * Extract words using Suzume Feedmill
 */
val extractWords(const std::string& pmiText, const std::string& originalText, val options) {
    try {
        // Parse options
        suzume::WordExtractionOptions extractOpt;

        // Min PMI score
        if (options.hasOwnProperty("minPmiScore")) {
            extractOpt.minPmiScore = options["minPmiScore"].as<double>();
        }

        // Min/Max length
        if (options.hasOwnProperty("minLength")) {
            extractOpt.minLength = options["minLength"].as<uint32_t>();
        }
        if (options.hasOwnProperty("maxLength")) {
            extractOpt.maxLength = options["maxLength"].as<uint32_t>();
        }

        // Top K
        if (options.hasOwnProperty("topK")) {
            extractOpt.topK = options["topK"].as<uint32_t>();
        }

        // Threads
        if (options.hasOwnProperty("threads")) {
            extractOpt.threads = options["threads"].as<uint32_t>();
        }

        // For extractWords, we need to use temporary files since there's no buffer API
        // In WebAssembly, we can use memory paths
        std::string pmiPath = "/tmp/pmi_results.tsv";
        std::string textPath = "/tmp/original_text.txt";

        // Write the input data to temporary files
        {
            std::ofstream pmiFile(pmiPath);
            pmiFile << pmiText;
            pmiFile.close();

            std::ofstream textFile(textPath);
            textFile << originalText;
            textFile.close();
        }

        // Extract words using file paths
        auto result = suzume::extractWords(pmiPath, textPath, extractOpt);

        // Create return arrays
        val words = val::array();
        val scores = val::array();
        val frequencies = val::array();
        val contexts = val::array();

        for (size_t i = 0; i < result.words.size(); i++) {
            words.call<void>("push", result.words[i]);
            scores.call<void>("push", result.scores[i]);
            frequencies.call<void>("push", result.frequencies[i]);

            // Add contexts if available
            if (i < result.contexts.size()) {
                contexts.call<void>("push", result.contexts[i]);
            } else {
                // Use std::string instead of string literal to avoid Emscripten binding issues
                std::string emptyStr;
                contexts.call<void>("push", emptyStr);
            }
        }

        // Create return object
        val returnVal = val::object();
        returnVal.set("words", words);
        returnVal.set("scores", scores);
        returnVal.set("frequencies", frequencies);
        returnVal.set("contexts", contexts);
        returnVal.set("processingTimeMs", result.processingTimeMs);

        return returnVal;
    } catch (const std::exception& e) {
        val returnVal = val::object();
        returnVal.set("error", e.what());
        return returnVal;
    }
}

// Bind C++ functions to JavaScript
EMSCRIPTEN_BINDINGS(suzume_feedmill) {
    function("normalize", &normalize);
    function("calculatePmi", &calculatePmi);
    function("extractWords", &extractWords);
}
