# Suzume Feedmill WebAssembly Module

This package contains the WebAssembly build of Suzume Feedmill, a high-performance corpus preprocessing engine for character-level n-gram and PMI extraction.

## Contents

- `suzume-feedmill.js` - JavaScript glue code
- `suzume-feedmill.wasm` - WebAssembly binary
- `example.html` - Browser example
- `example.js` - Node.js example

## Usage in Browser

```html
<script>
  var Module = {
    onRuntimeInitialized: function() {
      // Module is ready to use
      const result = Module.normalize("Your text here", {
        form: 'NFKC',
        threads: 2
      });
      console.log(result);
    }
  };
</script>
<script src="suzume-feedmill.js"></script>
```

## Usage in Node.js

```javascript
const SuzumeFeedmill = require('./suzume-feedmill.js');

SuzumeFeedmill().then(module => {
  const result = module.normalize("Your text here", {
    form: 'NFKC',
    threads: 2
  });
  console.log(result);
});
```

## API

### normalize(text, options)

Normalizes and deduplicates text.

Options:
- `form`: Normalization form ('NFKC' or 'NFC', default: 'NFKC')
- `threads`: Number of threads (default: 2)

### calculatePmi(text, options)

Calculates PMI (Pointwise Mutual Information) for character n-grams.

Options:
- `n`: N-gram size (1, 2, or 3, default: 2)
- `topK`: Number of top results to return (default: 2500)
- `minFreq`: Minimum frequency threshold (default: 3)
- `threads`: Number of threads (default: 2)

### extractWords(pmiText, originalText, options)

Extracts potential unknown words from PMI results.

Options:
- `minPmiScore`: Minimum PMI score threshold (default: 3.0)
- `minLength`: Minimum word length (default: 2)
- `maxLength`: Maximum word length (default: 10)
- `topK`: Number of top results to return (default: 100)
- `threads`: Number of threads (default: 2)

## Examples

See `example.html` and `example.js` for complete examples.
