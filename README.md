# C-to-C Compiler

A small C compiler front end written in C. This project currently provides a compile process and lexer/tokenizer for C-style source code, with the main goal of reading an input file, producing tokens, and preparing the compiler pipeline.

## Project structure

- `main.c` — program entry point that compiles `test.c` and reports success or failure.
- `compiler.c` — compiler orchestration, sets up compilation and lexical analysis.
- `cprocess.c` — file I/O callbacks used by the lexer for `next`, `peek`, and `push` operations.
- `lex_process.c` — lexer state management and token vector handling.
- `lexer.c` — lexical analysis implementation for numbers, strings, identifiers, operators, comments, and symbols.
- `token.c` — helper functions for token operations.
- `compiler.h` — shared compiler and lexer definitions, token types, and public APIs.
- `helpers/` — support utilities such as `buffer` and `vector`.
- `.gitignore` — ignores build artifacts and generated files.

## Build

Use the existing `Makefile` to compile the project:

```bash
make
```

This creates the `main` executable.

## Run

Run the compiler with:

```bash
./main
```

Currently `main` compiles the hard-coded file `test.c` into an output file named `test`.

## Notes

- The lexer supports numeric literals, string literals, character literals, identifiers, keywords, operators, and comments.
- The current pipeline ends after lexical analysis and token assembly; code generation is not yet implemented.
- `build/` is ignored by Git via `.gitignore`.

## Next steps

Possible improvements:

- Add parsing and AST construction.
- Add code generation or output translation.
- Support more C language constructs and preprocessing.
- Add command-line arguments for input/output file names.
