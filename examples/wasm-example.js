// Example Node.js script for using suzume-feedmill WebAssembly module

// Load the WASM module
const SuzumeFeedmill = require("./suzume-feedmill.js");

// Sample text for testing
const sampleText = `今日は良い天気ですね。明日も晴れるといいな。
今日は　良い天気ですね。明日も晴れるといいな。
今日は良い天気ですね！明日も晴れるといいな～
人工知能の研究が進んでいます。機械学習の応用が広がっています。
自然言語処理技術の発展により、翻訳精度が向上しています。`;

async function runExample() {
  try {
    console.log("Loading suzume-feedmill WASM module...");
    const module = await SuzumeFeedmill();
    console.log("Module loaded successfully.\n");

    // Step 1: Normalize text
    console.log("Step 1: Normalizing text...");
    const normResult = module.normalize(sampleText, {
      form: "NFKC",
      threads: 2,
    });

    console.log("Normalized text:");
    console.log(normResult.text);
    console.log(
      `Processed ${normResult.rows} rows, ${normResult.uniques} unique rows\n`
    );

    // Step 2: Calculate PMI
    console.log("Step 2: Calculating PMI...");
    const pmiResult = module.calculatePmi(normResult.text, {
      n: 2,
      topK: 10,
      minFreq: 1,
    });

    console.log("PMI results:");
    pmiResult.results.forEach((item) => {
      console.log(`${item.ngram}\t${item.score.toFixed(2)}\t${item.frequency}`);
    });
    console.log(`Processed ${pmiResult.grams} n-grams\n`);

    // Format PMI results for word extraction
    const pmiText = pmiResult.results
      .map((r) => `${r.ngram}\t${r.score}\t${r.frequency}`)
      .join("\n");

    // Step 3: Extract words
    console.log("Step 3: Extracting words...");
    const wordResult = module.extractWords(pmiText, normResult.text, {
      minPmiScore: 1.0,
      minLength: 2,
      maxLength: 10,
      topK: 10,
    });

    console.log("Extracted words:");
    for (let i = 0; i < wordResult.words.length; i++) {
      console.log(
        `${wordResult.words[i]}\t${wordResult.scores[i].toFixed(2)}\t${
          wordResult.frequencies[i]
        }\t${wordResult.verified[i] ? "✓" : ""}`
      );
    }
    console.log(`Extracted ${wordResult.words.length} words`);

    console.log("\nExample completed successfully!");
  } catch (error) {
    console.error("Error:", error);
  }
}

runExample();
