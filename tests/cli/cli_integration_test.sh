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
CLI_BIN="../../build/bin/suzume_feedmill_cli"

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
run_test "Invalid command" "$CLI_BIN invalid-command" 1

# Test 5: Normalize command with missing arguments (should fail)
run_test "Normalize missing arguments" "$CLI_BIN normalize" 1

# Test 6: Normalize command with basic arguments
run_test "Normalize basic" "$CLI_BIN normalize $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output.tsv" 0

# Test 7: Normalize command with options
run_test "Normalize with options" "$CLI_BIN normalize $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output_options.tsv --form NFKC --bloom-fp 0.05 --threads 2 --progress none" 0

# Test 8: Normalize command with quiet flag
run_test "Normalize with quiet flag" "$CLI_BIN normalize -q $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output_quiet.tsv" 0

# Test 9: PMI command with missing arguments (should fail)
run_test "PMI missing arguments" "$CLI_BIN pmi" 1

# Test 10: PMI command with basic arguments
run_test "PMI basic" "$CLI_BIN pmi $TEMP_DIR/pmi_input.txt $TEMP_DIR/pmi_output.tsv" 0

# Test 11: PMI command with options
run_test "PMI with options" "$CLI_BIN pmi $TEMP_DIR/pmi_input.txt $TEMP_DIR/pmi_output_options.tsv --n 2 --top 10 --min-freq 1 --threads 2 --progress none" 0

# Test 12: PMI command with quiet flag
run_test "PMI with quiet flag" "$CLI_BIN pmi -q $TEMP_DIR/pmi_input.txt $TEMP_DIR/pmi_output_quiet.tsv" 0

# Test 13: Normalize with non-existent input file (should fail)
run_test "Normalize non-existent input" "$CLI_BIN normalize non_existent_file.tsv $TEMP_DIR/normalize_output_error.tsv" 1

# Test 14: PMI with non-existent input file (should fail)
run_test "PMI non-existent input" "$CLI_BIN pmi non_existent_file.txt $TEMP_DIR/pmi_output_error.tsv" 1

# Test 15: Normalize with invalid form option (should fail)
run_test "Normalize invalid form" "$CLI_BIN normalize $TEMP_DIR/normalize_input.tsv $TEMP_DIR/normalize_output_error.tsv --form INVALID" 1

# Test 16: PMI with invalid n-gram size (should fail)
run_test "PMI invalid n-gram size" "$CLI_BIN pmi $TEMP_DIR/pmi_input.txt $TEMP_DIR/pmi_output_error.tsv --n 4" 1

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
