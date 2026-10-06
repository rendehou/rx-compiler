#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

enum class AstKind { Integer, Bool, Binary };

struct AstNode {
    AstKind kind;
    std::string value;
    std::vector<AstNode> children;
};

enum class Type { I32, Bool };

class SemanticError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

std::string_view typeName(Type type);
Type checkType(const AstNode& node);
