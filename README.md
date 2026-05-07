# mimiru_db
Academic database management system made with C++ and will

## Overview
The system implements a two-level hierarchical data storage (Databases $\rightarrow$ Tables $\rightarrow$ Records) supporting integer and string data types. It features rigid schema typing, integrity constraints (`NOT NULL`, `INDEXED`), and a custom SQL-like query language.

The architecture strictly follows SOLID principles, separating physical storage, indexing mechanisms, and query execution.

## Project Structure

The codebase is organized by architectural layers (domains):

```text
dbms-bplus-tree/
├── CMakeLists.txt          # Modern CMake build configuration
├── README.md               # This documentation file
└── src/                    # Application source code
    ├── core/               # Base abstractions: Data Types, Schema, Constraints
    ├── storage/            # Physical storage layer (I/O, Page Management)
    ├── index/              # Indexing mechanisms (B+ Tree implementation)
    ├── engine/             # Core DB logic: Table management, validation
    ├── query/              # Query Processor: SQL-like Parser and Executor
    └── main.cpp            # Entry point (Interactive & Batch mode handlers)
```

## Build and Run Instructions

The project uses **CMake** as its build system. It requires a compiler that supports **C++17** (e.g., GCC, Clang, or MSVC).

### 1. Build the project
Open your terminal in the root directory of the project and run the following commands:

```bash
# Generate build files in the 'build' directory
cmake -B build

# Compile the project
cmake --build build
```

### 2. Run the application
The application supports two modes according to the requirements:

**Interactive Mode** (reads queries from terminal):
```bash
# Linux / macOS
./build/prog

# Windows (PowerShell)
.\build\Debug\prog.exe
```

**Batch Mode** (executes queries from a text file):
```bash
./build/prog path/to/script.txt
```

## TODO List / Implementation Roadmap

### Phase 1: MVP & Core Data Structures (WIP)
-[x] Define base data types using `std::variant` and `std::optional`.
- [x] Implement Table Schema and column definitions.
- [x] Add DML validation logic (Type checking, `NOT NULL` constraints).
-[x] Implement In-Memory Storage mock for early testing.
- [ ] Implement `FileStorageManager` for binary disk I/O (Persistence).

### Phase 2: Indexing (Variant 2)
- [ ] Design B+ Tree node structures (saving to disk).
- [ ] Implement B+ Tree `insert` (Node splitting).
- [ ] Implement B+ Tree `remove` (Node merging/borrowing).
- [ ] Implement B+ Tree `find` and `findRange`.
- [ ] Integrate B+ Tree into the `Table` class (Indexes must only store `RecordId`).

### Phase 3: Query Processing (SQL-like)
- [ ] Implement Lexer/Tokenizer for the custom SQL syntax.
- [ ] Implement Parser to generate Abstract Syntax Trees (AST).
- [ ] Implement query semantic validation.
- [ ] Implement Query Executor.
- [ ] Format `SELECT` query outputs as JSON arrays.

### Phase 4: Application Interface
-[x] CLI skeleton.
- [ ] Interactive REPL loop with proper error handling.
- [ ] Batch script parsing and execution.

### Phase 5: Extra Requirements (Course Tasks)
- [ ] **[10] Default Values:** Support `DEFAULT[value]` in `CREATE TABLE`.
- [ ] **[11] Complex WHERE:** Support boolean logic (`AND`, `OR`) and parentheses `(...)`.
- [ ] **[12] Aggregations:** Support `SUM`, `COUNT`, `AVG` in `SELECT` queries.
- [ ] **[0] Temporal Persistence:** Support `REVERT` to rollback database state (No full snapshots allowed).
- [ ] **[0] String Interning:** Optimize memory by deduplicating string values.
