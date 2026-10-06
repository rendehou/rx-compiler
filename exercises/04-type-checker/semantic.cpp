#include "semantic.hpp"

#include <string>

namespace {

[[noreturn]] void operandError(const std::string& operation,
                               std::string_view expected,
                               Type left,
                               Type right) {
    throw SemanticError(
        "operator \"" + operation + "\" requires " +
        std::string(expected) + ", got (" + std::string(typeName(left)) +
        ", " + std::string(typeName(right)) + ")");
}

}  // namespace

std::string_view typeName(Type type) {
    switch (type) {
        case Type::I32: return "i32";
        case Type::Bool: return "bool";
    }
    throw SemanticError("unknown type");
}

Type checkType(const AstNode& node) {
    if (node.kind == AstKind::Integer) return Type::I32;
    if (node.kind == AstKind::Bool) return Type::Bool;
    if (node.children.size() != 2) {
        throw SemanticError("binary node must have exactly two children");
    }

    const Type left = checkType(node.children[0]);
    const Type right = checkType(node.children[1]);

    if (node.value == "+" || node.value == "*") {
        if (left != Type::I32 || right != Type::I32) {
            operandError(node.value, "(i32, i32)", left, right);
        }
        return Type::I32;
    }
    if (node.value == "&&") {
        if (left != Type::Bool || right != Type::Bool) {
            operandError(node.value, "(bool, bool)", left, right);
        }
        return Type::Bool;
    }
    if (node.value == "==") {
        if (left != right) {
            operandError(node.value, "two equal types", left, right);
        }
        return Type::Bool;
    }
    throw SemanticError("unsupported binary operator: " + node.value);
}
