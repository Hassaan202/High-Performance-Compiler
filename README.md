# SSC Compiler

A high-performance compiler that translates Simple Script Code (SSC) to LLVM IR for optimized execution.

## Overview

The SSC Compiler is designed to compile a simple scripting language (SSC) into LLVM IR (Intermediate Representation), which can then be optimized and compiled into machine code for efficient execution. This project demonstrates fundamental compiler construction techniques including lexical analysis, parsing, and code generation using LLVM infrastructure.

## Language Features

SSC (Simple Script Code) includes the following features:

- Variable declarations and assignments
- Arithmetic operations (+, -, *, /)
- Conditional execution (if-else statements)
- Loops (for loops)
- Functions (definition and calling)
- I/O operations (prints for strings, printd for doubles)

## Project Structure

```
.
├── IR.h              # LLVM IR code generation utilities
├── input.ssc         # Sample input file
├── Makefile          # Build system
├── README.md         # This file
├── ssc.l             # Flex lexical analyzer specification
└── ssc.y             # Bison parser specification
```

## Requirements

- LLVM (development version)
- Clang++ compiler
- Flex (lexical analyzer generator)
- Bison (parser generator)
- Make

## Build Instructions

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/SSC-Compiler.git
   cd SSC-Compiler
   ```

2. Build the compiler:
   ```bash
   make
   ```

3. Generate LLVM IR from an SSC file:
   ```bash
   make ir
   ```

4. Optimize the generated IR:
   ```bash
   make opt
   ```

5. Compile and run the generated code:
   ```bash
   make run
   ```

6. Run with optimizations:
   ```bash
   make run_opt
   ```

## Usage Examples

### Basic Compilation

To compile an SSC file:
```bash
./ssc_compiler input.ssc > output.ll
```

### Run with Optimizations

```bash
make run_opt
```

### Debug Mode

To run the compiler with debug output:
```bash
make debug
```

## Example SSC Program

```
function foo() { 
    printd(200);
    a = 2 + 3;
    printd(a);
}

if (2 > 3){
    prints("Hello");
} 
else{
    prints("Hello else");
}

for (i = 1; 5){
    printd(i);
}
```

## Language Syntax

### Variables and Expressions
```
variableName = expression;
```

### Output
```
printd(numericExpression);  // Print a double value
prints("string literal");   // Print a string
```

### Conditionals
```
if (condition) {
    statements;
}

if (condition) {
    statements;
} else {
    statements;
}
```

### Loops
```
for (counter = start; end) {
    statements;
}
```

### Functions
```
function functionName() {
    statements;
    return expression;
}
```

## Implementation Details

### Lexical Analysis

The lexical analyzer (`ssc.l`) uses Flex to recognize tokens such as:
- Keywords: `printd`, `prints`, `if`, `else`, `for`, `function`, `return`
- Identifiers
- Number literals
- String literals
- Operators and delimiters

### Syntax Analysis

The parser (`ssc.y`) uses Bison to define the grammar of the SSC language and handle:
- Expression evaluation
- Control flow statements
- Function definitions and calls
- Error recovery

### Code Generation

The code generator (`IR.h`) uses LLVM to:
- Create LLVM IR instructions for each language construct
- Handle variable scoping and symbol tables
- Implement control flow (if/else, loops)
- Generate code for function definitions and calls
- Manage I/O operations

## Optimization

The compiler supports various optimization levels through LLVM's optimization passes. Use the `OPT_LEVEL` variable in the Makefile to set the desired optimization level.

## Limitations and Future Work

- Currently, functions do not support parameters (only return values)
- Limited error handling and recovery
- No support for arrays or complex data types
- No type system beyond double-precision numbers and strings
