#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rx/ast/operator.hpp"
#include "rx/ast/statement.hpp"
#include "rx/ast/type_syntax.hpp"

namespace rx::ast {

struct Expr {
    SourceSpan span{};
    virtual ~Expr() = default;
};

struct IntegerExpr final : Expr {
    std::string spelling;  // Preserve radix, underscores, and optional suffix.
};

struct BooleanExpr final : Expr {
    bool value = false;
};

struct UnitExpr final : Expr {};

struct PathExpr final : Expr {
    Path path;
};

struct BlockExpr final : Expr {
    Block block;
};

struct IfExpr final : Expr {
    std::unique_ptr<Expr> condition;
    Block then_block;
    std::unique_ptr<Expr> else_branch;  // BlockExpr or nested IfExpr; absent if no else.
};

struct WhileExpr final : Expr {
    std::unique_ptr<Expr> condition;
    Block body;
};

struct LoopExpr final : Expr {
    Block body;
};

struct BreakExpr final : Expr {
    std::unique_ptr<Expr> value;
};

struct ContinueExpr final : Expr {};

struct ReturnExpr final : Expr {
    std::unique_ptr<Expr> value;
};

struct UnaryExpr final : Expr {
    UnaryOperator op = UnaryOperator::Not;
    std::unique_ptr<Expr> operand;
};

struct BinaryExpr final : Expr {
    BinaryOperator op = BinaryOperator::Add;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
};

struct AssignmentExpr final : Expr {
    AssignmentOperator op = AssignmentOperator::Assign;
    std::unique_ptr<Expr> place;
    std::unique_ptr<Expr> value;
};

struct CastExpr final : Expr {
    std::unique_ptr<Expr> value;
    TypeSyntax target_type;
};

struct CallExpr final : Expr {
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> arguments;
};

struct MethodCallExpr final : Expr {
    std::unique_ptr<Expr> receiver;
    PathSegment method;
    std::vector<std::unique_ptr<Expr>> arguments;
};

struct FieldAccessExpr final : Expr {
    std::unique_ptr<Expr> base;
    std::string field;
};

struct IndexExpr final : Expr {
    std::unique_ptr<Expr> base;
    std::unique_ptr<Expr> index;
};

struct ArrayListExpr final : Expr {
    std::vector<std::unique_ptr<Expr>> elements;
};

struct ArrayRepeatExpr final : Expr {
    std::unique_ptr<Expr> element;
    ConstValueSyntax count;
};

struct StructInitializerField {
    SourceSpan span{};
    std::string name;
    std::unique_ptr<Expr> value;
};

struct StructInitExpr final : Expr {
    Path path;
    std::vector<StructInitializerField> fields;  // Preserve source order/duplicates.
};

// Parentheses and ANTLR's condition*/statement*/closed* grammar wrappers do
// not create AST classes. The builder maps them to the expression nodes above.

}  // namespace rx::ast
