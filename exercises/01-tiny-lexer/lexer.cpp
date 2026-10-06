#include "lexer.hpp"

#include <cctype>
#include <stdexcept>

std::vector<Token> tokenize(std::string_view source) {
    std::vector<Token> tokens;
    std::size_t position = 0;

    while (position < source.size()) {
        const char current = source[position];

        if (std::isspace(static_cast<unsigned char>(current))) {
            ++position;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(current))) {
            const std::size_t start = position;
            while (position < source.size() &&
                   std::isdigit(static_cast<unsigned char>(source[position]))) {
                ++position;
            }
            tokens.push_back(Token{
                TokenKind::Integer,
                std::string(source.substr(start, position - start)),
                start,
            });
            continue;
        }

        TokenKind kind;
        switch (current) {
            case '+':
                kind = TokenKind::Plus;
                break;
            case '*':
                kind = TokenKind::Star;
                break;
            case '(':
                kind = TokenKind::LeftParen;
                break;
            case ')':
                kind = TokenKind::RightParen;
                break;
            default:
                throw std::runtime_error(
                    "unexpected character at position " +
                    std::to_string(position));
        }

        tokens.push_back(Token{kind, std::string(1, current), position});
        ++position;
    }

    tokens.push_back(Token{TokenKind::End, "", source.size()});
    return tokens;
}

std::string_view tokenKindName(TokenKind kind) {
    switch (kind) {
        case TokenKind::Integer:
            return "INTEGER";
        case TokenKind::Plus:
            return "PLUS";
        case TokenKind::Star:
            return "STAR";
        case TokenKind::LeftParen:
            return "LPAREN";
        case TokenKind::RightParen:
            return "RPAREN";
        case TokenKind::End:
            return "END";
    }
    throw std::runtime_error("unknown TokenKind");
}
