# Braggi

A modern programming language with a focus on simplicity, performance, and clear error messages.

## Building and Running

### Prerequisites

- CMake 3.10 or higher
- A C compiler supporting C11 (GCC or Clang recommended)
- Optional: libreadline-dev (for enhanced REPL experience)

### Build Instructions

```bash
# Create a build directory
mkdir -p build
cd build

# Configure
cmake ..

# Build
cmake --build .

# Run tests
ctest

# Run the REPL
./braggi-repl
```

### Installing readline (recommended)

For a better REPL experience with command history and line editing:

```bash
# On Ubuntu/Debian
sudo apt-get install libreadline-dev

# On Fedora
sudo dnf install readline-devel

# On macOS (using Homebrew)
brew install readline
```

## Project Structure

- `include/` - Header files
- `src/` - Source code
- `tests/` - Test cases
- `build/` - Build artifacts (created during build)

## Current Status

The project is in early development. Current features include:

- Source file handling
- REPL (with or without readline support)
- Basic testing framework

## Running Tests

```bash
# From the build directory
ctest

# To see verbose test output
ctest -V
```

## License

This project is open source and available under the [MIT License](LICENSE).

---

*"Y'all come back now, ya hear?" - The Braggi Team* 