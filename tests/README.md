# Braggi Language Tests

This directory contains test files for the Braggi language, including:

- Standard library tests
- Language feature tests
- Edge case tests
- Performance benchmarks

## Running Tests

Tests can be run using the Braggi test harness:

```bash
./build/bin/braggi_test_harness --test-dir=./tests --output-dir=./test_output
```

Or you can run individual tests directly:

```bash
./build/bin/braggi_compiler tests/stdlib_test.bg -o stdlib_test
./stdlib_test
```

## Test Files

- `stdlib_test.bg` - Tests the core standard library functionality
- Add more test files here as they are created

## Contributing Tests

When adding new tests, make sure they:
1. Test one specific feature or area
2. Have clear assertions
3. Print useful diagnostics
4. Clean up after themselves

Happy testing, pardner! 🤠 