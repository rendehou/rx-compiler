# Rx ANTLR CST viewer

This small Rust program generates a lexer and parser from the repository's
`grammar/Lexer.g4` and `grammar/Parser.g4`, parses an Rx source file, and prints
ANTLR's parenthesized concrete syntax tree (CST). It does not build an AST.

## Requirements

- Java 11 or newer
- Rust and Cargo
- `curl` and `sha256sum` (only needed to download the pinned generator)

The Rust target is provided by the `antlr4rust` generator and runtime, both
pinned to the 0.6 line. The generator jar is downloaded into `.tools/` and is
not committed.

## Run

From this directory:

```sh
make cst
make cst INPUT=path/to/example.rx
```

The first run downloads the pinned ANTLR generator if needed, generates Rust
sources under `src/generated/`, then compiles and runs the viewer. The parser
entry rule is `crate`. The output is ANTLR's parenthesized tree: rule names
appear as branches, while matched tokens (including punctuation) appear as
leaves.

The Makefile copies the two source grammars into `.tools/grammar/` and renames
their declarations to `RxLexer` and `RxParser`. This avoids Rust name clashes;
the repository's original grammar files are left untouched. The parser uses
the token vocabulary generated from the lexer, so the lexer runs first. Both
listener and visitor code are generated.

Generated Rust sources, token files, and ANTLR metadata are ignored by Git.
After changing a source grammar, run `make generate`; `src/generated/mod.rs`
lists the generated Rust modules.

Whitespace and comments are assigned to ANTLR's hidden channel by the lexer.
They remain available in the token stream, but are not children in the printed
parse tree.
