# C Compiler (Intermediate Code Generation)

A simplified C compiler built using **Flex (Lex)** and **Bison (Yacc)** that performs lexical analysis, syntax analysis, semantic analysis, symbol table management, Abstract Syntax Tree (AST) construction, and Three-Address Code (TAC) generation. 

This project was developed as a course project for **CSE420: Compiler Design** at BRAC University.

---

## 📌 Features

* **Lexical & Syntax Analysis:** Tokenizes input code and parses standard C control structures, declarations, expressions, and function definitions using Lex/Yacc.
* **Symbol Table & Scope Management:** Implements nested scope tables to handle variable scope, lookup, insertion, and type checking.
* **Abstract Syntax Tree (AST):** Constructs an AST representation of the parsed source program.
* **Intermediate Code Generation (ICG):** Translates C source code into Three-Address Code (TAC) for downstream compilation phases.

---

## 📁 Repository Structure

```text
.
├── 22101046.l           # Flex specification file (Lexical Analyzer)
├── 22101046.y           # Bison specification file (Parser & ICG)
├── symbol_info.h        # Class/struct definition for symbols
├── scope_table.h        # Implementation of scope management
├── symbol_table.h       # Full symbol table management implementation
├── ast.h                # AST node definitions and construction logic
├── three_addr_code.h    # Three-Address Code (TAC) generation utilities
├── input.c              # Sample C program used for testing
└── script.sh            # Shell script to compile and execute the project

### Code Comments Added to `22101046.y` and `22101046.l`

```
=====================================================================================


## 📌 PREREQUISITES
----------------
 - GCC / G++ (C/C++ Compiler)
 - Flex (Fast Lexical Analyzer Generator)
 - Bison (GNU Parser Generator)

## Installation (Debian/Ubuntu):
```text
sudo apt update && sudo apt install build-essential flex bison
```

## 🚀 HOW TO RUN
-------------
### Automated Build:
```text
chmod +x script.sh && ./script.sh
```

### Manual Build:
```text
bison -d -y 22101046.y
flex 22101046.l
g++ -w y.tab.c lex.yy.c -o compiler
./compiler input.c
```
 
## 📄 OUTPUT
---------
### Running the executable against input.c generates log files containing:
1. Identified tokens and lexemes
2. Parse tree / AST node structure
3. Symbol table state across scopes
4. Generated Three-Address Code (TAC)

=====================================================================================
