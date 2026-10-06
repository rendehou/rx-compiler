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
};

enum class ParseKind {
    Expression,
    Term,
    Primary,
    IntegerToken,
    PlusToken,
    StarToken,
    LeftParenToken,
    RightParenToken,
};

struct ParseNode {
    ParseKind kind;
    std::string text;
    std::vector<ParseNode> children;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    ParseNode parse();

private:
    ParseNode parseExpression();
    ParseNode parseTerm();
    ParseNode parsePrimary();

    const Token& peek() const;
    Token consume(TokenKind expected);
    static ParseNode tokenNode(const Token& token);

    std::vector<Token> tokens_;
    std::size_t position_ = 0;
};

std::string_view parseKindName(ParseKind kind);
void printParseTree(const ParseNode& node, int depth = 0);
