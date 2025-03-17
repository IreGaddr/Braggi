# Braggi 🌌

> *"Poetry in motion; if there's an error in your syntax, then no there's no collapse"* 🤠

Braggi ain't your grandpappy's programming language - it's a revolutionary approach to systems programming that'll make you question everything you thought you knew about compilers and memory management. Saddle up, partner!

## 🌟 The Quantum Compiler Revolution

Traditional compilers are as predictable as Texas weather - lexer scans, parser parses, AST builds. **BORING!** Braggi throws that playbook out the window and rewrites the rules:

```
       Traditional Compilers        |        Braggi's Approach
-----------------------------------|----------------------------------
 Lexer → Parser → AST → Code Gen   |   Entropy Field → Constraint Application → 
                                   |   Wave Function Collapse → Certainty
```

## 🔥 What Makes Braggi Special?

### 🧠 Wave Function Collapse Compilation
We've taken a procedural generation algorithm used in game development and weaponized it for compilation! Your code exists in a quantum superposition of possible interpretations until our constraints collapse it into working code. 

```braggi
// This code is in superposition until constraints collapse it
func calculate(value) {
    // Types determined by constraint propagation!
    result = value * 2
    return result
}
```

### 🏞️ Region-Based Memory Management With Attitude
Forget garbage collection OR manual management - we've lassoed a third way:

```braggi
region GameObjects(10MB) regime FIFO {
    // All memory here follows First-In-First-Out access
    // and gets auto-cleaned when the region ends
    
    var player = Entity::new();
    var enemies = List::new();
    
    // Memory automatically freed when region ends
    // No more leaks, no more dangling pointers!
}
```

### 🔍 Periscope Memory System
Need to safely share memory between regions? Our periscope system lets you extend object lifetimes with compile-time safety guarantees!

```braggi
region Temporary regime FILO {
    var config = load_config();
    
    periscope config to GameLogic {
        // Config safely accessible in GameLogic region!
    }
}
```

### 🧩 Entity Component System Architecture
Under the hood, Braggi uses an ECS to work its magic - the same tech that powers modern game engines. This ain't just academic theory; it's battle-tested technology reborn for language design!

## 🚀 Performance That'll Knock Your Boots Off

Braggi doesn't just talk the talk - it walks the walk faster than a jackrabbit on a hot griddle:

- **SIMD-Friendly Design**: Constraint operations scream with vectorization *work in progress*
- **Cache-Optimized Memory Layout**: Regions create natural memory locality *work in progress*  
- **Parallelizable Compilation**: WFC algorithm scales across cores like a dream *work in progress*
- **Self-Optimizing Potential**: Braggi can use its own regime system to optimize itself! *work in progress*

## 🔧 Building & Running

### Prerequisites

- CMake 3.10+
- C compiler supporting C11 (GCC/Clang)
- libreadline-dev (optional, for a REPL that'll make you smile)

### Quick Start

```bash
# Git'r done with our simple build script
./run_build.sh

# Or do it yourself, Texas style
mkdir -p build && cd build
cmake ..
cmake --build .

# Fire up the REPL and start collapsin' some wavefunctions!
./bin/braggi-repl
```

## 📊 Project Status

Braggi v0.0.1 is our pioneer release - we've got the foundations built but there's gold in them thar hills yet to mine!

- ✅ Region-based memory management
- ✅ Wave Function Collapse engine
- ✅ ECS-based constraint system
- ✅ Error handling that actually makes sense
- 🔄 Standard library (in progress)
- 🔄 Self-hosting compiler (comin' soon!)

## 🤠 Join the Quantum Compiler Posse!

We're roundin' up developers with a taste for innovation and a dash of courage. If you've ever looked at a compiler and thought "there's gotta be a better way," well partner, you've found your new home!

## 📜 License

This project is open source and available under the [MIT License](LICENSE).

---

*"In Texas, we measure code quality in horsepower. In Ireland, we measure it in smiles. Braggi delivers both!" - The Braggi Team* 🍀🤠 