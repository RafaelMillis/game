#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
PINK='\033[0;35m'
NC='\033[0m' # No Color

# Test counter
TOTAL_TESTS=0
PASSED_TESTS=0

# Path to the executable and test directories
EXECUTABLE="../build/tanks_game"
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$(cd "${TEST_DIR}/../build" && pwd)"
ALGORITHM_CONFIG="${BUILD_DIR}/algorithm_types.txt"

# Save original algorithm configuration if it exists
if [ -f "$ALGORITHM_CONFIG" ]; then
    cp "$ALGORITHM_CONFIG" "${ALGORITHM_CONFIG}.bak"
fi

# Function to run a single test
run_test() {
    local test_name=$1
    local input_file=$2
    local expected_output=$3
    local actual_output="${BUILD_DIR}/output_${test_name}.txt"
    
    echo "Running test: $test_name"
    ((TOTAL_TESTS++))
    
    # Check if executable exists
    if [ ! -f "$EXECUTABLE" ]; then
        echo -e "${RED}Error: Executable not found at $EXECUTABLE${NC}"
        echo "Please make sure the game is built and placed in the build directory"
        exit 1
    fi
    
    # Run the game with the test input
    cd "$(dirname "$EXECUTABLE")" # Change to executable directory
    "./$(basename "$EXECUTABLE")" "${TEST_DIR}/${input_file}" > /dev/null 2>&1
    cd - > /dev/null # Return to original directory
    
    # Check if expected output file exists
    if [ ! -f "$expected_output" ]; then
        echo -e "${RED}Warning: Expected output file not found: $expected_output${NC}"
        echo "Creating expected output file from actual output"
        cp "${actual_output}" "$expected_output"
        return
    fi
    
    # Read first line of expected output to determine verification type
    read -r first_line < "$expected_output"
    
    case "$first_line" in
        "VERIFY_SHELL_EXHAUSTION")
            # Use Python to verify shell exhaustion
            if python3 - <<EOF
import sys
sys.path.append("${TEST_DIR}")
from verify_output import OutputVerifier
verifier = OutputVerifier()
with open("${actual_output}", 'r') as f:
    actual = f.read()
if verifier.verify_shell_exhaustion(actual):
    sys.exit(0)
else:
    print("Verification errors:", file=sys.stderr)
    for error in verifier.get_errors():
        print(f"  {error}", file=sys.stderr)
    sys.exit(1)
EOF
            then
                echo -e "${GREEN}✓ Test passed: $test_name${NC}"
                ((PASSED_TESTS++))
            else
                echo -e "${RED}✗ Test failed: $test_name${NC}"
                echo -e "${PINK}Test failed! Here are the differences:${NC}"
                echo -e "${BLUE}ACTUAL OUTPUT (your program's output):${NC}"
                cat "${actual_output}"
                echo
                echo -e "${BLUE}EXPECTED OUTPUT:${NC}"
                echo "1. Must contain 'Shoot' action"
                echo "2. Must end with 'Tie, both players have zero shells for 40 steps'"
            fi
            ;;
            
        "VERIFY_CHASING_WINS")
            # Use Python to verify chasing algorithm wins
            if python3 - <<EOF
import sys
sys.path.append("${TEST_DIR}")
from verify_output import OutputVerifier
verifier = OutputVerifier()
with open("${actual_output}", 'r') as f:
    actual = f.read()
if verifier.verify_chasing_wins(actual):
    sys.exit(0)
else:
    print("Verification errors:", file=sys.stderr)
    for error in verifier.get_errors():
        print(f"  {error}", file=sys.stderr)
    sys.exit(1)
EOF
            then
                echo -e "${GREEN}✓ Test passed: $test_name${NC}"
                ((PASSED_TESTS++))
            else
                echo -e "${RED}✗ Test failed: $test_name${NC}"
                echo -e "${PINK}Test failed! Here are the differences:${NC}"
                echo -e "${BLUE}ACTUAL OUTPUT (your program's output):${NC}"
                cat "${actual_output}"
                echo
                echo -e "${BLUE}EXPECTED OUTPUT:${NC}"
                echo "1. Must contain 'Shoot' action (tanks must engage in combat)"
                echo "2. Must end with 'Player 1 won with X tanks still alive'"
            fi
            ;;
            
        *)
            # Compare output with expected output
            if diff -q "${actual_output}" "$expected_output" > /dev/null; then
                echo -e "${GREEN}✓ Test passed: $test_name${NC}"
                ((PASSED_TESTS++))
            else
                echo -e "${RED}✗ Test failed: $test_name${NC}"
                echo -e "${PINK}Test failed! Here are the differences:${NC}"
                echo -e "${BLUE}ACTUAL OUTPUT (your program's output):${NC}"
                if [ ! -s "${actual_output}" ]; then
                    echo "<empty file>"
                else
                    cat "${actual_output}"
                fi
                echo
                echo -e "${BLUE}EXPECTED OUTPUT (what it should be):${NC}"
                cat "$expected_output"
                echo
                echo "Detailed diff (- means in actual but not expected, + means in expected but not actual):"
                diff "${actual_output}" "$expected_output"
            fi
            ;;
    esac
    echo "----------------------------------------"
}

# Function to run all tests in a directory
run_tests_in_dir() {
    local dir=$1
    local test_type=$2
    
    echo "Running $test_type tests..."
    echo "========================================"
    
    # Generate test cases for this directory
    if [ "$test_type" = "Game Logic" ]; then
        python3 "${TEST_DIR}/generate_test_cases.py"
    elif [ "$test_type" = "Algorithm Comparison" ]; then
        python3 "${TEST_DIR}/generate_algorithm_tests.py"
    fi
    
    for input_file in "${TEST_DIR}/${dir}"/*.txt; do
        if [[ -f "$input_file" ]]; then
            # Get test name from filename (without .txt extension)
            local test_name=$(basename "$input_file" .txt)
            local expected_output="${TEST_DIR}/expected_outputs/${test_name}_expected.txt"
            local relative_input_path="${dir}/$(basename "$input_file")"
            
            run_test "$test_name" "$relative_input_path" "$expected_output"
        fi
    done
}

# Main test execution
echo "Starting Tank Battle Test Suite"
echo "=============================="

# Check if executable exists before starting tests
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}Error: Executable not found at $EXECUTABLE${NC}"
    echo "Please make sure the game is built and placed in the build directory"
    exit 1
fi

# Run tests for each category
run_tests_in_dir "maps/file_system" "File System"
run_tests_in_dir "maps/header_errors" "Header Errors"
run_tests_in_dir "maps/dimension_mismatches" "Dimension Mismatches"
run_tests_in_dir "maps/content_edge_cases" "Content Edge Cases"
run_tests_in_dir "maps/game_logic" "Game Logic"
run_tests_in_dir "maps/algorithm_comparison" "Algorithm Comparison"

# Print summary
echo "=============================="
echo "Test Summary:"
echo "Total tests: $TOTAL_TESTS"
echo "Passed tests: $PASSED_TESTS"
echo "Failed tests: $((TOTAL_TESTS - PASSED_TESTS))"
if [ $TOTAL_TESTS -gt 0 ]; then
    echo "Success rate: $(( (PASSED_TESTS * 100) / TOTAL_TESTS ))%"
else
    echo "Success rate: 0%"
fi

# Restore original algorithm configuration if it existed
if [ -f "${ALGORITHM_CONFIG}.bak" ]; then
    mv "${ALGORITHM_CONFIG}.bak" "$ALGORITHM_CONFIG"
fi
