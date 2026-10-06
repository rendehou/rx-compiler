#include "scope.hpp"

#include <cstddef>
#include <string>

ScopeChecker::ScopeChecker(std::ostream& output) : output_(output) {}

void ScopeChecker::check(const AstNode& root) {
    scopes_.clear();
    visit(root);
}

Type ScopeChecker::visit(const AstNode& node) {
    switch (node.kind) {
        case AstKind::Block: return visitBlock(node);
        case AstKind::Let: return visitLet(node);
        case AstKind::Name: return visitName(node);
        case AstKind::Integer: return Type::I32;
        case AstKind::Bool: return Type::Bool;
    }
    throw SemanticError("unknown AST node");
}

Type ScopeChecker::visitBlock(const AstNode& node) {
    const std::size_t depth = scopes_.size();
    scopes_.emplace_back();
    output_ << "enter scope " << depth << '\n';
    for (const AstNode& child : node.children) visit(child);
    output_ << "leave scope " << depth << '\n';
    scopes_.pop_back();
    return Type::Unit;
}

Type ScopeChecker::visitLet(const AstNode& node) {
    if (scopes_.empty() || node.children.size() != 1) {
        throw SemanticError("malformed let declaration");
    }
    const Type initializer = visit(node.children[0]);
    if (initializer != node.declared_type) {
        throw SemanticError(
            "initializer for " + node.text + " has type " +
            std::string(typeName(initializer)) + ", expected " +
            std::string(typeName(node.declared_type)));
    }

    auto& current = scopes_.back();
    const std::size_t depth = scopes_.size() - 1;
    if (current.find(node.text) != current.end()) {
        throw SemanticError("duplicate declaration of " + node.text +
                            " in scope " + std::to_string(depth));
    }
    current.emplace(node.text, node.declared_type);
    output_ << "declare " << node.text << ": " << typeName(node.declared_type)
            << " in scope " << depth << '\n';
    return Type::Unit;
}

Type ScopeChecker::visitName(const AstNode& node) {
    for (std::size_t count = scopes_.size(); count > 0; --count) {
        const std::size_t depth = count - 1;
        const auto found = scopes_[depth].find(node.text);
        if (found != scopes_[depth].end()) {
            output_ << "resolve " << node.text << " -> "
                    << typeName(found->second) << " from scope " << depth << '\n';
            return found->second;
        }
    }
    throw SemanticError("unknown name: " + node.text);
}

std::string_view typeName(Type type) {
    switch (type) {
        case Type::I32: return "i32";
        case Type::Bool: return "bool";
        case Type::Unit: return "unit";
    }
    throw SemanticError("unknown type");
}
