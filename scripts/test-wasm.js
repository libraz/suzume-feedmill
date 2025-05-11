#!/usr/bin/env node
/**
 * WebAssembly test script for suzume-feedmill
 *
 * This script tests the basic functionality of the WebAssembly module
 * by running normalize, calculatePmi, and extractWords functions.
 */

const fs = require("fs");
const path = require("path");

// Get project root directory
const projectRoot = path.resolve(__dirname, "..");
const wasmDir = path.join(projectRoot, "wasm");
const wasmModule = path.join(wasmDir, "suzume-feedmill.js");

// Check if WASM module exists
if (!fs.existsSync(wasmModule)) {
  console.error(`Error: WASM module not found at ${wasmModule}`);
  console.error(
    "Please run scripts/build-wasm.sh first to build the WASM module."
  );
  process.exit(1);
}

// Sample text for testing
const sampleText = `今日は良い天気ですね。明日も晴れるといいな。
今日は　良い天気ですね。明日も晴れるといいな。
今日は良い天気ですね！明日も晴れるといいな～
人工知能の研究が進んでいます。機械学習の応用が広がっています。
自然言語処理技術の発展により、翻訳精度が向上しています。`;

// Load the WASM module
console.log("Loading WASM module...");
const SuzumeFeedmill = require(wasmModule);

// Run tests
async function runTests() {
  try {
    const module = await SuzumeFeedmill();
    console.log("WASM module loaded successfully.");

    // Test 1: Normalize text
    console.log("\n=== Test 1: Normalize text ===");
    const normResult = module.normalize(sampleText, {
      form: "NFKC",
      threads: 2,
    });

    if (!normResult || normResult.error) {
      throw new Error(
        `Normalization failed: ${normResult?.error || "Unknown error"}`
      );
    }

    console.log("Normalized text:");
    console.log(normResult.text);
    console.log(
      `Processed ${normResult.rows} rows, ${normResult.uniques} unique rows`
    );
    console.log("Test 1 passed!");

    // Test 2: Calculate PMI
    console.log("\n=== Test 2: Calculate PMI ===");
    const pmiResult = module.calculatePmi(normResult.text, {
      n: 2,
      topK: 10,
      minFreq: 1,
    });

    if (!pmiResult || pmiResult.error) {
      throw new Error(
        `PMI calculation failed: ${pmiResult?.error || "Unknown error"}`
      );
    }

    console.log("PMI results:");
    pmiResult.results.forEach((item) => {
      console.log(`${item.ngram}\t${item.score.toFixed(2)}\t${item.frequency}`);
    });
    console.log(`Processed ${pmiResult.grams} n-grams`);
    console.log("Test 2 passed!");

    // Test 3: Extract words
    console.log("\n=== Test 3: Extract words ===");
    const wordResult = module.extractWords(
      pmiResult.results
        .map((r) => `${r.ngram}\t${r.score}\t${r.frequency}`)
        .join("\n"),
      normResult.text,
      {
        minPmiScore: 1.0,
        minLength: 2,
        maxLength: 10,
        topK: 10,
      }
    );

    if (!wordResult || wordResult.error) {
      throw new Error(
        `Word extraction failed: ${wordResult?.error || "Unknown error"}`
      );
    }

    console.log("Extracted words:");
    for (let i = 0; i < wordResult.words.length; i++) {
      console.log(
        `${wordResult.words[i]}\t${wordResult.scores[i].toFixed(2)}\t${
          wordResult.frequencies[i]
        }`
      );
    }
    console.log(`Extracted ${wordResult.words.length} words`);
    console.log("Test 3 passed!");

    console.log("\nAll tests passed!");
    return true;
  } catch (error) {
    console.error(`\nTest failed: ${error.message}`);
    console.error(error.stack);
    return false;
  }
}

// Run tests and exit with appropriate code
runTests().then((success) => {
  process.exit(success ? 0 : 1);
});
