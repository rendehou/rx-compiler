#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rx/ast/node.hpp"

namespace rx::ast {

// syntax主要保存语法节点，通过字符串先存储，先不进行解析，等到semantic再解析成对应内容

struct TypeSyntax;
struct GenericArgumentSyntax;

struct PathSegment final : AstNode {
    PathSegment();
    ~PathSegment() override;
    [[nodiscard]] AstChildren children() const override;
    std::string name;
    std::vector<std::shared_ptr<GenericArgumentSyntax>> generic_arguments;
};

struct Path final : AstNode {
    Path();
    [[nodiscard]] AstChildren children() const override;
    bool absolute = false;
    std::vector<std::shared_ptr<PathSegment>> segments;
};

enum class ConstValueKind { Integer, Boolean, Path, Negate };

struct ConstValueSyntax final : AstNode {
    ConstValueSyntax();
    ~ConstValueSyntax() override;
    [[nodiscard]] AstChildren children() const override;
    ConstValueKind kind = ConstValueKind::Integer;
    std::string integer_spelling;
    bool boolean_value = false;
    std::shared_ptr<Path> path;
    std::shared_ptr<ConstValueSyntax> operand;
};

enum class TypeSyntaxKind { Unit, Path, Reference, Array };

struct TypeSyntax final : AstNode {
    TypeSyntax();
    ~TypeSyntax() override;
    [[nodiscard]] AstChildren children() const override;
    TypeSyntaxKind kind = TypeSyntaxKind::Unit;
    std::shared_ptr<Path> path;
    std::string lifetime;
    bool mutable_reference = false;
    std::shared_ptr<TypeSyntax> referent;
    std::shared_ptr<TypeSyntax> array_element;
    std::shared_ptr<ConstValueSyntax> length;
};


enum class GenericArgumentKind { Lifetime, Type };

struct GenericArgumentSyntax final : AstNode {
    GenericArgumentSyntax();
    [[nodiscard]] AstChildren children() const override;
    GenericArgumentKind kind = GenericArgumentKind::Type;
    std::string lifetime;
    std::shared_ptr<TypeSyntax> type;
};

}  // namespace rx::ast
