# mimiru_db
Academic Database Management System written in modern C++ (C++17).

## Overview
The system implements a two-level hierarchical data storage (Databases $\rightarrow$ Tables $\rightarrow$ Records) supporting integer and string data types. It features rigid schema typing, integrity constraints (`NOT NULL`, `INDEXED`), and a custom SQL-like query language.

The architecture strictly follows SOLID principles and modern C++ best practices, featuring a custom **Disk Pager**, a persistent **B+ Tree**, and a fully decoupled **Lexer-Parser-Executor** pipeline for query processing.

## Project Structure

The codebase is strictly separated into interface (`include/`) and implementation (`src/`) directories, organized by architectural domains:

```text
mimiru_db/
├── CMakeLists.txt              # Modern Target-based CMake configuration
├── README.md                   # This documentation file
├── include/mimiru/             # Public API & Headers
│   ├── core/                   # Overlays, Data Types, Schema, Exceptions
│   ├── storage/                # Physical storage layer (Pager, Disk I/O)
│   ├── index/                  # Indexing mechanisms (B+ Tree)
│   ├── engine/                 # Table manipulation, System & DB Manager
│   └── query/                  # Lexer, Parser (AST definitions)
├── src/                        # Implementations
│   ├── storage/pager.cpp
│   ├── index/bplus_tree.cpp
│   ├── engine/
│   ├── query/
│   └── main.cpp                # Entry point (Interactive & Batch loops)
└── tests/                      # Google Test Suite (GTest)
    ├── CMakeLists.txt
    ├── test_storage.cpp
    ├── test_bplus_tree.cpp
    ├── test_engine.cpp
    └── test_parser.cpp
```

## Build and Run Instructions

The project uses **CMake** (v3.14+) and strictly enforces **C++17**.
*Note: The build system automatically fetches `GoogleTest` and `nlohmann/json` from GitHub via `FetchContent`. An internet connection is required for the first configuration.*

### 1. Build the project
Open your terminal in the root directory and run:

```bash
# Generate build files
cmake -B build

# Compile the project and tests
cmake --build build
```

### 2. Run the application
The application supports two modes according to the technical requirements:

**Interactive Mode** (REPL reading from terminal):
```bash
./build/prog
```

**Batch Mode** (Executes sequential queries from a file):
```bash
./build/prog path/to/script.txt
```

### 3. Run the Test Suite
The storage engine, B+ Tree logic, and parsers are heavily covered by unit tests. To run them:
```bash
./build/tests/mimiru_tests
```
