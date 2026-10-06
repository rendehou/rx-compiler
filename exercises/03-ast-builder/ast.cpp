#include "ast.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>

namespace {

void requireKind(const ParseNode& node, ParseKind expected) {
    if (node.kind != expected) {
        throw std::runtime_error("unexpected ParseNode kind");
    }
}

AstNode buildExpression(const ParseNode& node);
AstNode buildTerm(const ParseNode& node);
AstNode buildPrimary(const ParseNode& node);

AstNode buildExpression(const ParseNode& node) {
    requireKind(node, ParseKind::Expression);
    if (node.children.empty() || node.children.size() % 2 == 0) {
        throw std::runtime_error("malformed Expression node");
    }

    AstNode result = buildTerm(node.children[0]);
    for (std::size_t index = 1; index < node.children.size(); index += 2) {
        const ParseNode& operation = node.children[index];
        requireKind(operation, ParseKind::PlusToken);
        AstNode right = buildTerm(node.children[index + 1]);
        result = AstNode{
            AstKind::Binary,
            operation.text,
            {std::move(result), std::move(right)},
        };
    }
    return result;
}

AstNode buildTerm(const ParseNode& node) {
    requireKind(node, ParseKind::Term);
    if (node.children.empty() || node.children.size() % 2 == 0) {
        throw std::runtime_error("malformed Term node");
    }

    AstNode result = buildPrimary(node.children[0]);
    for (std::size_t index = 1; index < node.children.size(); index += 2) {
        const ParseNode& operation = node.children[index];
        requireKind(operation, ParseKind::StarToken);
        AstNode right = buildPrimary(node.children[index + 1]);
        result = AstNode{
            AstKind::Binary,
            operation.text,
            {std::move(result), std::move(right)},
        };
    }
    return result;
}

AstNode buildPrimary(const ParseNode& node) {
    requireKind(node, ParseKind::Primary);

    if (node.children.size() == 1 &&
        node.children[0].kind == ParseKind::IntegerToken) {
        return AstNode{AstKind::Integer, node.children[0].text, {}};
    }

    if (node.children.size() == 3 &&
        node.children[0].kind == ParseKind::LeftParenToken &&
        node.children[1].kind == ParseKind::Expression &&
        node.children[2].kind == ParseKind::RightParenToken) {
        return buildExpression(node.children[1]);
    }

    throw std::runtime_error("malformed Primary node");
}

}  // namespace

AstNode buildAst(const ParseNode& root) {
    return buildExpression(root);
}

void printAst(const AstNode& node, int depth) {
    std::cout << std::string(static_cast<std::size_t>(depth * 2), ' ');
    if (node.kind == AstKind::Integer) {
        std::cout << "Integer";
    } else {
        std::cout << "Binary";
    }
    std::cout << " \"" << node.value << "\"\n";

    for (const AstNode& child : node.children) {
        printAst(child, depth + 1);
    }
}
