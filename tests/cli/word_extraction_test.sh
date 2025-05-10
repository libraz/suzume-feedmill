#!/bin/bash
# Tests for word extraction CLI functionality

# Exit on error
set -e

# Set up test directory
TEST_DIR="$(mktemp -d)"
echo "Using temporary directory: $TEST_DIR"

# Clean up on exit
trap 'rm -rf "$TEST_DIR"' EXIT

# Path to the CLI executable
CLI_EXECUTABLE="../../build/bin/suzume-feedmill"

# Create test files
echo "Creating test files..."

# Create PMI results file
cat > "$TEST_DIR/pmi_results.tsv" << EOF
ngram	score	freq
人工知能	5.2	10
機械学習	4.8	8
深層学習	4.5	7
自然言語	4.2	6
処理技術	4.0	5
人工知	3.8	4
知能研	3.5	3
研究開	3.2	2
開発者	3.0	1
EOF

# Create original text file
cat > "$TEST_DIR/original_text.txt" << EOF
人工知能と機械学習の研究が進んでいます。
深層学習を用いた自然言語処理技術の開発が行われています。
人工知能研究開発者が集まるカンファレンスが開催されました。
EOF

# Output file
OUTPUT_FILE="$TEST_DIR/extracted_words.tsv"

# Test 1: Basic word extraction
echo "Test 1: Basic word extraction..."
"$CLI_EXECUTABLE" word-extract "$TEST_DIR/pmi_results.tsv" "$TEST_DIR/original_text.txt" "$OUTPUT_FILE" --min-pmi 3.0 --top 5 --progress none

# Check that the output file exists
if [ ! -f "$OUTPUT_FILE" ]; then
    echo "Error: Output file not created"
    exit 1
fi

# Check that the output file contains some results
if [ ! -s "$OUTPUT_FILE" ]; then
    echo "Error: Output file is empty"
    exit 1
fi

# Count the number of lines in the output file
LINE_COUNT=$(wc -l < "$OUTPUT_FILE")
if [ "$LINE_COUNT" -lt 1 ]; then
    echo "Error: Output file has too few lines"
    exit 1
fi

echo "Test 1 passed"

# Test 2: Word extraction with different options
echo "Test 2: Word extraction with different options..."
"$CLI_EXECUTABLE" word-extract "$TEST_DIR/pmi_results.tsv" "$TEST_DIR/original_text.txt" "$OUTPUT_FILE" --min-pmi 4.5 --top 3 --progress none

# Check that the output file exists
if [ ! -f "$OUTPUT_FILE" ]; then
    echo "Error: Output file not created"
    exit 1
fi

# Count the number of lines in the output file
LINE_COUNT=$(wc -l < "$OUTPUT_FILE")
if [ "$LINE_COUNT" -gt 4 ]; then  # Header + 3 results
    echo "Error: Output file has too many lines"
    exit 1
fi

echo "Test 2 passed"

# Test 3: Word extraction with no verification
echo "Test 3: Word extraction with no verification..."
"$CLI_EXECUTABLE" word-extract "$TEST_DIR/pmi_results.tsv" "$TEST_DIR/original_text.txt" "$OUTPUT_FILE" --min-pmi 3.0 --top 10 --no-verify --progress none

# Check that the output file exists
if [ ! -f "$OUTPUT_FILE" ]; then
    echo "Error: Output file not created"
    exit 1
fi

# Check that the output file contains more results
LINE_COUNT=$(wc -l < "$OUTPUT_FILE")
if [ "$LINE_COUNT" -lt 5 ]; then
    echo "Error: Output file has too few lines"
    exit 1
fi

echo "Test 3 passed"

# Test 4: Word extraction with language-specific options
echo "Test 4: Word extraction with language-specific options..."
"$CLI_EXECUTABLE" word-extract "$TEST_DIR/pmi_results.tsv" "$TEST_DIR/original_text.txt" "$OUTPUT_FILE" --min-pmi 3.0 --top 5 --language ja --progress none

# Check that the output file exists
if [ ! -f "$OUTPUT_FILE" ]; then
    echo "Error: Output file not created"
    exit 1
fi

echo "Test 4 passed"

# Test 5: Error handling - non-existent input file
echo "Test 5: Error handling - non-existent input file..."
if "$CLI_EXECUTABLE" word-extract "non_existent_file.tsv" "$TEST_DIR/original_text.txt" "$OUTPUT_FILE" --progress none 2>/dev/null; then
    echo "Error: Command should have failed with non-existent input file"
    exit 1
fi

echo "Test 5 passed"

# Test 6: Error handling - non-existent original text file
echo "Test 6: Error handling - non-existent original text file..."
if "$CLI_EXECUTABLE" word-extract "$TEST_DIR/pmi_results.tsv" "non_existent_file.txt" "$OUTPUT_FILE" --progress none 2>/dev/null; then
    echo "Error: Command should have failed with non-existent original text file"
    exit 1
fi

echo "Test 6 passed"

# All tests passed
echo "All tests passed!"
exit 0
