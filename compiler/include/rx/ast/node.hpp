
#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "rx/ast/source_span.hpp"

namespace rx::ast {

    using NodeId = std::size_t;

    const NodeId InvalidNodeId = static_cast<NodeId>(-1);

    // 所有 AST 节点的具体种类
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

    // 所有 AST 节点的多态基类
    class AstNode {
    public:
        SourceSpan span{};

        NodeKind category;

        // 当前 AST 节点的唯一编号,用于ir的层次遍历
        // 在 AST 构建完成后统一分配
        mutable NodeId node_id = InvalidNodeId;

        virtual ~AstNode() = default;

        [[nodiscard]] virtual AstChildren children() const {
            return {};
        }

    protected:
        explicit AstNode(NodeKind category)
            : category(category) {}
    };

    // 为整棵 AST 分配唯一编号
    void assignNodeIds(const AstNode& root);

    // 以下辅助函数统一处理 AST 子节点

    inline void appendChild(AstChildren& result, const AstNode* child) {
        if (child != nullptr) {
            result.push_back(child);
        }
    }

    inline void appendChild(AstChildren& result, const AstNode& child) {
        result.push_back(&child);
    }

    template <class T>
    inline void appendChild(
        AstChildren& result,
        const std::shared_ptr<T>& child
    ) {
        appendChild(result, child.get());
    }

    template <class T>
    inline void appendChild(
        AstChildren& result,
        const std::optional<T>& child
    ) {
        if (child) {
            appendChild(result, *child);
        }
    }

    template <class T>
    inline void appendChildren(
        AstChildren& result,
        const std::vector<T>& children
    ) {
        for (const auto& child : children) {
            appendChild(result, child);
        }
    }

} // namespace rx::ast
