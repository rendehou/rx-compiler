#include "parser.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

ParseNode Parser::parse() {
    ParseNode root = parseExpression();
    consume(TokenKind::End);
    return root;
}

ParseNode Parser::parseExpression() {
    ParseNode expression{ParseKind::Expression, "", {}};
    expression.children.push_back(parseTerm());

    while (peek().kind == TokenKind::Plus) {
        expression.children.push_back(tokenNode(consume(TokenKind::Plus)));
        expression.children.push_back(parseTerm());
    }

    return expression;
}

ParseNode Parser::parseTerm() {
    ParseNode term{ParseKind::Term, "", {}};
    term.children.push_back(parsePrimary());

    while (peek().kind == TokenKind::Star) {
        term.children.push_back(tokenNode(consume(TokenKind::Star)));
        term.children.push_back(parsePrimary());
    }

    return term;
}

ParseNode Parser::parsePrimary() {
    ParseNode primary{ParseKind::Primary, "", {}};

    if (peek().kind == TokenKind::Integer) {
        primary.children.push_back(tokenNode(consume(TokenKind::Integer)));
        return primary;
    }

    if (peek().kind == TokenKind::LeftParen) {
        primary.children.push_back(tokenNode(consume(TokenKind::LeftParen)));
        primary.children.push_back(parseExpression());
        primary.children.push_back(tokenNode(consume(TokenKind::RightParen)));
        return primary;
    }

    throw std::runtime_error("expected INTEGER or LPAREN, got: " + peek().text);
}

const Token& Parser::peek() const {
    if (position_ >= tokens_.size()) {
        throw std::runtime_error("unexpected end of token vector");
    }
    return tokens_[position_];
}

Token Parser::consume(TokenKind expected) {
    const Token token = peek();
    if (token.kind != expected) {
        throw std::runtime_error("unexpected token: " + token.text);
    }
    ++position_;
    return token;
}

ParseNode Parser::tokenNode(const Token& token) {
    switch (token.kind) {
        case TokenKind::Integer:
            return ParseNode{ParseKind::IntegerToken, token.text, {}};
        case TokenKind::Plus:
            return ParseNode{ParseKind::PlusToken, token.text, {}};
        case TokenKind::Star:
            return ParseNode{ParseKind::StarToken, token.text, {}};
        case TokenKind::LeftParen:
            return ParseNode{ParseKind::LeftParenToken, token.text, {}};
        case TokenKind::RightParen:
            return ParseNode{ParseKind::RightParenToken, token.text, {}};
        case TokenKind::End:
            break;
    }
    throw std::runtime_error("END does not become a ParseNode");
}

std::string_view parseKindName(ParseKind kind) {
    switch (kind) {
        case ParseKind::Expression:
            return "Expression";
        case ParseKind::Term:
            return "Term";
        case ParseKind::Primary:
            return "Primary";
        case ParseKind::IntegerToken:
            return "INTEGER";
        case ParseKind::PlusToken:
            return "PLUS";
        case ParseKind::StarToken:
            return "STAR";
        case ParseKind::LeftParenToken:
            return "LPAREN";
        case ParseKind::RightParenToken:
            return "RPAREN";
    }
    throw std::runtime_error("unknown ParseKind");
}

void printParseTree(const ParseNode& node, int depth) {
    std::cout << std::string(static_cast<std::size_t>(depth * 2), ' ')
              << parseKindName(node.kind);
    if (!node.text.empty()) {
        std::cout << " \"" << node.text << '"';
    }
    std::cout << '\n';

    for (const ParseNode& child : node.children) {
        printParseTree(child, depth + 1);
    }
}
