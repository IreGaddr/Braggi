# Braggi Compiler Testing System

This document describes the testing system for the Braggi compiler. The testing system is designed to verify that the compiler can parse and process files without crashing, and that it generates the correct output.

## Test Harness

The test harness is implemented in `tools/test_harness.c`. It performs the following steps for each test:

1. Tokenizes the source file
2. Applies constraints to the tokens
3. Generates output code
4. Compares the generated output with expected output

The test harness can be run with the following command:

```bash
./scripts/run_tests.sh
```

## Test Cases

Test cases are located in the `tests` directory. Each test case is a `.bg` file that contains Braggi code. The test harness will run each test case and verify that it compiles successfully.

Current test cases include:

- `simple_test.bg`: A simple "Hello, World" program
- `function_test.bg`: Tests function definitions and calls
- `control_flow_test.bg`: Tests control flow statements (if, while, etc.)
- `variable_test.bg`: Tests variable declarations and assignments
- `type_system_test.bg`: Tests the type system
- `error_cases_test.bg`: Tests error handling
- `regime_test.bg`: Tests regime declarations and usage
- `custom_regime_test.bg`: Tests custom regime declarations
- `region_test.bg`: Tests region declarations and usage
- `periscope_test.bg`: Tests periscope functionality
- `stdlib_test.bg`: Tests standard library functions
- `data_processing_test.bg`: Tests data processing functionality

## Expected Output

For each test case, there is an expected output file in the `tests/expected_outputs` directory. The expected output file has the same name as the test case, but with a `.out` extension. The test harness compares the generated output with the expected output to verify that the compiler is generating the correct code.

If the expected output file doesn't exist, the test will pass if compilation succeeds. This is useful for tests that are only checking that the compiler doesn't crash.

## Updating Expected Output

If you make intentional changes to the compiler that affect the generated output, you'll need to update the expected output files. You can do this by running:

```bash
./scripts/run_tests.sh --update-expected
```

This will run all the tests and update the expected output files with the current output.

## Source Tests

In addition to the test cases, there is a separate test program called `test_source` that tests the compiler's source code handling. This program is run as part of the test harness.

## Adding New Tests

To add a new test case:

1. Create a new `.bg` file in the `tests` directory
2. Run the tests with `./scripts/run_tests.sh`
3. If the test passes, update the expected output with `./scripts/run_tests.sh --update-expected`

## Debugging Tests

If a test fails, the test harness will print an error message and the test will be marked as failed. You can examine the generated output in the `build/test_outputs` directory and compare it with the expected output in the `tests/expected_outputs` directory.

The test harness also creates log files in the `build/test_outputs` directory for each test case. These log files contain the output of the diff command when comparing the generated output with the expected output.

## Wave Function Constraint Collapse

The Braggi compiler uses a technique called Wave Function Constraint Collapse for compilation. This technique is inspired by quantum mechanics and allows for flexible and powerful code generation. The testing system ensures that this technique is working correctly and generating the expected output.

---

"Where Wave Function Constraint Collapse meets compiler magic! 🤠" 