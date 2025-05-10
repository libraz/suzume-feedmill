#!/bin/bash
# WebAssembly build script

set -e

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

# Check if Emscripten is available
if ! command -v emcc &> /dev/null; then
    echo "Error: Emscripten (emcc) not found."
    echo "Please install Emscripten: https://emscripten.org/docs/getting_started/downloads.html"
    echo "Example: "
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk"
    echo "  ./emsdk install latest"
    echo "  ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    exit 1
fi

# Create build directory
BUILD_DIR="$PROJECT_ROOT/build-wasm"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure project with CMake
echo "Configuring WebAssembly build..."
emcmake cmake .. -DBUILD_WASM=ON -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building WebAssembly module..."
emmake make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)

# Check output files
if [ -f "$BUILD_DIR/suzume-feedmill.js" ] && [ -f "$BUILD_DIR/suzume-feedmill.wasm" ]; then
    echo "Build successful!"
    echo "Output files:"
    echo "  $BUILD_DIR/suzume-feedmill.js"
    echo "  $BUILD_DIR/suzume-feedmill.wasm"

    # Copy to output directory
    OUTPUT_DIR="$PROJECT_ROOT/wasm"
    mkdir -p "$OUTPUT_DIR"
    cp "$BUILD_DIR/suzume-feedmill.js" "$OUTPUT_DIR/"
    cp "$BUILD_DIR/suzume-feedmill.wasm" "$OUTPUT_DIR/"
    echo "Files copied to $OUTPUT_DIR"
else
    echo "Error: Build failed or output files not found."
    exit 1
fi

echo "WebAssembly build completed!"
