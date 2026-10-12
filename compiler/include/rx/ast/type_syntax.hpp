#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rx/ast/node.hpp"

namespace rx::ast {

struct TypeSyntax;
struct GenericArgumentSyntax;

// 路径中的一个段，直接继承 AstNode；来自 typePathSegment/pathExprSegment 等路径规则。
// name 保存本段标识符，generic_arguments 保存紧随本段的泛型实参。
struct PathSegment final : AstNode {
    PathSegment();
    ~PathSegment() override;
    [[nodiscard]] AstChildren children() const override;
    std::string name;
    std::vector<std::shared_ptr<GenericArgumentSyntax>> generic_arguments;
};

// 完整路径节点，直接继承 AstNode；来自 typePath、pathInExpression 等规则。
struct Path final : AstNode {
    Path();
    [[nodiscard]] AstChildren children() const override;
    bool absolute = false;
    std::vector<std::shared_ptr<PathSegment>> segments;
};

// 常量语法的具体形式标签。
enum class ConstValueKind { Integer, Boolean, Path, Negate };

// 常量值语法节点。
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

// 类型语法的具体形式标签。
enum class TypeSyntaxKind { Unit, Path, Reference, Array };

// 类型语法节点。
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

// 泛型实参具体形式标签。
enum class GenericArgumentKind { Lifetime, Type };

// 泛型实参节点。
struct GenericArgumentSyntax final : AstNode {
    GenericArgumentSyntax();
    [[nodiscard]] AstChildren children() const override;
    GenericArgumentKind kind = GenericArgumentKind::Type;
    std::string lifetime;
    std::shared_ptr<TypeSyntax> type;
};

}  // namespace rx::ast
