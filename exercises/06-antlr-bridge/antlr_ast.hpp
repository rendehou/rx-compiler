#pragma once

#include <string>
#include <vector>

#include "RxParser.h"

enum class AstKind {
    Integer,
    Bool,
    Binary,
};

struct AstNode {
    AstKind kind;
    std::string value;
    std::vector<AstNode> children;
};

AstNode buildAst(rx::RxParser::ExpressionContext* context);
void printAst(const AstNode& node, int depth = 0);
