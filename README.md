# suzume-feedmill

> Grind the feed, sharpen the tokens.

A high-performance corpus preprocessing engine for character-level n-gram and PMI extraction, designed to supply "unknown word candidate powder" to Suzume series (e.g., SuzumeFeed).

## Features

- **High-performance text normalization**

  - Unicode normalization (NFKC/NFC)
  - Duplicate removal with Bloom filter
  - Multi-threaded processing

- **Fast n-gram and PMI extraction**

  - Support for 1-3 character n-grams
  - Top-K PMI extraction using min-heap
  - Configurable minimum frequency threshold

- **Unknown word extraction** (Core feature for the project's main purpose)

  - Candidate generation from high-PMI n-grams
  - Verification against original text
  - Filtering based on statistical and contextual analysis
  - Ranking of potential unknown words
  - Component-based pipeline for discovering truly unknown words

- **Cross-platform**
  - Prebuilt binaries for Linux (glibc/musl), macOS (Intel/ARM), and Windows (x64)
  - Available as a standalone CLI
  - WebAssembly support for browser and Node.js environments

## Installation

### From Source

```bash
# Clone the repository
git clone https://github.com/yourusername/suzume-feedmill.git
cd suzume-feedmill

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# Install (optional)
sudo make install
```

### Using Prebuilt Binaries

Prebuilt binaries for various platforms are available on the [Releases](https://github.com/yourusername/suzume-feedmill/releases) page.

## Quick Start Guide

Here's how to get started with suzume-feedmill in just a few steps:

```bash
# Normalize a text file (removes duplicates and standardizes text)
suzume-feedmill normalize input.tsv normalized.tsv

# Calculate PMI for character n-grams
suzume-feedmill pmi normalized.tsv ngrams.tsv --n 2 --top 5000

# Extract unknown words
suzume-feedmill word-extract ngrams.tsv normalized.tsv words.tsv
```

See the "Data Formats" section below for detailed examples of input and output formats.

## CLI Usage

### Text Normalization

```bash
suzume-feedmill normalize <in.tsv> <out.tsv> [options]

Options:
  --threads N         Number of threads (default: logical cores)
  --form NFKC|NFC     Normalization form (default: NFKC)
  --bloom-fp 0.01     Bloom filter false positive rate
  --progress tty|json|none  Progress reporting format (default: tty)
  --sample N          Sample N lines randomly from input
  --min-length N      Minimum line length (0 = no minimum)
  --max-length N      Maximum line length (0 = no maximum)
  --stats-json        Output statistics as JSON to stdout
```

### PMI Calculation

```bash
suzume-feedmill pmi <in.txt> <out.tsv> [options]

Options:
  --n 1|2|3           N-gram size (default: 2)
  --top K             Number of top results (default: 2500)
  --min-freq N        Minimum frequency threshold (default: 3)
  --threads N         Number of threads (default: logical cores)
  --progress tty|json|none  Progress reporting format (default: tty)
  --stats-json        Output statistics as JSON to stdout
```

### Word Extraction

```bash
suzume-feedmill word-extract <pmi-results.tsv> <original-text.txt> <output.tsv> [options]

Options:
  --min-score N       Minimum PMI score threshold (default: 3.0)
  --min-length N      Minimum word length (default: 2)
  --max-length N      Maximum word length (default: 10)
  --top K             Number of top results (default: 100)
  --verify true|false Verify candidates in original text (default: true)
  --threads N         Number of threads (default: logical cores)
  --progress tty|json|none  Progress reporting format (default: tty)
  --stats-json        Output statistics as JSON to stdout
```

## C++ API

The library provides a C++ API for integration into your own applications:

```cpp
#include <suzume_feedmill.h>
#include <iostream>

int main() {
  // Text normalization
  suzume::NormalizeOptions normOpt;
  normOpt.threads = 8;
  auto normResult = suzume::normalize("raw.tsv", "clean.tsv", normOpt);

  std::cout << "Processed " << normResult.rows << " rows, "
            << normResult.uniques << " unique rows in "
            << normResult.elapsedMs << " ms" << std::endl;

  // PMI calculation
  suzume::PmiOptions pmiOpt;
  pmiOpt.n = 2;
  pmiOpt.topK = 2500;
  auto pmiResult = suzume::calculatePmi("clean.tsv", "grams.tsv", pmiOpt);

  std::cout << "Processed " << pmiResult.grams << " n-grams in "
            << pmiResult.elapsedMs << " ms" << std::endl;

  return 0;
}
```

## Data Formats

suzume-feedmill uses specific data formats for its input and output files:

### Input Text Format

The input text file should contain one text entry per line. This can be a sentence, paragraph, or any text unit you want to process:

```
今日は良い天気ですね。明日も晴れるといいな。
今日は　良い天気ですね。明日も晴れるといいな。
今日は良い天気ですね！明日も晴れるといいな～
人工知能の研究が進んでいます。機械学習の応用が広がっています。
自然言語処理技術の発展により、翻訳精度が向上しています。
```

### Normalized Text Format

The normalized text file contains deduplicated, normalized text entries. During normalization, the following transformations are applied:

- Duplicate entries are removed
- Whitespace is standardized
- Punctuation is normalized
- Unicode normalization (NFKC/NFC) is applied

```
今日は良い天気ですね。明日も晴れるといいな。
人工知能の研究が進んでいます。機械学習の応用が広がっています。
自然言語処理技術の発展により、翻訳精度が向上しています。
```

Note: In this example, the first three lines from the input were considered duplicates after normalization and merged into a single line.

### N-gram PMI Format

The n-gram PMI file is a tab-separated values (TSV) file with three columns:

1. n-gram: The character sequence
2. PMI score: The pointwise mutual information score
3. Frequency: How many times this n-gram appears in the corpus

```
人工	8.76	127
工知	7.92	98
知能	7.45	76
```

### Word Extraction Format

The word extraction output is a tab-separated values (TSV) file with four columns:

1. Word: The extracted word candidate
2. Score: Average PMI score
3. Components: Number of component n-grams
4. Verification: Check mark (✓) if found in original text

```
人工知能	7.38	3	✓
機械学習	6.94	2	✓
自然言語処理	5.87	4	✓
```

## Real-World Example Workflow

Here's a typical workflow for discovering unknown words in a large corpus:

1. **Data Collection**: Gather a large corpus of text data (e.g., web pages, articles, social media posts)

   ```bash
   # Example: Collect 1 million tweets into tweets.tsv
   ```

2. **Text Normalization**: Clean and normalize the text data

   ```bash
   # Basic normalization
   suzume-feedmill normalize tweets.tsv normalized.tsv --threads 8
   
   # Normalization with sampling (process only 1000 random lines)
   suzume-feedmill normalize tweets.tsv normalized_sample.tsv --sample 1000
   
   # Normalization with line length filters
   suzume-feedmill normalize tweets.tsv normalized_filtered.tsv --min-length 10 --max-length 200
   
   # Normalization with JSON statistics output
   suzume-feedmill normalize tweets.tsv normalized.tsv --stats-json > stats.json
   
   # Using stdin/stdout for pipeline processing
   cat tweets.tsv | suzume-feedmill normalize - - | grep "keyword" > filtered.tsv
   ```

   Input example (tweets.tsv) - One text entry per line:

   ```
   今日は良い天気ですね。明日も晴れるといいな。
   今日は　良い天気ですね。明日も晴れるといいな。
   今日は良い天気ですね！明日も晴れるといいな～
   人工知能の研究が進んでいます。機械学習の応用が広がっています。
   自然言語処理技術の発展により、翻訳精度が向上しています。
   ```

   Output example (normalized.tsv) - Normalized, deduplicated text:

   ```
   今日は良い天気ですね。明日も晴れるといいな。
   人工知能の研究が進んでいます。機械学習の応用が広がっています。
   自然言語処理技術の発展により、翻訳精度が向上しています。
   ```

   (Note: Duplicates and variations are removed, whitespace is normalized, and punctuation is standardized)

3. **PMI Calculation**: Extract statistically significant character-level n-grams

   ```bash
   suzume-feedmill pmi normalized.tsv ngrams.tsv --n 2 --top 5000 --min-freq 5
   ```

   Output example (ngrams.tsv) - Tab-separated values with n-grams, scores, and frequencies:

   ```
   人工	8.76	127
   工知	7.92	98
   知能	7.45	76
   機械	7.21	64
   学習	6.89	112
   自然	6.54	89
   然言	6.32	85
   言語	6.21	92
   語処	5.98	78
   処理	5.87	81
   ```

   (Format: n-gram[TAB]PMI score[TAB]frequency)

4. **Unknown Word Extraction**: Extract potential unknown words from the PMI results

   ```bash
   suzume-feedmill word-extract ngrams.tsv normalized.tsv potential-words.tsv --min-score 3.0 --top 100
   ```

   Output example (potential-words.tsv) - Tab-separated values with extracted words and metadata:

   ```
   人工知能	7.38	3	✓
   機械学習	6.94	2	✓
   自然言語処理	5.87	4	✓
   深層学習	5.21	2	✓
   ニューラルネットワーク	4.95	5	✓
   教師あり学習	4.76	3	✓
   データマイニング	4.52	3	✓
   ```

   (Format: word[TAB]average PMI score[TAB]number of component n-grams[TAB]verification status)

   The verification status (✓) indicates that the word was found in the original text. Words that don't appear in the original text but are constructed from high-PMI n-grams would not have this mark.

## Architecture

- **Core**: C++17 implementation with SIMD optimizations
- **Normalization**: ICU for UTF-8 validation and normalization
- **Deduplication**: xxHash64 + BloomFilter for efficient duplicate detection
- **Data Structures**: robin_hood hash map with lock-free thread merging
- **PMI Calculation**: Memory-mapped frequency counting with min-heap for top-K extraction
- **Word Extraction**: Component-based pipeline for candidate generation, verification, filtering, and ranking

## Technical Glossary

- **Bloom filter**: A space-efficient probabilistic data structure used for testing whether an element is a member of a set. Used in suzume-feedmill for efficient duplicate detection with minimal memory usage.

- **PMI (Pointwise Mutual Information)**: A statistical measure of association between two events (in this case, character co-occurrences). High PMI scores indicate characters that appear together more frequently than would be expected by chance.

- **n-gram**: A contiguous sequence of n items (characters in our case) from a given sample of text. suzume-feedmill works with character-level n-grams (1-3 characters).

- **min-heap**: A binary tree data structure where the parent node is always smaller than or equal to its children. Used for efficient top-K extraction in PMI calculation.

- **ICU (International Components for Unicode)**: A mature, widely used set of C/C++ libraries for Unicode support and software internationalization. Used for UTF-8 validation and normalization.

- **xxHash64**: A fast non-cryptographic hash algorithm, used for efficient string hashing in the duplicate detection process. It provides the initial hash values for both the Bloom filter and the hash map.

- **robin_hood hash map**: An open addressing hash table implementation that uses robin hood hashing to reduce variance in probe sequence length. Used for efficient in-memory data structures.

These technologies work together in the duplicate detection process: xxHash64 quickly hashes strings, the Bloom filter provides a fast initial check (with possible false positives), and the robin_hood hash map performs accurate verification when needed. This two-stage approach combines speed and accuracy.

## Project Structure

```
/src            C++ core implementation
  /cli          CLI implementation
  /core         Core functionality
  /io           Input/output handling
  /parallel     Parallel processing
  /third_party  Third-party libraries
  /wasm         WebAssembly bindings
/include        Public headers
/bin            Executable binaries
/examples       Usage examples
/tests          Tests
/CMakeLists.txt Build configuration
```

## Building from Source

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# Run tests
make test
```

## WebAssembly Support

suzume-feedmill can be compiled to WebAssembly (WASM) for use in web browsers or Node.js environments. This allows you to run the library directly in the browser without any server-side processing.

### Building for WebAssembly

To build the WebAssembly module, you need to install Emscripten first:

```bash
# Install Emscripten (if not already installed)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

Then, build the WebAssembly module:

```bash
# Create build directory
mkdir build-wasm && cd build-wasm

# Configure with Emscripten
emcmake cmake .. -DBUILD_WASM=ON

# Build
emmake make
```

This will generate the following files:

- `suzume-feedmill.js`: JavaScript glue code
- `suzume-feedmill.wasm`: WebAssembly binary

### Using in a Web Browser

```html
<!DOCTYPE html>
<html>
  <head>
    <title>suzume-feedmill WASM Demo</title>
  </head>
  <body>
    <h1>suzume-feedmill WASM Demo</h1>
    <textarea id="input" rows="10" cols="50">
今日は良い天気ですね。明日も晴れるといいな。
今日は　良い天気ですね。明日も晴れるといいな。
今日は良い天気ですね！明日も晴れるといいな～</textarea
    >
    <button id="normalize">Normalize</button>
    <div id="output"></div>

    <script>
      // Load the WASM module
      var Module = {
        onRuntimeInitialized: function () {
          document
            .getElementById("normalize")
            .addEventListener("click", function () {
              var input = document.getElementById("input").value;

              // Call the normalize function
              var result = Module.normalize(input, {
                form: "NFKC",
                threads: 2,
              });

              // Display the result
              document.getElementById("output").innerHTML =
                "<h3>Normalized Text:</h3>" +
                "<pre>" +
                result.text +
                "</pre>" +
                "<h3>Stats:</h3>" +
                "<p>Rows: " +
                result.rows +
                "</p>" +
                "<p>Unique rows: " +
                result.uniques +
                "</p>" +
                "<p>Duplicates removed: " +
                result.duplicates +
                "</p>";
            });
        },
      };
    </script>
    <script src="suzume-feedmill.js"></script>
  </body>
</html>
```

### Using in Node.js

```javascript
// Load the WASM module
const SuzumeFeedmill = require("./suzume-feedmill.js");

SuzumeFeedmill().then((module) => {
  // Normalize text
  const text = `今日は良い天気ですね。明日も晴れるといいな。
今日は　良い天気ですね。明日も晴れるといいな。
今日は良い天気ですね！明日も晴れるといいな～`;

  const normResult = module.normalize(text, {
    form: "NFKC",
    threads: 2,
  });

  console.log("Normalized text:");
  console.log(normResult.text);
  console.log(
    `Processed ${normResult.rows} rows, ${normResult.uniques} unique rows`,
  );

  // Calculate PMI
  const pmiResult = module.calculatePmi(normResult.text, {
    n: 2,
    topK: 10,
    minFreq: 1,
  });

  console.log("\nPMI results:");
  pmiResult.results.forEach((item) => {
    console.log(`${item.ngram}\t${item.score.toFixed(2)}\t${item.frequency}`);
  });
});
```

## License

MIT

## Important Note on Character-Level Processing

suzume-feedmill operates at the **character level**, not the word level. This is by design, as it's particularly useful for:

1. **Languages without explicit word boundaries** (Japanese, Chinese, etc.)
2. **Discovering truly unknown words** that wouldn't be recognized by existing tokenizers
3. **Identifying sub-word patterns** that might be meaningful

For word-level analysis, you would typically:

1. Use suzume-feedmill to identify interesting character combinations
2. Apply post-processing to combine adjacent high-PMI n-grams
3. Filter the results based on frequency, length, and other criteria

The examples in the `examples/` directory demonstrate both the basic usage and more advanced post-processing techniques.

## Related Projects

- SuzumeFeed (coming soon) - Uses the output from suzume-feedmill to enhance tokenization models
