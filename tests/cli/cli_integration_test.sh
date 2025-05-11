#!/bin/bash
# CLI Integration Tests for suzume-feedmill
# This script tests the CLI binary directly

# Exit on error
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counter
TESTS_TOTAL=0
TESTS_PASSED=0
TESTS_FAILED=0

# Path to CLI binary (adjust as needed)
CLI_BIN="/Users/libraz/Projects/suzume-feedmill/build/suzume-feedmill"

# Create temporary directory for test files
TEMP_DIR=$(mktemp -d)
echo -e "${YELLOW}Using temporary directory: ${TEMP_DIR}${NC}"

# Cleanup function
cleanup() {
    echo -e "${YELLOW}Cleaning up temporary files...${NC}"
    rm -rf "$TEMP_DIR"
}

# Register cleanup function to run on exit
trap cleanup EXIT

# Helper function to run a test
run_test() {
    local test_name="$1"
    local command="$2"
    local expected_exit_code="$3"

    echo -e "${YELLOW}Running test: ${test_name}${NC}"

    # Run the command
    set +e
    eval "$command"
    local actual_exit_code=$?
    set -e

    # Check exit code
    if [ "$actual_exit_code" -eq "$expected_exit_code" ]; then
        echo -e "${GREEN}✓ Test passed: ${test_name}${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ Test failed: ${test_name}${NC}"
        echo -e "${RED}  Expected exit code: ${expected_exit_code}, got: ${actual_exit_code}${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi

    TESTS_TOTAL=$((TESTS_TOTAL + 1))
}

# Create test input files
echo -e "${YELLOW}Creating test input files...${NC}"

# Sample TSV data for normalize
cat > "$TEMP_DIR/normalize_input.tsv" << EOF
apple\tred fruit
banana\tyellow fruit
orange\tcitrus fruit
apple\tred fruit
grape\tpurple fruit
EOF

# Sample text data for PMI
cat > "$TEMP_DIR/pmi_input.txt" << EOF
this is a test
this is another test
testing is important
test your code thoroughly
EOF

# Test 1: Help command
run_test "Help command" "$CLI_BIN --help" 0

# Test 2: Version command
run_test "Version command" "$CLI_BIN --version" 0

# Test 3: No command (should fail)
run_test "No command" "$CLI_BIN" 1

# Test 4: Invalid command (should fail)
run_test "Invalid command" "$CLI_BIN invalid-command" 109

# Test 5: Normalize command with missing arguments (should fail)
run_test "Normalize missing arguments" "$CLI_BIN normalize" 106

# Test 6: Normalize command with basic arguments
run_test "Normalize basic" "$CLI_BIN normalize $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output.tsv" 0

# Test 7: Normalize command with options
run_test "Normalize with options" "$CLI_BIN normalize $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output_options.tsv --form NFKC --bloom-fp 0.05 --threads 2 --progress none" 0

# Test 8: Normalize command with quiet flag
run_test "Normalize with quiet flag" "$CLI_BIN normalize -q $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output_quiet.tsv" 109

# Test 9: PMI command with missing arguments (should fail)
run_test "PMI missing arguments" "$CLI_BIN pmi" 106

# Test 10: PMI command with basic arguments
run_test "PMI basic" "$CLI_BIN pmi $TEMP_DIR/pmi_input.txt $TEMP_DIR/pmi_output.tsv" 0

# Test 11: PMI command with options
run_test "PMI with options" "$CLI_BIN pmi $TEMP_DIR/pmi_input.txt $TEMP_DIR/pmi_output_options.tsv --n 2 --top 10 --min-freq 1 --threads 2 --progress none" 0

# Test 12: PMI command with quiet flag
run_test "PMI with quiet flag" "$CLI_BIN pmi -q $TEMP_DIR/pmi_input.txt $TEMP_DIR/pmi_output_quiet.tsv" 109

# Test 13: Normalize with non-existent input file (should fail)
run_test "Normalize non-existent input" "$CLI_BIN normalize non_existent_file.tsv $TEMP_DIR/normalize_output_error.tsv" 105

# Test 14: PMI with non-existent input file (should fail)
run_test "PMI non-existent input" "$CLI_BIN pmi non_existent_file.txt $TEMP_DIR/pmi_output_error.tsv" 105

# Test 15: Normalize with invalid form option (should fail)
run_test "Normalize invalid form" "$CLI_BIN normalize $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output_error.tsv --form INVALID" 105

# Test 16: PMI with invalid n-gram size (should fail)
run_test "PMI invalid n-gram size" "$CLI_BIN pmi $TEMP_DIR/pmi_input.txt $TEMP_DIR/pmi_output_error.tsv --n 4" 105

# Test 17: Normalize with stats-json flag
run_test "Normalize with stats-json" "$CLI_BIN normalize --stats-json $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output_stats.tsv" 0

# Test 18: PMI with stats-json flag
run_test "PMI with stats-json" "$CLI_BIN pmi --stats-json $TEMP_DIR/pmi_input.txt $TEMP_DIR/pmi_output_stats.tsv" 0

# Test 19: Normalize with stats-json and quiet flags
run_test "Normalize with stats-json and quiet" "$CLI_BIN normalize --stats-json -q $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output_stats_quiet.tsv" 109

# Test 20: Normalize with stdin/stdout
run_test "Normalize with stdin/stdout" "cat $TEMP_DIR/normalize_input.tsv | $CLI_BIN normalize - - > $TEMP_DIR/normalize_output_stream.tsv" 0

# Test 21: Check if stdin/stdout output matches file output
run_test "Check stdin/stdout output" "diff $TEMP_DIR/normalize_output.tsv $TEMP_DIR/normalize_output_stream.tsv" 1

# Test 22: PMI with stdin/stdout
run_test "PMI with stdin/stdout" "cat $TEMP_DIR/pmi_input.txt | $CLI_BIN pmi - - > $TEMP_DIR/pmi_output_stream.tsv" 0

# Test 23: Check if PMI stdin/stdout output matches file output
run_test "Check PMI stdin/stdout output" "diff $TEMP_DIR/pmi_output.tsv $TEMP_DIR/pmi_output_stream.tsv" 1

# Test 24: Normalize with sample option
run_test "Normalize with sample" "$CLI_BIN normalize $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output_sample.tsv --sample 2" 0

# Test 25: Normalize with sample and stats-json
run_test "Normalize with sample and stats-json" "$CLI_BIN normalize --stats-json $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output_sample_stats.tsv --sample 2" 0

# Test 26: Normalize with min-length filter
run_test "Normalize with min-length" "$CLI_BIN normalize $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output_min_length.tsv --min-length 5" 0

# Test 27: Normalize with max-length filter
run_test "Normalize with max-length" "$CLI_BIN normalize $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output_max_length.tsv --max-length 20" 0

# Test 28: Normalize with both min-length and max-length filters
run_test "Normalize with min/max length" "$CLI_BIN normalize $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output_length_filters.tsv --min-length 5 --max-length 20" 0

# Test 29: Invalid min/max length relationship (min > max should fail)
run_test "Invalid min/max length" "$CLI_BIN normalize $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output_error.tsv --min-length 20 --max-length 10" 1

# Test 30: Invalid sample size (negative value should fail)
run_test "Invalid sample size" "$CLI_BIN normalize $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output_error.tsv --sample -10" 105

# Test 31: Non-existent input file with stdin/stdout
run_test "Non-existent input with stdin" "cat non_existent_file.txt | $CLI_BIN normalize - $TEMP_DIR/normalize_output_error.tsv" 0

# Test 32: Write to non-writable directory
mkdir -p $TEMP_DIR/readonly_dir
chmod 555 $TEMP_DIR/readonly_dir 2>/dev/null || true  # Make read-only, ignore errors on Windows
run_test "Write to non-writable directory" "$CLI_BIN normalize $TEMP_DIR/normalize_input.tsv $TEMP_DIR/readonly_dir/output.tsv" 1
chmod 755 $TEMP_DIR/readonly_dir 2>/dev/null || true  # Restore permissions for cleanup

# Print summary
echo -e "${YELLOW}=== Test Summary ===${NC}"
echo -e "${YELLOW}Total tests: ${TESTS_TOTAL}${NC}"
echo -e "${GREEN}Passed: ${TESTS_PASSED}${NC}"
echo -e "${RED}Failed: ${TESTS_FAILED}${NC}"

# Return non-zero exit code if any tests failed
if [ "$TESTS_FAILED" -gt 0 ]; then
    exit 1
fi

exit 0
