# Rx ANTLR CST viewer (C++)

This standalone C++ program uses the repository's `grammar/RxLexer.g4` and
`grammar/RxParser.g4` to parse an Rx source file and print ANTLR's parenthesized
concrete syntax tree (CST). The original grammar files are used directly and
are not modified. This tool only displays the CST; it does not build an AST.

## Requirements

- Java 11 or newer
- C++17 compiler and CMake 3.18 or newer
- `curl` and `sha256sum` (only needed to download the pinned ANTLR tool)
- Network access on the first CMake configure, to fetch the matching C++ runtime

ANTLR's Java generator and C++ runtime are separate components. `make generate`
downloads ANTLR 4.13.2 and writes C++ lexer, parser, listener, and visitor files
to `src/generated/`. `make cst` also configures and builds the C++ runtime from
the matching ANTLR 4.13.2 source tag, compiles the viewer, and prints the tree.

## Run

From this directory:

```sh
make cst
make cst INPUT=path/to/example.rx
```

The parser entry rule is `crate`. Rule names appear as branches in the output;
matched tokens, including punctuation, appear as leaves. Whitespace and
comments are assigned to the lexer's hidden channel, so they are not shown in
the printed tree.

Run `make generate` after changing either source grammar. Run `make clean` to
remove generated C++ files and the CMake build directory. Downloads are kept in
`.tools/` and build artifacts are kept in `build/`; both directories are ignored
by Git.
