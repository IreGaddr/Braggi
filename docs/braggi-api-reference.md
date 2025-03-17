# Braggi API Reference
## "Where the quantum meets the cowboy"

> "Documentation's like a good fence line - keeps everything where it should be, and lets folks know what's what." - Texas API Wisdom

## Table of Contents

1. [Core API](#1-core-api)
2. [Memory Regions & Regimes](#2-memory-regions--regimes)
3. [Entropy & WFCCC](#3-entropy--wfccc)
4. [Entity Component System](#4-entity-component-system)
5. [Code Generation](#5-code-generation)
6. [Tooling & Integration](#6-tooling--integration)

---

## 1. Core API

### 1.1 Initialization & Configuration

```c
// Initialize the Braggi compiler system
BraggiContext* braggi_init(const BraggiConfig* config);

// Configure compilation options
BraggiConfig braggi_config_create_default(void);
void braggi_config_set_target_arch(BraggiConfig* config, TargetArch arch);
void braggi_config_set_optimization_level(BraggiConfig* config, int level);

// Cleanup and resource management
void braggi_cleanup(BraggiContext* context);
```

### 1.2 Compilation Pipeline

```c
// Compile a source file to a target format
CompileResult braggi_compile_file(BraggiContext* context, const char* filename, 
                                  const char* output_filename);

// Compile source code from a string
CompileResult braggi_compile_string(BraggiContext* context, const char* source_code,
                                   const char* output_filename);

// Access compilation errors
ErrorList* braggi_get_errors(BraggiContext* context);
const char* braggi_error_get_message(Error* error);
SourcePosition braggi_error_get_position(Error* error);
```

### 1.3 Utility Functions

```c
// Version information
const char* braggi_get_version(void);
bool braggi_check_version_compatibility(const char* min_version);

// Diagnostics
void braggi_set_log_level(LogLevel level);
void braggi_set_log_callback(LogCallback callback);
```

## 2. Memory Regions & Regimes

### 2.1 Region Management

```c
// Create a new memory region
Region* braggi_region_create(const char* name, size_t size);

// Create a nested region
Region* braggi_region_create_nested(Region* parent, const char* name, size_t size);

// Region with specific regime
Region* braggi_region_create_with_regime(const char* name, size_t size, RegimeType regime);

// Destroy a region and all its allocations
void braggi_region_destroy(Region* region);

// Set active region for allocations
void braggi_region_set_active(Region* region);
Region* braggi_region_get_active(void);
```

### 2.2 Allocation Functions

```c
// Allocate memory in current region
void* braggi_alloc(size_t size);

// Allocate in specific region
void* braggi_region_alloc(Region* region, size_t size);

// Free allocation (only valid for some regime types)
void braggi_free(void* ptr);

// Region information
size_t braggi_region_get_used_size(Region* region);
size_t braggi_region_get_allocation_count(Region* region);
RegimeType braggi_region_get_regime(Region* region);
```

### 2.3 Periscope Operations

```c
// Create a periscope for extending an allocation's lifetime
Periscope* braggi_periscope_create(void* value, Region* target_region);

// Begin a periscope block
void braggi_periscope_begin(Periscope* periscope);

// End a periscope block
void braggi_periscope_end(Periscope* periscope);

// Periscope information
Region* braggi_periscope_get_source_region(Periscope* periscope);
Region* braggi_periscope_get_target_region(Periscope* periscope);
```

### 2.4 Regime Types

```c
// Regime type enumeration
typedef enum {
    REGIME_DEFAULT, // System default (usually SEQ)
    REGIME_FIFO,    // First-In-First-Out
    REGIME_FILO,    // First-In-Last-Out (Stack-like)
    REGIME_SEQ,     // Sequential with random accessibility
    REGIME_RAND,    // Random access pattern
    // Extended regime types
    REGIME_CONC,    // Concurrent access (thread-safe)
    REGIME_PART,    // Partitioned for parallel access
    REGIME_PIPE     // Pipeline-optimized
} RegimeType;

// Check if two regimes are compatible for periscope
bool braggi_regime_are_compatible(RegimeType source, RegimeType target);
```

## 3. Entropy & WFCCC

### 3.1 Entropy Field

```c
// Create a new entropy field for WFCCC
EntropyField* braggi_entropy_field_create(size_t width, size_t height);

// Configure entropy field options
void braggi_entropy_field_set_observation_strategy(EntropyField* field, 
                                                  ObservationStrategy strategy);

// Get entropy statistics
float braggi_entropy_field_get_total_entropy(EntropyField* field);
float braggi_entropy_field_get_min_entropy(EntropyField* field);
size_t braggi_entropy_field_get_undecided_count(EntropyField* field);

// Clean up resources
void braggi_entropy_field_destroy(EntropyField* field);
```

### 3.2 States & Cells

```c
// Create a new state
State* braggi_state_create(uint32_t state_id, const char* name);

// Add possible states to a cell
void braggi_cell_add_possible_state(Cell* cell, State* state);
void braggi_cell_add_possible_states(Cell* cell, State** states, size_t count);

// Cell operations
Cell* braggi_entropy_field_get_cell(EntropyField* field, size_t x, size_t y);
float braggi_cell_get_entropy(Cell* cell);
void braggi_cell_collapse(Cell* cell, State* state);
bool braggi_cell_is_collapsed(Cell* cell);
State* braggi_cell_get_state(Cell* cell);
```

### 3.3 Constraints

```c
// Create constraints between cells
Constraint* braggi_constraint_create(ConstraintType type);

// Add cells to a constraint
void braggi_constraint_add_cell(Constraint* constraint, Cell* cell);

// Define constraint rules
void braggi_constraint_set_validator(Constraint* constraint, 
                                    ConstraintValidator validator);

// Apply constraints to cells
void braggi_constraint_apply(Constraint* constraint, EntropyField* field);
bool braggi_constraint_is_satisfied(Constraint* constraint);
```

### 3.4 WFCCC Algorithm

```c
// Run the full WFCCC algorithm on an entropy field
bool braggi_wfccc_run(EntropyField* field);

// Step-by-step WFCCC for debugging
bool braggi_wfccc_step(EntropyField* field);
WFCCCState braggi_wfccc_get_state(EntropyField* field);

// Configure WFCCC parameters
void braggi_wfccc_set_max_steps(EntropyField* field, size_t max_steps);
void braggi_wfccc_set_seed(EntropyField* field, uint64_t seed);
```

## 4. Entity Component System

### 4.1 ECS World

```c
// Create a new ECS world
ECSWorld* braggi_ecs_world_create(size_t entity_capacity, size_t max_component_types);

// Create ECS world using a memory region
ECSWorld* braggi_ecs_create_world_with_region(Region* region, Region* entity_region, 
                                             RegimeType regime);

// Destroy an ECS world
void braggi_ecs_world_destroy(ECSWorld* world);

// Update all systems in the world
void braggi_ecs_update(ECSWorld* world, float delta_time);
```

### 4.2 Entities

```c
// Create a new entity
EntityID braggi_ecs_create_entity(ECSWorld* world);

// Destroy an entity and all its components
void braggi_ecs_destroy_entity(ECSWorld* world, EntityID entity);

// Check if an entity exists
bool braggi_ecs_entity_exists(ECSWorld* world, EntityID entity);
```

### 4.3 Components

```c
// Register a new component type
ComponentTypeID braggi_ecs_register_component(ECSWorld* world, size_t component_size);

// Register a component with constructor and destructor
ComponentTypeID braggi_ecs_register_component_type(ECSWorld* world, 
                                                 const ComponentTypeInfo* info);

// Add a component to an entity
void* braggi_ecs_add_component(ECSWorld* world, EntityID entity, 
                              ComponentTypeID component_type);

// Add a component with initial data
void braggi_ecs_add_component_data(ECSWorld* world, EntityID entity, 
                                  ComponentTypeID component_type, void* component_data);

// Remove a component from an entity
void braggi_ecs_remove_component(ECSWorld* world, EntityID entity, 
                                ComponentTypeID component_type);

// Get a component from an entity
void* braggi_ecs_get_component(ECSWorld* world, EntityID entity, 
                              ComponentTypeID component_type);

// Check if an entity has a component
bool braggi_ecs_has_component(ECSWorld* world, EntityID entity, 
                             ComponentTypeID component_type);
```

### 4.4 Systems

```c
// Create a new system
System* braggi_ecs_system_create(ComponentMask component_mask, 
                               SystemUpdateFunc update_func, void* user_data);

// Create a system with detailed info
System* braggi_ecs_create_system(const SystemInfo* info);

// Register a system with the world
void braggi_ecs_register_system(ECSWorld* world, System* system);

// Update a specific system
bool braggi_ecs_update_system(ECSWorld* world, System* system, float delta_time);

// Destroy a system
void braggi_ecs_system_destroy(System* system);
```

### 4.5 Queries

```c
// Create a query for entities with specific components
EntityQuery braggi_ecs_query_entities(ECSWorld* world, ComponentMask required_components);

// Get the next entity in a query
bool braggi_ecs_query_next(EntityQuery* query, EntityID* out_entity);

// Get all entities with specific components
Vector* braggi_ecs_get_entities_with_components(ECSWorld* world, 
                                              ComponentMask component_mask);
```

### 4.6 WFCCC-ECS Integration

```c
// Initialize the entropy-ECS integration
bool braggi_entropy_ecs_initialize(BraggiContext* context);

// Register entropy components
bool braggi_entropy_register_components(ECSWorld* world, ComponentTypeID* component_ids);

// Create entities for entropy elements
EntityID braggi_entropy_create_state_entity(ECSWorld* world, EntropyState* state, 
                                          uint32_t cell_id);
EntityID braggi_entropy_create_constraint_entity(ECSWorld* world, 
                                               EntropyConstraint* constraint);

// Apply WFCCC using ECS
bool braggi_entropy_apply_wfc_with_ecs(BraggiContext* context);

// Create WFCCC systems
System* braggi_entropy_create_sync_system(BraggiContext* context);
System* braggi_entropy_create_constraint_system(BraggiContext* context);
System* braggi_entropy_create_wfc_system(BraggiContext* context);
```

## 5. Code Generation

### 5.1 Code Generation Context

```c
// Create a code generation context
CodeGenContext* braggi_codegen_create_context(BraggiContext* context, TargetArch arch);

// Configure code generation options
void braggi_codegen_set_output_format(CodeGenContext* ctx, OutputFormat format);
void braggi_codegen_set_optimization_level(CodeGenContext* ctx, int level);
void braggi_codegen_enable_debug_info(CodeGenContext* ctx, bool enable);

// Generate code
bool braggi_codegen_generate(CodeGenContext* ctx, const char* output_filename);

// Clean up resources
void braggi_codegen_destroy_context(CodeGenContext* ctx);
```

### 5.2 Target Architectures

```c
// Target architecture enumeration
typedef enum {
    TARGET_ARCH_AUTO,    // Automatically detect
    TARGET_ARCH_X86_64,  // x86-64
    TARGET_ARCH_ARM,     // ARM 32-bit
    TARGET_ARCH_ARM64,   // ARM 64-bit (AArch64)
    TARGET_ARCH_WASM,    // WebAssembly
    TARGET_ARCH_BYTECODE // Braggi bytecode
} TargetArch;

// Output format enumeration
typedef enum {
    OUTPUT_FORMAT_OBJECT,     // Object file
    OUTPUT_FORMAT_EXECUTABLE, // Executable
    OUTPUT_FORMAT_ASSEMBLY,   // Assembly text
    OUTPUT_FORMAT_BYTECODE,   // Bytecode
    OUTPUT_FORMAT_LIBRARY     // Shared library
} OutputFormat;
```

### 5.3 Backend Management

```c
// Register architecture backends
void braggi_codegen_register_x86_64_backend(void);
void braggi_codegen_register_arm_backend(void);
void braggi_codegen_register_arm64_backend(void);
void braggi_codegen_register_wasm_backend(void);
void braggi_codegen_register_bytecode_backend(void);

// Check if a backend is available
bool braggi_codegen_is_backend_available(TargetArch arch);

// Get information about a backend
const char* braggi_codegen_get_backend_name(TargetArch arch);
const char* braggi_codegen_get_backend_version(TargetArch arch);
```

### 5.4 Code Generation Systems (ECS)

```c
// Register all code generation systems
void braggi_ecs_register_codegen_systems(ECSWorld* world);

// Create individual code generation systems
System* braggi_ecs_create_backend_init_system(void);
System* braggi_ecs_create_codegen_context_system(void);
System* braggi_ecs_create_code_generation_system(void);
System* braggi_ecs_create_code_output_system(void);

// Update individual code generation systems
void braggi_ecs_update_backend_init_system(ECSWorld* world, float delta_time);
void braggi_ecs_update_codegen_context_system(ECSWorld* world, float delta_time);
void braggi_ecs_update_code_generation_system(ECSWorld* world, float delta_time);
void braggi_ecs_update_code_output_system(ECSWorld* world, float delta_time);

// Process a single entity through all code generation systems
bool braggi_ecs_process_codegen_entity(ECSWorld* world, EntityID entity);
```

### 5.5 Register Allocation

```c
// Allocate registers for code generation
RegisterAllocationResult* braggi_codegen_allocate_registers(CodeGenContext* ctx);

// Get register allocation information
Register* braggi_codegen_get_register_for_value(RegisterAllocationResult* result, 
                                              ValueID value_id);

// Free register allocation result
void braggi_codegen_free_register_allocation(RegisterAllocationResult* result);
```

## 6. Tooling & Integration

### 6.1 REPL & Interpreter

```c
// Create a new REPL environment
REPLContext* braggi_repl_create(void);

// Evaluate code in the REPL
REPLResult braggi_repl_eval(REPLContext* repl, const char* code);

// Get result as string
const char* braggi_repl_get_result_string(REPLResult* result);

// Clean up REPL resources
void braggi_repl_destroy(REPLContext* repl);
```

### 6.2 Language Server Protocol

```c
// Initialize LSP server
LSPServer* braggi_lsp_create_server(void);

// Process LSP message
char* braggi_lsp_process_message(LSPServer* server, const char* message);

// Start LSP server on stdin/stdout
void braggi_lsp_run_server(LSPServer* server);

// Clean up LSP resources
void braggi_lsp_destroy_server(LSPServer* server);
```

### 6.3 Debugging Support

```c
// Initialize debugger
Debugger* braggi_debug_create(BraggiContext* context);

// Set breakpoint
BreakpointID braggi_debug_set_breakpoint(Debugger* debugger, 
                                       const char* filename, int line);

// Start debugging
void braggi_debug_start(Debugger* debugger);

// Step execution
void braggi_debug_step(Debugger* debugger);

// Continue execution
void braggi_debug_continue(Debugger* debugger);

// Get variable value
char* braggi_debug_get_variable(Debugger* debugger, const char* variable_name);

// Clean up debugger resources
void braggi_debug_destroy(Debugger* debugger);
```

### 6.4 Testing Framework

```c
// Create test suite
TestSuite* braggi_test_create_suite(const char* name);

// Add test to suite
void braggi_test_add_test(TestSuite* suite, const char* name, 
                         TestFunction func, void* user_data);

// Run tests
TestResult* braggi_test_run_suite(TestSuite* suite);

// Get test statistics
int braggi_test_get_passed_count(TestResult* result);
int braggi_test_get_failed_count(TestResult* result);
const char* braggi_test_get_failure_message(TestResult* result, int index);
```

### 6.5 Documentation Generation

```c
// Generate documentation for a Braggi project
bool braggi_docs_generate(const char* source_dir, const char* output_dir);

// Configure documentation options
void braggi_docs_set_format(DocsFormat format);
void braggi_docs_include_examples(bool include);
void braggi_docs_include_private_api(bool include);
```

---

## Appendix A: ECS Component Reference

### A.1 Code Representation Components

- **TokenComponent**: Represents a token in the source code
- **SourcePositionComponent**: Position information for tokens and AST nodes
- **SourceReferenceComponent**: Reference to the source file

### A.2 WFCCC State Components

- **EntropyCellComponent**: Represents a cell in the entropy field
- **StateComponent**: Represents the current state of a cell
- **PossibleStatesComponent**: Tracks possible states for a cell
- **ConstraintComponent**: Represents constraints between cells
- **ConstraintTargetComponent**: Links constraints to their target cells

### A.3 AST Components

- **ASTNodeComponent**: Represents a node in the abstract syntax tree
- **ASTReferenceComponent**: Links between AST nodes
- **TypeComponent**: Type information for expressions and declarations

### A.4 Region Components

- **RegionComponent**: Represents a memory region in the code
- **AllocationComponent**: Represents an allocation within a region
- **PeriscopeComponent**: Represents a periscope operation

### A.5 Code Generation Components

- **CodeGenInfoComponent**: Metadata for code generation
- **RegisterComponent**: Register allocation information
- **MemoryLocationComponent**: Memory location for variables
- **TargetArchComponent**: Target architecture configuration
- **CodeGenContextComponent**: Context for code generation
- **CodeBlobComponent**: Generated code or assembly

## Appendix B: ECS System Reference

### B.1 Tokenization & Parsing Systems

- **TokenizeSystem**: Converts source code to token entities
- **EntropyInitSystem**: Creates entropy cells for tokens
- **ConstraintSystem**: Creates constraint entities between cells
- **PropagationSystem**: Propagates constraints through cells
- **CollapseSystem**: Collapses cells to final states using WFCCC

### B.2 AST & Analysis Systems

- **ASTBuildSystem**: Constructs AST from collapsed cells
- **TypeCheckSystem**: Performs type checking on AST entities
- **RegionAnalysisSystem**: Analyzes regions and periscopes
- **FlowAnalysisSystem**: Analyzes control flow

### B.3 Code Generation Systems

- **BackendInitSystem**: Initializes appropriate code generator backend
- **CodeGenContextSystem**: Creates code generation contexts
- **RegisterAllocSystem**: Allocates registers for code entities
- **CodeGenerationSystem**: Generates code for AST entities
- **OutputSystem**: Writes final code to output files

---

*"The docs may seem longer than a Texas mile, but they'll save you more time than a shortcut through the back pasture."* - Irish-Texan Documentation Philosophy 