# Braggi

> *"Poetry in motion; if error your syntax, then no collapse"*

Braggi is a modern systems programming language with a revolutionary approach to memory management, error handling, and concurrency. Instead of following the traditional lexer → parser → AST pipeline, Braggi reframes language processing as an entropy reduction problem with syntax and grammar constraints.

## Language Philosophy

Braggi was born from the desire to create a language that's both powerful and approachable, combining:

- **Memory Safety Without Garbage Collection**: Region-based memory management with lifetime constraints
- **Expressive Error Handling**: Clear, actionable error messages with cultural flair
- **Wave Function Collapse**: Innovative constraint-based approach to syntax and semantics
- **Region Semantics**: First-class regions providing control over memory without manual management

## Key Features

### Region-Based Memory Management
```braggi
region GameLoop {
    entities: Vec<Entity> = Vec::new();
    
    // Memory automatically freed when region ends
    fn process() {
        let temp = region Temporary {
            // Sub-region for short-lived allocations
        }
    }
}
```

### Wave Function Collapse Constraint System
Braggi uses a Wave Function Collapse (WFC) algorithm to resolve code constructs. This innovative approach allows for:

- More natural error messages
- Gradual type resolution
- Context-aware syntax features

```braggi
// The compiler collapses possibilities as it processes code
func calculate(value) {
    // Type determined by constraints and usage
    result = value * 2
    return result // Return type propagates upward
}
```

### Entropy Reduction Programming Model
Instead of traditional parsing, Braggi collapses a field of possibilities into valid code through constraint propagation, leading to better error messages and more flexible syntax.

## Building and Running

### Prerequisites

- CMake 3.10 or higher
- A C compiler supporting C11 (GCC or Clang recommended)
- Optional: libreadline-dev (for enhanced REPL experience)

### Build Instructions

```bash
# Simple build script
./run_build.sh

# Or manual build
mkdir -p build
cd build
cmake ..
cmake --build .

# Run the REPL
./bin/braggi-repl
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
  - `src/region.c` - Memory region management
  - `src/error.c` - Error handling with personality
  - `src/ecs.c` - Internal Entity Component System for constraint processing
- `tests/` - Test cases with example Braggi code
- `tools/` - Development tools including REPL
- `lib/` - Standard library in Braggi

## Current Status

Braggi 0.0.1 is our initial build with core functionality working. The language is in active development with:

- ✅ Memory region implementation
- ✅ Internal ECS for WFCCC processing
- ✅ Error handling system
- ✅ Basic REPL
- 🔄 Standard library
- 🔄 Self-hosting compiler

## Contributing

Braggi welcomes contributors with an appreciation for both elegant design and practical solutions.

## License

This project is open source and available under the [MIT License](LICENSE).

---

*"Good code is like a good story - it has a beginning, a middle, and no unexpected segfaults!" - Irish-Texan Programming Wisdom* 