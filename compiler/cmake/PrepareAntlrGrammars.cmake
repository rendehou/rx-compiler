if(NOT DEFINED SOURCE_DIR OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "SOURCE_DIR and OUTPUT_DIR are required")
endif()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")

# The course grammars are intentionally named Lexer and Parser.  Those names
# collide with antlr4::Lexer and antlr4::Parser in C++, so generation uses
# renamed build copies while the official grammar files remain untouched.
file(READ "${SOURCE_DIR}/Lexer.g4" lexer_grammar)
string(REPLACE "lexer grammar Lexer;" "lexer grammar RxLexer;" lexer_grammar "${lexer_grammar}")
file(WRITE "${OUTPUT_DIR}/RxLexer.g4" "${lexer_grammar}")

file(READ "${SOURCE_DIR}/Parser.g4" parser_grammar)
string(REPLACE "parser grammar Parser;" "parser grammar RxParser;" parser_grammar "${parser_grammar}")
string(REPLACE "tokenVocab=Lexer;" "tokenVocab=RxLexer;" parser_grammar "${parser_grammar}")
file(WRITE "${OUTPUT_DIR}/RxParser.g4" "${parser_grammar}")

