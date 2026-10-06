#pragma once

#include <string>
#include <string_view>
#include <vector>

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

enum class AstKind {
    Integer,
    Binary,
};

struct AstNode {
    AstKind kind;
    std::string value;
    std::vector<AstNode> children;
};

AstNode buildAst(const ParseNode& root);
void printAst(const AstNode& node, int depth = 0);
