#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

enum class TokenKind {
    Integer,
    Plus,
    Star,
    LeftParen,
    RightParen,
    End,
};

struct Token {
    TokenKind kind;
    std::string text;
    std::size_t position;
};

std::vector<Token> tokenize(std::string_view source);
std::string_view tokenKindName(TokenKind kind);
