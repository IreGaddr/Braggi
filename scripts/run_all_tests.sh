#!/bin/bash

# Braggi Test Script
# "Testing code is like riding a bull - you gotta check every angle or you'll end up face-down in the dirt!"

# Set some pretty colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Track test failures
FAILED_TESTS=0
TOTAL_TESTS=0
SKIPPED_TESTS=0

# Process command line arguments
VERBOSE=0
for arg in "$@"; do
    if [ "$arg" == "--verbose" ] || [ "$arg" == "-v" ]; then
        VERBOSE=1
    fi
done

# Helper function to run a test and track results
run_test() {
    local test_name="$1"
    local test_cmd="$2"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -e "${BLUE}Running test: ${CYAN}${test_name}${NC}"
    echo -e "${PURPLE}Command: ${test_cmd}${NC}"
    
    # Capture the output in case we need it
    local output
    local start_time=$(date +%s.%N)
    
    if [ $VERBOSE -eq 1 ]; then
        # Run the test command with output displayed
        if eval "$test_cmd"; then
            local end_time=$(date +%s.%N)
            local duration=$(echo "$end_time - $start_time" | bc)
            echo -e "${GREEN}✓ Test passed: ${test_name} ${YELLOW}(in ${duration}s)${NC}"
            echo ""
        else
            local end_time=$(date +%s.%N)
            local duration=$(echo "$end_time - $start_time" | bc)
            echo -e "${RED}✗ Test failed: ${test_name} ${YELLOW}(in ${duration}s)${NC}"
            FAILED_TESTS=$((FAILED_TESTS + 1))
            echo ""
        fi
    else
        # Run the test command, capturing output
        output=$(eval "$test_cmd" 2>&1)
        local result=$?
        local end_time=$(date +%s.%N)
        local duration=$(echo "$end_time - $start_time" | bc)
        
        if [ $result -eq 0 ]; then
            echo -e "${GREEN}✓ Test passed: ${test_name} ${YELLOW}(in ${duration}s)${NC}"
            echo ""
        else
            echo -e "${RED}✗ Test failed: ${test_name} ${YELLOW}(in ${duration}s)${NC}"
            echo -e "${YELLOW}Test output:${NC}"
            echo "$output" | head -n 15 # Show the first 15 lines of output
            if [ $(echo "$output" | wc -l) -gt 15 ]; then
                echo -e "${YELLOW}... (output truncated, run with --verbose for full output)${NC}"
            fi
            FAILED_TESTS=$((FAILED_TESTS + 1))
            echo ""
        fi
    fi
}

echo -e "${BLUE}=======================================${NC}"
echo -e "${BLUE}     BRAGGI TEST SUITE     ${NC}"
echo -e "${BLUE}     Roundin' up them bugs...     ${NC}"
echo -e "${BLUE}=======================================${NC}"

# Determine the project root directory
PROJECT_ROOT=$(dirname "$(dirname "$(readlink -f "$0")")")
BUILD_DIR="${PROJECT_ROOT}/build"

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Build directory not found at ${BUILD_DIR}${NC}"
    echo -e "${YELLOW}Please run the build script first:${NC}"
    echo -e "${YELLOW}  ${PROJECT_ROOT}/scripts/run_build.sh${NC}"
    exit 1
fi

# Move to build directory
cd "$BUILD_DIR" || { 
    echo -e "${RED}Failed to cd into ${BUILD_DIR}${NC}"
    exit 1
}

echo -e "${YELLOW}Checking for test binaries...${NC}"

# Create bin directory if it doesn't exist
if [ ! -d "${BUILD_DIR}/bin" ]; then
    echo -e "${YELLOW}Creating bin directory${NC}"
    mkdir -p "${BUILD_DIR}/bin"
fi

# Check if test binaries exist
if [ ! -f "${BUILD_DIR}/bin/braggi_test_harness" ]; then
    echo -e "${RED}Test harness not found at ${BUILD_DIR}/bin/braggi_test_harness${NC}"
    echo -e "${YELLOW}It seems the project hasn't been fully built.${NC}"
    echo -e "${YELLOW}Please run the build script first:${NC}"
    echo -e "${YELLOW}  ${PROJECT_ROOT}/scripts/run_build.sh${NC}"
    exit 1
fi

if [ ! -f "${BUILD_DIR}/bin/test_source" ]; then
    echo -e "${YELLOW}Source test not found at ${BUILD_DIR}/bin/test_source${NC}"
    echo -e "${YELLOW}Skipping source tests.${NC}"
    SKIPPED_TESTS=$((SKIPPED_TESTS + 1))
fi

# Create test output directory if it doesn't exist
mkdir -p "${BUILD_DIR}/test_output"

# Make sure tests directory exists
if [ ! -d "${BUILD_DIR}/tests" ]; then
    echo -e "${YELLOW}Creating tests directory${NC}"
    mkdir -p "${BUILD_DIR}/tests"
    
    # Copy test files if needed
    if [ -d "${PROJECT_ROOT}/tests" ]; then
        echo -e "${YELLOW}Copying test files from ${PROJECT_ROOT}/tests${NC}"
        cp -r "${PROJECT_ROOT}/tests/"*.bg "${BUILD_DIR}/tests/" 2>/dev/null || true
    else
        echo -e "${YELLOW}No tests directory found at ${PROJECT_ROOT}/tests${NC}"
        echo -e "${YELLOW}You may need to create some test files.${NC}"
    fi
fi

# Check if we have any test files
TEST_FILES_COUNT=$(find "${BUILD_DIR}/tests" -name "*.bg" 2>/dev/null | wc -l)
if [ "$TEST_FILES_COUNT" -eq 0 ]; then
    echo -e "${YELLOW}No .bg test files found in ${BUILD_DIR}/tests${NC}"
    echo -e "${YELLOW}Test harness will run but may not find any tests to run.${NC}"
fi

echo -e "${GREEN}Starting tests...${NC}"
echo ""

# Run source tests if available
if [ -f "${BUILD_DIR}/bin/test_source" ]; then
    run_test "Source Tests" "${BUILD_DIR}/bin/test_source"
fi

# Run test harness
run_test "Braggi Test Harness" "${BUILD_DIR}/bin/braggi_test_harness --test-dir=${BUILD_DIR}/tests --output-dir=${BUILD_DIR}/test_output $([ $VERBOSE -eq 1 ] && echo '--verbose')"

# Check if any other binaries in the bin directory contain "test"
echo -e "${YELLOW}Looking for additional test binaries...${NC}"
OTHER_TEST_BINS=0
for test_bin in "${BUILD_DIR}/bin/"*test*; do
    if [ -f "$test_bin" ] && [ -x "$test_bin" ] && 
       [ "$test_bin" != "${BUILD_DIR}/bin/test_source" ] && 
       [ "$test_bin" != "${BUILD_DIR}/bin/braggi_test_harness" ]; then
        test_name=$(basename "$test_bin")
        run_test "$test_name" "$test_bin"
        OTHER_TEST_BINS=$((OTHER_TEST_BINS + 1))
    fi
done

if [ "$OTHER_TEST_BINS" -eq 0 ]; then
    echo -e "${YELLOW}No additional test binaries found.${NC}"
fi

# Display test summary
echo -e "${BLUE}=======================================${NC}"
echo -e "${BLUE}     TEST SUMMARY     ${NC}"
echo -e "${BLUE}=======================================${NC}"
echo -e "Total tests: ${TOTAL_TESTS}"
echo -e "Passed: ${GREEN}$((TOTAL_TESTS - FAILED_TESTS))${NC}"
echo -e "Failed: ${RED}${FAILED_TESTS}${NC}"

if [ "$SKIPPED_TESTS" -gt 0 ]; then
    echo -e "Skipped: ${YELLOW}${SKIPPED_TESTS}${NC}"
fi

# Set exit code based on test results
if [ "$FAILED_TESTS" -eq 0 ]; then
    echo -e "${GREEN}All tests passed! Slicker than a greased pig at a county fair!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed. Looks like we've got some bug wranglin' to do, partner.${NC}"
    exit 1
fi 