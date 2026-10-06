#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

enum class Type { I32, Bool, Unit };
enum class AstKind { Block, Let, Name, Integer, Bool };

struct AstNode {
    AstKind kind;
    std::string text;
    Type declared_type = Type::Unit;
    std::vector<AstNode> children;
};

class SemanticError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ScopeChecker {
public:
    explicit ScopeChecker(std::ostream& output);
    void check(const AstNode& root);

private:
    Type visit(const AstNode& node);
    Type visitBlock(const AstNode& node);
    Type visitLet(const AstNode& node);
    Type visitName(const AstNode& node);

    std::ostream& output_;
    std::vector<std::unordered_map<std::string, Type>> scopes_;
};

std::string_view typeName(Type type);
