# suzume-feedmill Examples

This directory contains practical examples demonstrating how to use the suzume-feedmill C++ library.

## Language Support

Examples are available in multiple languages:
- **en/** - English examples
- **ja/** - Japanese examples (日本語版)

## Available Examples

1. **normalize-basic-example.cpp** - Basic text normalization with filtering
2. **normalize-sampling-example.cpp** - Random sampling from large datasets
3. **progress-with-eta-example.cpp** - Progress tracking with ETA calculation
4. **stats-simple-example.cpp** - Simple statistics output and performance metrics
5. **streaming-io-example.cpp** - Streaming I/O and pipeline processing

## Building Examples

### Option 1: Using CMake (Recommended)

```bash
# Build English examples (default)
mkdir build && cd build
cmake ..
make

# Build Japanese examples
mkdir build-ja && cd build-ja
cmake .. -DEXAMPLE_LANG=ja
make

# Run examples
./normalize-basic-example
./progress-with-eta-example
```

### Option 2: Using Makefile

```bash
# First, build the main library
cd ..
mkdir build && cd build
cmake ..
make
cd ../examples

# Build English examples (default)
make all

# Build Japanese examples
make all LANG=ja

# Run examples
./build/normalize-basic-example
./build/progress-with-eta-example
```

### Option 3: Manual Compilation

```bash
# Build main library first (if not done already)
cd .. && mkdir build && cd build && cmake .. && make && cd ../examples

# Compile individual example (macOS)
c++ -std=c++17 -I../include -I../src en/normalize-basic-example.cpp \
    -o normalize-basic-example ../build/src/libsuzume_core.a \
    ../build/src/core/libsuzume_core_lib.a ../build/src/io/libsuzume_io.a \
    ../build/src/parallel/libsuzume_parallel.a ../build/libxxhash.a \
    -I$(brew --prefix icu4c)/include -L$(brew --prefix icu4c)/lib \
    -licuuc -licuio

# Run
./normalize-basic-example
```

## Prerequisites

### System Dependencies

**macOS:**
```bash
brew install icu4c cmake
```

**Ubuntu/Debian:**
```bash
sudo apt-get install libicu-dev cmake build-essential
```

**CentOS/RHEL:**
```bash
sudo yum install libicu-devel cmake gcc-c++
```

### Library Dependencies

The examples require the main suzume-feedmill library to be built first:

```bash
cd ..
mkdir build && cd build
cmake ..
make
```

## Example Descriptions

### normalize-basic-example.cpp
Demonstrates basic text normalization functionality including:
- Loading text from file
- Applying Unicode normalization (NFKC)
- Filtering by line length
- Removing duplicates
- Performance measurement

### normalize-sampling-example.cpp
Shows how to process large datasets efficiently:
- Random sampling from input
- Memory-efficient processing
- Progress tracking
- Statistical analysis

### progress-with-eta-example.cpp
Illustrates progress callback implementation:
- Real-time progress updates
- ETA (Estimated Time of Arrival) calculation
- Custom progress callback functions
- Performance monitoring

### stats-simple-example.cpp
Demonstrates statistics collection and simple output:
- Performance metrics gathering
- Simple text formatting of results
- Processing time analysis
- Summary reporting

### streaming-io-example.cpp
Shows streaming I/O capabilities:
- Pipeline processing
- stdin/stdout handling
- Large file processing
- Memory-efficient streaming

## Input Data

The examples include sample data in the `output/` directory:
- `simple-text.txt` - Basic Japanese text sample
- `advanced-*.tsv` - Pre-processed example outputs

You can use your own data by modifying the file paths in the examples.

## Cleanup

To clean up build files and temporary outputs:

```bash
# Clean CMake build
rm -rf build/

# Clean Makefile build  
make clean

# Clean example outputs
rm -f test-*.txt test-*.tsv *test-output*
rm -rf *_example_output/
```

## Troubleshooting

### Common Issues

1. **ICU library not found**
   ```
   Error: library 'icuuc' not found
   ```
   Solution: Install ICU development packages (see Prerequisites above)

2. **suzume-feedmill library not found**
   ```
   Error: libsuzume_core.a not found
   ```
   Solution: Build the main library first (see Library Dependencies above)

3. **Compiler errors**
   ```
   Error: C++17 features not supported
   ```
   Solution: Use a modern compiler (GCC 7+, Clang 6+, MSVC 2017+)

### macOS Specific Issues

If you encounter SDK path issues:
```bash
export SDKROOT=$(xcrun --show-sdk-path)
```

### Performance Tips

- Use Release builds for performance testing: `cmake -DCMAKE_BUILD_TYPE=Release ..`
- For large datasets, consider using the sampling examples
- Monitor memory usage with the stats examples

## WebAssembly Examples

For WebAssembly usage examples, see:
- `wasm-example.html` - Browser integration
- `wasm-example.js` - Node.js usage
- `wasm-readme.md` - WebAssembly API documentation

## Contributing

When adding new examples:
1. Follow the existing naming convention: `feature-description-example.cpp`
2. Include comprehensive error handling
3. Add progress tracking for long-running operations
4. Document the example purpose and usage
5. Update this README with the new example description