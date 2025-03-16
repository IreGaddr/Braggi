# Braggi Language: Integrated Specification
## "Poetry in Motion"
If the wave can't collapse, there's an error in the syntax.
## 1. Introduction

Braggi is a purely functional systems programming language featuring a novel region-based memory management system and a revolutionary compilation approach called Wave Function Constraint Collapse Compilation (WFCCC). The combination of these two innovations creates a language that provides memory safety without garbage collection while offering precise error detection and reporting.

### 1.1 Design Philosophy

Braggi is designed around two core principles:

1. **Memory management should be both expressive and safe**. Rather than relying on garbage collection or manual memory management, Braggi introduces a region-based approach with explicitly declared access patterns (regimes).

2. **Compilation should be a unified process of constraint application**. Instead of sequential phases of lexing, parsing, and analysis, Braggi treats compilation as an entropy reduction problem through constraint propagation.

### 1.2 Key Innovations

- **Region-Based Memory Management**: Explicit memory regions with controlled lifetimes
- **Memory Access Regimes**: Declared patterns for memory access (FIFO, FILO, SEQ, RAND)
- **Periscope Mechanism**: Controlled lifetime extension with compile-time safety verification
- **Wave Function Constraint Collapse Compilation**: Unified approach to program analysis

## 2. Memory Model: Regions & Regimes

### 2.1 Regions

A region is a logical grouping of memory allocations with a common lifetime. Regions are lexically scoped by default.

```
region MyData {
    // Allocations within this region share a common lifetime
    let x = 42;
    let y = "hello";
}
// x and y are deallocated when the region ends
```

Regions can be nested, creating a hierarchy of lifetimes:

```
region Outer {
    let a = 1;
    
    region Inner {
        let b = 2;
        // Both a and b are accessible here
    }
    // Only a is accessible here, b has been deallocated
}
// Both a and b are out of scope
```

### 2.2 Regimes

A regime specifies the access pattern for a memory region, determining how data within the region can be accessed and modified.

| Regime | Description |
|--------|-------------|
| FIFO   | First-In-First-Out access pattern. Elements must be processed in the order they were created. |
| FILO   | First-In-Last-Out access pattern (stack-like behavior). |
| SEQ    | Sequential access pattern. Elements are accessed in order, but with random accessibility. |
| RAND   | Random access pattern. Elements can be accessed in any order. |

Example of region with regime declaration:

```
region Queue regime FIFO {
    let a = 1;
    let b = 2;
    let c = 3;
    // a must be accessed before b, which must be accessed before c
}
```

### 2.3 Periscope Mechanism

The periscope mechanism allows values from one region to escape or extend their lifetimes into another region, with compile-time verification of safety constraints.

```
region Source regime SEQ {
    let value = compute_something();
    
    periscope value to Target {
        // value's lifetime is extended into the Target region
    }
}

region Target regime FIFO {
    // value can be used here
}
```

### 2.4 Regime Compatibility Matrix

The compatibility between regimes determines whether a periscope operation is allowed. This is verified at compile time.

| From ↓ To → | FIFO | FILO | SEQ | RAND |
|-------------|------|------|-----|------|
| FIFO        | ✓    | ✓    | ✓   | ✗    |
| FILO        | ✗    | ✓    | ✗   | ✗    |
| SEQ         | ✓    | ✓    | ✓   | ✓    |
| RAND        | ✗    | ✗    | ✓   | ✓    |

### 2.5 Region Lifetime Rules

For any region R with lifetime L_R, and any value v allocated in R:
- lifetime(v) ≤ L_R

For nested regions R₁ and R₂ where R₂ is nested inside R₁:
- L_R₂ ⊂ L_R₁

For a periscope operation from region R_S with regime ρ_S to region R_T with regime ρ_T:
- The operation is valid if and only if (ρ_S, ρ_T) is compatible according to the matrix
- The periscope operation creates a contract that extends the lifetime of the value

## 3. Wave Function Constraint Collapse Compilation (WFCCC)

### 3.1 Core Concept

WFCCC treats program analysis as an entropy reduction problem:
- Source code begins in a superposition of possible interpretations
- Constraints (grammar rules, type requirements, region rules) act as "measurements"
- The application of constraints causes possibilities to collapse until only valid interpretations remain

### 3.2 Entropy as Ambiguity

In the WFCCC model:
- High entropy = many possible interpretations of a code segment
- Low entropy = few possible interpretations
- Zero entropy = exactly one valid interpretation

The compilation process is framed as an entropy reduction problem, with the goal of reducing the entire program's entropy to zero.

### 3.3 Core WFCCC Algorithm

1. **Entropy Field Initialization**
   - Source code is converted into a grid of "cells"
   - Each cell represents a character or token with multiple possible interpretations
   - Initial state assigns all grammatically possible interpretations to each cell

2. **Constraint Application**
   - Syntactic constraints (grammar rules)
   - Semantic constraints (type rules, name resolution)
   - Region constraints (lifetime rules, regime compatibility)

3. **Collapse Propagation**
   - When a cell's possibilities are reduced, the constraints propagate to affected cells
   - Propagation continues until the system reaches a stable state

4. **Observation Step**
   - Select the cell with lowest entropy (fewest remaining options)
   - Collapse it to a single interpretation
   - Propagate the consequences through constraints

5. **Iteration**
   - Repeat observation and propagation until the entire program collapses
   - Terminate with success (single valid program) or failure (contradiction/error)

### 3.4 Propagation Starting From Main

The WFCCC process begins with the program entry point (main function):
- Initial constraints are applied to the main function
- As main function collapses, constraints propagate to called functions
- Function calls create constraint relationships between caller and callee
- Region and regime information propagates through function boundaries
- The entire program gradually collapses in a wave-like pattern starting from main

### 3.5 Constraint Types for Regions

Region-specific constraints include:
- Region containment (regions properly nested)
- Lifetime validity (no use after free)
- Regime compatibility (correct periscope operations)
- Access pattern enforcement (following regime rules)

These constraints propagate through the program during the WFCCC process.

## 4. Language Features

### 4.1 Type System

Braggi features a strong, static type system with:
- Algebraic data types
- Parametric polymorphism
- Region-aware types that track allocation context

### 4.2 Core Syntax

```
// Function definition
fn add(x: Int, y: Int) -> Int {
    x + y
}

// Region and regime declaration
region Data regime SEQ {
    let values = [1, 2, 3, 4, 5];
    let doubled = map(values, fn(x) { x * 2 });
}

// Conditional expressions
let max = if a > b { a } else { b };

// Pattern matching
match option {
    Some(value) => process(value),
    None => default_value
}
```

### 4.3 Region-Specific Syntax

```
// Basic region
region BasicRegion {
    let x = compute();
}

// Region with regime
region QueueRegion regime FIFO {
    let first = 1;
    let second = 2;
    // first must be processed before second
}

// Periscope operation
region Source regime SEQ {
    let value = 42;
    
    periscope value to Target {
        // value's lifetime is extended
    }
}

// Multiple nested regions
region Outer regime RAND {
    let a = 1;
    
    region Middle regime SEQ {
        let b = 2;
        
        region Inner regime FIFO {
            let c = 3;
            // a, b, and c accessible here
        }
        // a and b accessible here
    }
    // only a accessible here
}
```

## 5. Error Handling and Reporting

### 5.1 Error Types

The WFCCC approach provides detailed error information:
1. **Collapse Failure**: The algorithm reaches a cell with zero possible states
2. **Ambiguous Collapse**: Multiple valid interpretations remain after all constraints
3. **Constraint Violation**: A specific constraint cannot be satisfied
4. **Region Violation**: A region-specific rule is violated (e.g., incompatible regimes)

### 5.2 Error Reporting

For the error in `periscope value from FILO to FIFO`:

```
ERROR: Collapse failure
Location: Line 12, Column 5-28
Details: Cannot collapse "periscope value to Target" to a valid interpretation
Violated constraint: Regime compatibility (FILO cannot periscope to FIFO)
Suggestion: Consider changing the source regime to SEQ or the target regime to FILO
```

## 6. Implementation Strategy

### 6.1 Entropy Field Data Structure

```
struct Cell {
    id: usize,
    position: SourcePosition,
    possibleStates: ArrayList(State),
    neighbors: ArrayList(usize), // IDs of neighboring cells
    
    fn entropy(self) -> f64 {
        -log2(self.possibleStates.len())
    }
    
    fn collapse(self, stateIndex: usize) {
        let selectedState = self.possibleStates[stateIndex];
        self.possibleStates.clear();
        self.possibleStates.append(selectedState);
    }
}
```

### 6.2 Region and Regime Implementation

```
struct Region {
    id: usize,
    name: String,
    regime: RegimeType,
    parent: Option<usize>, // Parent region ID
    allocations: ArrayList(Allocation),
    
    fn isCompatibleWith(self, other: Region, direction: PeriscopeDirection) -> bool {
        // Check compatibility matrix
        compatibilityMatrix[self.regime][other.regime][direction]
    }
}

enum RegimeType {
    FIFO,
    FILO,
    SEQ,
    RAND
}
```

### 6.3 Constraint Representation

```
struct Constraint {
    type: ConstraintType,
    cells: []CellId,
    validate: fn(states: []State) -> bool,
    
    fn apply(self, field: *EntropyField) {
        // Apply constraint logic
        let cellStates = field.getCellStates(self.cells);
        if (!self.validate(cellStates)) {
            field.removePossibilities(self.cells, cellStates);
        }
    }
}
```

### 6.4 WFCCC Algorithm Core

```
fn WFCCC(sourceCode: String) -> Result<ASTRepresentation, Error> {
    let entropyField = initializeEntropyField(sourceCode);
    applyGlobalConstraints(entropyField);
    
    while (!entropyField.fullyCollapsed()) {
        if (entropyField.hasContradiction()) {
            return Error(entropyField.getContradictionSource());
        }
        
        let cell = entropyField.findLowestEntropyCell();
        let state = cell.selectStateToCollapse();
        cell.collapse(state);
        propagateConstraints(entropyField, cell);
    }
    
    return entropyField.toASTRepresentation();
}
```

## 7. Example: Data Processing Pipeline

```
fn process_data(input: Array<Int>) -> Array<Int> {
    region InputData regime SEQ {
        let data = input;
        
        periscope data to Processing {
            // data lifetime extended to Processing region
        }
    }
    
    region Processing regime FIFO {
        // Process data in a pipeline fashion
        let stage1 = transform_stage1(data);
        let stage2 = transform_stage2(stage1);
        
        periscope stage2 to Output {
            // stage2 lifetime extended to Output region
        }
    }
    
    region Output regime RAND {
        // Use processed data for random access operations
        let result = finalize(stage2);
        return result;
    }
}
```

## 8. Compilation Process Flow

1. **Source Code Input**: Program text entered
2. **Entropy Field Construction**: Initialize all possibilities
3. **Main Function Identification**: Start constraint propagation from main
4. **Region Boundary Identification**: Detect region declarations
5. **Regime Compatibility Verification**: Check all periscope operations
6. **Constraint Propagation**: Apply constraints and collapse possibilities
7. **Code Generation**: Convert fully collapsed program to target code

## 9. Grammar (EBNF)

```
Program        ::= Declaration*

Declaration    ::= FunctionDecl | RegionDecl | TypeDecl | ImportDecl

FunctionDecl   ::= 'fn' Identifier '(' ParameterList? ')' '->' Type '{' Statement* '}'

RegionDecl     ::= 'region' Identifier RegimeOpt '{' Statement* '}'

RegimeOpt      ::= ε | 'regime' RegimeType

RegimeType     ::= 'FIFO' | 'FILO' | 'SEQ' | 'RAND'

PeriscopeStmt  ::= 'periscope' Expression 'to' Identifier '{' Statement* '}'

TypeDecl       ::= 'type' Identifier '=' Type

ParameterList  ::= Parameter (',' Parameter)*

Parameter      ::= Identifier ':' Type

Type           ::= BasicType | ArrayType | FunctionType | UserType

Statement      ::= ExpressionStmt | LetStmt | RegionDecl | PeriscopeStmt
                | ReturnStmt | IfStmt | MatchStmt

Expression     ::= Literal | Identifier | BinaryExpr | UnaryExpr
                | CallExpr | IndexExpr | IfExpr | MatchExpr
```

## 10. Future Directions

### 10.1 Advanced Regimes
Future versions may include more specialized regimes:
- CONC - Concurrent access with safety guarantees
- PART - Partitioned access for parallel processing
- PIPE - Pipeline-specific optimization for data flow

### 10.2 WFCCC Optimizations
- Constraint caching to avoid redundant checks
- Parallel constraint application for performance
- Heuristic selection to prioritize cells for collapse

### 10.3 Custom Regimes
A mechanism for defining custom regimes with specific safety properties:

```
define regime SORTED {
    invariant: forall i, j where i < j: elements[i] <= elements[j];
    compatible_with: [SEQ, FIFO];
}
```

### 10.4 IDE Integration
WFCCC provides rich information for IDE features:
- Real-time error detection and reporting
- Code completion based on possible valid states
- Refactoring suggestions based on constraint propagation
