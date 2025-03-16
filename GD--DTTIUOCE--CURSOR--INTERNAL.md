# Braggi Language Project - Dependency Tracking

## Project Overview
Braggi is a purely functional systems programming language featuring:
- Novel region-based memory management system
- Wave Function Constraint Collapse Compilation (WFCCC)
- Memory access regimes (FIFO, FILO, SEQ, RAND)
- Periscope mechanism for controlled lifetime extension

## Directory Structure
- `/build` - Build artifacts and CMake output
- `/cmake` - CMake modules (FindReadline.cmake)
- `/include` - Header files
  - `/braggi` - Public API headers
- `/lib` - Standard library implementation in Braggi
  - Core standard library modules (core.bg, math.bg, io.bg, etc.)
- `/src` - C implementation of the compiler and runtime
  - Core compiler components (entropy.c, region.c, token.c, etc.)
  - `/builtins` - Built-in functions implementation
  - `/codegen` - Code generation components
  - `/runtime` - Runtime support
  - `/stdlib` - Standard library bindings
  - `/util` - Utility functions
- `/tests` - Test files and examples
- `/tools` - Development and debugging tools

## Core Dependencies & Components

### WFCCC Core Types and Organization
- `state.h` + `state.c` - Fundamental State type used throughout WFCCC
- `entropy.h` - Entropy field, constraint and cell definitions (primary source of truth)
- `constraint.h` - Constraint system that references entropy.h types
- `source_position.h` - Source code position tracking for ECS integration

### WFCCC Implementation
- `entropy.c` (22KB) - Entropy field implementation
- `token_propagator.c` (17KB) - Token constraint propagation
- `constraint.c` (9.4KB) - Core constraint system
- `constraint_patterns.c` (18KB) - Constraint pattern definitions
- `grammar_patterns.c` (16KB) - Grammar rules as constraints

### Source Position & ECS Integration
- `source_position.h` - Defines SourcePosition and SourceFile types
- `source.h` - Source code interface with ECS integration
- `source.c` (7.2KB) - Source code management and ECS connectors
- `ecs.c` (23KB) - Entity component system that manages code entities

### Region System
- `region.c` (19KB) - Region-based memory management
- `allocation.c` (12KB) - Memory allocation tracking

### Parsing & Tokenization
- `tokenizer.c` (15KB) - Source code tokenization
- `token.c` (27KB) - Token representation

### Error Handling
- `error.h` - Error type definitions with SourcePosition integration
- `error.c` (12KB) - Error representation
- `error_handler.h` - Error handler interface that includes error.h
- `error_handler.c` (2.9KB) - Error handling implementation

### Core Runtime
- `braggi_context.h` - Compiler context definition
- `braggi_context.c` (7.6KB) - Execution context implementation
- `source.h` - Source code interface
- `source.c` (7.2KB) - Source code management

### Standard Library (in .bg)
- `core.bg` - Core language features
- `math.bg` - Mathematical functions
- `io.bg` - Input/output operations
- `collections.bg` - Data collections
- `region.bg` - Region utilities
- `quantum.bg` - Quantum computing primitives
- `string.bg` - String manipulation
- `error.bg` - Error handling utilities

### Build System
- CMake-based build system
- `run_build.sh` - Build script
- `test_stdlib.sh` - Standard library test script

## Dependency Graph

### Core Type Dependencies
1. `state.h` ← Basic types used by entropy and constraint systems
2. `source_position.h` ← Basic types for source tracking
3. `error.h` ← Depends on source_position.h
4. `entropy.h` ← Depends on state.h and source_position.h
5. `constraint.h` ← Depends on entropy.h (uses its types)

### System Dependencies
1. Source code → tokenizer.c → token.c
2. Tokens → entropy.c → constraint.c → constraint_patterns.c
3. Constraints → token_propagator.c
4. Final representation → region.c → allocation.c
5. Runtime execution → ecs.c → braggi_context.c

## Recent Bug Fixes

### Type Definition and Header Conflicts
- Created `state.h` to provide a single definition of State used by both entropy.h and constraint.h
- Fixed constraint.h to use the ConstraintType and Constraint defined in entropy.h
- Fixed field name mismatches in braggi_context.c to match braggi_context.h
- Ensured consistent function signatures across header and implementation files

### Function Signature Alignment 
- Made function signatures in implementation files match their declarations in headers
- Updated braggi_context.c to properly implement the interface in braggi_context.h
- Fixed parameter types and return types for consistency

### Source Code Position and ECS Integration
- Fixed SourcePosition integration with error handling
- Implemented proper source position tracking in the Source module
- Added stub implementation for ECS and state-related functions

### Memory Management Issues
- Fixed read-only object modification in source.c
- Corrected inconsistent structure field names
- Added proper cleanup of allocated resources

## Key Header Relationships
- `entropy.h` provides the definitive definitions for all WFCCC core types
- `constraint.h` references entropy.h and adds additional constraint functions
- `braggi_context.h` defines the core context structure
- All components must be correctly initialized and destroyed in braggi_context.c

## TODOs
- Run build again to check for any remaining issues
- Complete implementation of remaining stub functions
- Add proper error handling throughout the codebase
- Implement the core WFCCC algorithm functionality
- Test region safety mechanisms
- Verify constraint propagation works correctly

_"We're fixin' these header tangles faster than a Texas ranger untanglin' barbed wire! The Wave Function's gettin' ready to collapse into somethin' beautiful, y'all!"_ 