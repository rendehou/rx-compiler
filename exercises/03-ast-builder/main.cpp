#include "ast.hpp"

#include <utility>
#include <vector>

namespace {

ParseNode leaf(ParseKind kind, std::string text) {
    return ParseNode{kind, std::move(text), {}};
}

ParseNode branch(ParseKind kind, std::vector<ParseNode> children) {
    return ParseNode{kind, "", std::move(children)};
}

ParseNode integerPrimary(std::string text) {
    return branch(ParseKind::Primary, {
        leaf(ParseKind::IntegerToken, std::move(text)),
    });
}

ParseNode inputParseTree() {
    ParseNode parenthesizedSum = branch(ParseKind::Expression, {
        branch(ParseKind::Term, {integerPrimary("4")}),
        leaf(ParseKind::PlusToken, "+"),
        branch(ParseKind::Term, {integerPrimary("5")}),
    });

    ParseNode groupedPrimary = branch(ParseKind::Primary, {
        leaf(ParseKind::LeftParenToken, "("),
        std::move(parenthesizedSum),
        leaf(ParseKind::RightParenToken, ")"),
    });

    return branch(ParseKind::Expression, {
        branch(ParseKind::Term, {integerPrimary("12")}),
        leaf(ParseKind::PlusToken, "+"),
        branch(ParseKind::Term, {
            integerPrimary("3"),
            leaf(ParseKind::StarToken, "*"),
            std::move(groupedPrimary),
        }),
    });
}

}  // namespace

int main() {
    const ParseNode parseTree = inputParseTree();
    const AstNode ast = buildAst(parseTree);
    printAst(ast);
}
