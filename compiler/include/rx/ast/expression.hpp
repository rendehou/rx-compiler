#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rx/ast/operator.hpp"
#include "rx/ast/statement.hpp"
#include "rx/ast/type_syntax.hpp"

namespace rx::ast {

struct Expr : AstNode {
    virtual ~Expr() = default;
    [[nodiscard]] AstChildren children() const override = 0;
protected:
    explicit Expr(NodeKind kind) : AstNode(kind) {}
};

struct IntegerExpr final : Expr {
    IntegerExpr();
    [[nodiscard]] AstChildren children() const override;
    std::string spelling;
};

struct BooleanExpr final : Expr {
    BooleanExpr();
    [[nodiscard]] AstChildren children() const override;
    bool value = false;
};

struct UnitExpr final : Expr {
    UnitExpr();
    [[nodiscard]] AstChildren children() const override;
};

struct PathExpr final : Expr {
    PathExpr();
    [[nodiscard]] AstChildren children() const override;
    std::shared_ptr<Path> path;
};

struct BlockExpr final : Expr {
    BlockExpr();
    [[nodiscard]] AstChildren children() const override;
    std::shared_ptr<Block> block;
};

struct IfExpr final : Expr {
    IfExpr();
    [[nodiscard]] AstChildren children() const override;
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Block> then_block;
    std::shared_ptr<Expr> else_branch;
};

struct WhileExpr final : Expr {
    WhileExpr();
    [[nodiscard]] AstChildren children() const override;
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Block> body;
};

struct LoopExpr final : Expr {
    LoopExpr();
    [[nodiscard]] AstChildren children() const override;
    std::shared_ptr<Block> body;
};

struct BreakExpr final : Expr {
    BreakExpr();
    [[nodiscard]] AstChildren children() const override;
    std::shared_ptr<Expr> value;
};

struct ContinueExpr final : Expr {
    ContinueExpr();
    [[nodiscard]] AstChildren children() const override;
};

struct ReturnExpr final : Expr {
    ReturnExpr();
    [[nodiscard]] AstChildren children() const override;
    std::shared_ptr<Expr> value;
};

struct UnaryExpr final : Expr {
    UnaryExpr();
    [[nodiscard]] AstChildren children() const override;
    UnaryOperator op = UnaryOperator::Not;
    std::shared_ptr<Expr> operand;
};

struct BinaryExpr final : Expr {
    BinaryExpr();
    [[nodiscard]] AstChildren children() const override;
    BinaryOperator op = BinaryOperator::Add;
    std::shared_ptr<Expr> left;
    std::shared_ptr<Expr> right;
};

struct AssignmentExpr final : Expr {
    AssignmentExpr();
    [[nodiscard]] AstChildren children() const override;
    AssignmentOperator op = AssignmentOperator::Assign;
    std::shared_ptr<Expr> place;
    std::shared_ptr<Expr> value;
};

struct CastExpr final : Expr {
    CastExpr();
    [[nodiscard]] AstChildren children() const override;
    std::shared_ptr<Expr> value;
    std::shared_ptr<TypeSyntax> target_type;
};

struct CallExpr final : Expr {
    CallExpr();
    [[nodiscard]] AstChildren children() const override;
    std::shared_ptr<Expr> callee;
    std::vector<std::shared_ptr<Expr>> arguments;
};

struct MethodCallExpr final : Expr {
    MethodCallExpr();
    [[nodiscard]] AstChildren children() const override;
    std::shared_ptr<Expr> receiver;
    std::shared_ptr<PathSegment> method;
    std::vector<std::shared_ptr<Expr>> arguments;
};

struct FieldAccessExpr final : Expr {
    FieldAccessExpr();
    [[nodiscard]] AstChildren children() const override;
    std::shared_ptr<Expr> base;
    std::string field;
};

struct IndexExpr final : Expr {
    IndexExpr();
    [[nodiscard]] AstChildren children() const override;
    std::shared_ptr<Expr> base;
    std::shared_ptr<Expr> index;
};

struct ArrayListExpr final : Expr {
    ArrayListExpr();
    [[nodiscard]] AstChildren children() const override;
    std::vector<std::shared_ptr<Expr>> elements;
};

struct ArrayRepeatExpr final : Expr {
    ArrayRepeatExpr();
    [[nodiscard]] AstChildren children() const override;
    std::shared_ptr<Expr> element;
    std::shared_ptr<ConstValueSyntax> count;
};

struct StructInitializerField final : AstNode {
    StructInitializerField();
    ~StructInitializerField() override;
    [[nodiscard]] AstChildren children() const override;
    std::string name;
    std::shared_ptr<Expr> value;
};

struct StructInitExpr final : Expr {
    StructInitExpr();
    [[nodiscard]] AstChildren children() const override;
    std::shared_ptr<Path> path;
    std::vector<std::shared_ptr<StructInitializerField>> fields;
};

}  // namespace rx::ast
