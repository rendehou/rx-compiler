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
    PathSegment(PathSegment&&) noexcept;
    PathSegment& operator=(PathSegment&&) noexcept;
    PathSegment(const PathSegment&) = delete;
    PathSegment& operator=(const PathSegment&) = delete;
    [[nodiscard]] AstChildren children() const override;
    std::string name;                               // 本段名称，可为普通标识符、self 或 Self。
    std::vector<GenericArgumentSyntax> generic_arguments; // 本段显式携带的泛型实参，保持顺序。
};

// 完整路径节点，直接继承 AstNode；来自 typePath、pathInExpression 等规则。
// absolute 记录路径是否以 :: 开始，segments 保存逐段路径及每段泛型实参。
struct Path final : AstNode {
    Path();
    [[nodiscard]] AstChildren children() const override;
    bool absolute = false;                          // 是否为以 :: 开头的绝对路径。
    std::vector<PathSegment> segments;              // 路径段列表，例如 module::name。
};

// 常量语法的具体形式标签；ConstValueSyntax 用它区分整数、布尔、路径和负值。
enum class ConstValueKind { Integer, Boolean, Path, Negate };

// 常量值语法节点，直接继承 AstNode；对应受限的 constValue 规则，而非普通 Expr。
// kind 决定使用哪个值字段：整数原文、布尔值、路径，或负号操作数。
struct ConstValueSyntax final : AstNode {
    ConstValueSyntax();
    ~ConstValueSyntax() override;
    ConstValueSyntax(ConstValueSyntax&&) noexcept;
    ConstValueSyntax& operator=(ConstValueSyntax&&) noexcept;
    ConstValueSyntax(const ConstValueSyntax&) = delete;
    ConstValueSyntax& operator=(const ConstValueSyntax&) = delete;
    [[nodiscard]] AstChildren children() const override;
    ConstValueKind kind = ConstValueKind::Integer;  // 当前常量值的语法形态。
    std::string integer_spelling;                   // 整数字面量原文，仅 Integer 时使用。
    bool boolean_value = false;                    // true/false 的值，仅 Boolean 时使用。
    Path path;                                      // 常量路径，仅 Path 时使用。
    std::unique_ptr<ConstValueSyntax> operand;      // 一元负号后的常量，仅 Negate 时使用。
};

// 类型语法的具体形式标签；TypeSyntax 用它选择下面对应的类型字段。
enum class TypeSyntaxKind { Unit, Path, Reference, Array };

// 类型语法节点，直接继承 AstNode；对应 typeRef 及 referenceType/arrayType/typePath。
// 它保存源码写出的类型形式；类型解析结果另由 semantic 层保存。
struct TypeSyntax final : AstNode {
    TypeSyntax();
    ~TypeSyntax() override;
    TypeSyntax(TypeSyntax&&) noexcept;
    TypeSyntax& operator=(TypeSyntax&&) noexcept;
    TypeSyntax(const TypeSyntax&) = delete;
    TypeSyntax& operator=(const TypeSyntax&) = delete;
    [[nodiscard]] AstChildren children() const override;
    TypeSyntaxKind kind = TypeSyntaxKind::Unit;   // 单元、路径、引用或数组类型。
    Path path;                                    // kind 为 Path 时保存的类型路径。
    std::string lifetime;                         // 引用类型显式写出的生命周期；未写时为空。
    bool mutable_reference = false;               // 引用类型是否带 mut。
    std::unique_ptr<TypeSyntax> referent;          // 引用所指向的内部类型。
    std::unique_ptr<TypeSyntax> array_element;     // 数组元素类型。
    std::unique_ptr<ConstValueSyntax> length;      // 数组长度常量语法。
};

// 泛型实参具体形式标签；GenericArgumentSyntax 用它区分生命周期实参和类型实参。
enum class GenericArgumentKind { Lifetime, Type };

// 泛型实参节点，直接继承 AstNode；对应 genericArg 规则的 lifetime/type 两种选择。
// kind 决定有效字段：生命周期文本或一棵 TypeSyntax 子节点。
struct GenericArgumentSyntax final : AstNode {
    GenericArgumentSyntax();
    [[nodiscard]] AstChildren children() const override;
    GenericArgumentKind kind = GenericArgumentKind::Type; // 此实参是生命周期还是类型。
    std::string lifetime;                                // 生命周期实参的名称。
    TypeSyntax type;                                     // 类型实参的类型语法。
};

}  // namespace rx::ast
