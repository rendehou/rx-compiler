#include "semantic.hpp"

#include <iostream>
#include <utility>

namespace {

AstNode integer(std::string text) {
    return AstNode{AstKind::Integer, std::move(text), {}};
}

AstNode boolean(std::string text) {
    return AstNode{AstKind::Bool, std::move(text), {}};
}

AstNode binary(std::string operation, AstNode left, AstNode right) {
    return AstNode{AstKind::Binary, std::move(operation),
                   {std::move(left), std::move(right)}};
}

}  // namespace

int main() {
    const AstNode valid = binary(
        "==", binary("+", integer("1"), integer("2")), integer("3"));
    const AstNode invalid = binary("+", boolean("true"), integer("1"));

    std::cout << "valid: " << typeName(checkType(valid)) << '\n';
    try {
        (void)checkType(invalid);
    } catch (const SemanticError& error) {
        std::cout << "invalid: " << error.what() << '\n';
    }
}
