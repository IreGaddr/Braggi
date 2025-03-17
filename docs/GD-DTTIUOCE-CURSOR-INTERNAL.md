# Braggi Code Generation Dependency Tracking

"Y'all got to know what depends on what, or your code's gonna be messier than a rattlesnake in a piñata!" - Texas Code Wrangler

## Architecture Overview

```
                          +------------------+
                          |   CodeGenContext |
                          +------------------+
                                   |
                                   v
                          +------------------+
                          | CodeGen Manager  |
                          +------------------+
                                   |
                 +----------------------------------+
                 |                 |                |
        +------------------+  +-----------+  +------------+
        |  x86_64 Backend  |  | ARM Backend|  |ARM64 Backend|
        +------------------+  +-----------+  +------------+
                 |                 |                |
        +------------------+  +-----------+  +------------+
        | x86_64 ASM       |  | ARM ASM   |  |ARM64 ASM   |
        +------------------+  +-----------+  +------------+
```

## Key Files and Dependencies

### Header Files
- `include/braggi/codegen.h`: Main code generation interface for users
- `include/braggi/codegen_arch.h`: Architecture-specific code generation interface
- `include/braggi/error.h`: Error handling API used by code generators
- `include/braggi/entropy.h`: Entropy field and Wave Function Collapse algorithm
- `include/braggi/region.h`: Memory region management used by code generators

### Implementation Files
- `src/codegen/codegen.c`: Main code generation implementation
- `src/codegen/codegen_manager.c`: Manager for multiple architecture backends
- `src/codegen/x86_64.c`: x86_64 architecture implementation
- `src/codegen/arm.c`: ARM (32-bit) architecture implementation
- `src/codegen/arm64.c`: ARM64 (AArch64) architecture implementation
- `src/codegen/wasm.c`: WebAssembly implementation (future)
- `src/codegen/bytecode.c`: Bytecode generation (future)

## Dependency Chain

1. **User Code** -> `braggi_codegen_init()` -> Creates a `CodeGenContext` with architecture options
2. **CodeGenContext** -> `braggi_codegen_generate()` -> Delegates to `codegen_manager.c`
3. **CodeGen Manager** -> Selects appropriate architecture backend
4. **Architecture Backend** -> Generates assembly or binary code
5. **Output** -> Written to file by architecture backend

## Data Flow

1. `BraggiContext` provides token stream from WFC algorithm
2. `CodeGenContext` holds architecture-specific options and pointers
3. Architecture backend processes tokens and generates code
4. Generated code is emitted to a file in the specified format

## Critical Dependencies

- All backends depend on the `CodeGenerator` struct from `codegen_arch.h`
- All backends must implement initialization functions: `braggi_register_ARCH_backend()`
- All backends use the same interface but have architecture-specific implementations
- `codegen_manager.c` orchestrates all backends and provides error handling

## Backend Registration Process

1. During initialization, `braggi_codegen_manager_init()` registers available backends
2. Each backend implements `braggi_register_ARCH_backend()` to create a `CodeGenerator` struct
3. The manager maintains a list of registered backends and selects appropriate one by architecture

## Error Handling Chain

1. Architecture backends report errors through `ErrorHandler` provided by manager
2. Manager aggregates errors and passes them up to main code generation module
3. User code receives errors through return values or dedicated functions

## Future Extensions

- Support for WASM backend
- Support for custom bytecode generation
- Support for target-specific optimizations

"Remember, pardner, understanding your dependencies is like knowin' where all the fences are on your ranch - keeps everything running smooth!" - Texas Code Ranch wisdom 