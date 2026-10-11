#!/usr/bin/env python3
"""Create ANTLR-compatible temporary grammar copies with Rust-safe names."""

from pathlib import Path
import sys


def replace_once(text: str, old: str, new: str, path: Path) -> str:
    count = text.count(old)
    if count != 1:
        raise ValueError(f"expected one {old!r} in {path}, found {count}")
    return text.replace(old, new, 1)


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: prepare_grammar.py GRAMMAR_DIR OUTPUT_DIR", file=sys.stderr)
        return 2

    source_dir, output_dir = map(Path, sys.argv[1:])
    lexer_path = source_dir / "Lexer.g4"
    parser_path = source_dir / "Parser.g4"

    lexer = lexer_path.read_text(encoding="utf-8")
    parser = parser_path.read_text(encoding="utf-8")
    lexer = replace_once(lexer, "lexer grammar Lexer;", "lexer grammar RxLexer;", lexer_path)
    parser = replace_once(parser, "parser grammar Parser;", "parser grammar RxParser;", parser_path)
    parser = replace_once(parser, "tokenVocab=Lexer", "tokenVocab=RxLexer", parser_path)

    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / "RxLexer.g4").write_text(lexer, encoding="utf-8")
    (output_dir / "RxParser.g4").write_text(parser, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
