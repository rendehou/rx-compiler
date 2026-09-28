#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "rx/ast/source_span.hpp"

namespace rx::ast {

// 所有 AST 节点的“具体种类”标签。每个枚举值对应下面某个具体节点类；
// C++ 继承关系负责多态，NodeKind 则方便打印、遍历和调试时直接判断节点类别。
enum class NodeKind {
    Program,
    Item,
    UseItem,
    FunctionItem,
    StructItem,
    ConstantItem,
    ImplItem,
    UseTree,
    LifetimeParameter,
    GenericParameters,
    WherePredicate,
    WhereClause,
    FunctionParameter,
    Receiver,
    StructField,
    DeriveAttribute,
    AssociatedItem,
    Block,
    Statement,
    EmptyStatement,
    LetStatement,
    ExpressionStatement,
    Expr,
    IntegerExpr,
    BooleanExpr,
    UnitExpr,
    PathExpr,
    BlockExpr,
    IfExpr,
    WhileExpr,
    LoopExpr,
    BreakExpr,
    ContinueExpr,
    ReturnExpr,
    UnaryExpr,
    BinaryExpr,
    AssignmentExpr,
    CastExpr,
    CallExpr,
    MethodCallExpr,
    FieldAccessExpr,
    IndexExpr,
    ArrayListExpr,
    ArrayRepeatExpr,
    StructInitExpr,
    StructInitializerField,
    Path,
    PathSegment,
    ConstValueSyntax,
    TypeSyntax,
    GenericArgumentSyntax,
};

using AstChildren = std::vector<const class AstNode*>;

// 所有 AST 节点的多态基类。具体语法节点直接或间接继承它。
class AstNode {
public:
    SourceSpan span{};  // 对应源代码中的半开区间位置 [begin, end)。
    NodeKind category;  // 该对象的具体类别，例如 FunctionItem、BinaryExpr。

    virtual ~AstNode() = default;

    // 返回本节点的直接孩子，不拥有孩子；孩子的实际所有权仍由各派生类的
    // unique_ptr、vector 或值成员保存。叶节点返回空列表。
    [[nodiscard]] virtual AstChildren children() const { return {}; }

protected:
    explicit AstNode(NodeKind category) : category(category) {}
};

// 以下辅助函数把拥有型成员转换成 children() 所需的非拥有指针列表。
inline void appendChild(AstChildren& result, const AstNode* child) {
    if (child != nullptr) result.push_back(child);
}

inline void appendChild(AstChildren& result, const AstNode& child) {
    result.push_back(&child);
}

template <class T>
inline void appendChild(AstChildren& result, const std::unique_ptr<T>& child) {
    appendChild(result, child.get());
}

template <class T>
inline void appendChild(AstChildren& result, const std::optional<T>& child) {
    if (child) appendChild(result, *child);
}

template <class T>
inline void appendChildren(AstChildren& result, const std::vector<T>& children) {
    for (const auto& child : children) appendChild(result, child);
}

}  // namespace rx::ast
