#include "rx/ast/expression.hpp"
#include "rx/ast/item.hpp"
#include "rx/ast/program.hpp"
#include "rx/ast/statement.hpp"
#include "rx/ast/type_syntax.hpp"

#include <utility>

namespace rx::ast {

LifetimeParameter::LifetimeParameter() : AstNode(NodeKind::LifetimeParameter) {}
AstChildren LifetimeParameter::children() const { return {}; }

GenericParameters::GenericParameters() : AstNode(NodeKind::GenericParameters) {}
AstChildren GenericParameters::children() const {
    AstChildren result;
    appendChildren(result, lifetimes);
    return result;
}

WherePredicate::WherePredicate() : AstNode(NodeKind::WherePredicate) {}
AstChildren WherePredicate::children() const {
    AstChildren result;
    appendChild(result, type_subject);
    return result;
}

WhereClause::WhereClause() : AstNode(NodeKind::WhereClause) {}
AstChildren WhereClause::children() const {
    AstChildren result;
    appendChildren(result, predicates);
    return result;
}

FunctionParameter::FunctionParameter() : AstNode(NodeKind::FunctionParameter) {}
AstChildren FunctionParameter::children() const {
    AstChildren result;
    appendChild(result, type);
    return result;
}

Receiver::Receiver() : AstNode(NodeKind::Receiver) {}
AstChildren Receiver::children() const { return {}; }

UseTree::UseTree() : AstNode(NodeKind::UseTree) {}
AstChildren UseTree::children() const {
    AstChildren result;
    appendChildren(result, group_items);
    return result;
}

UseItem::UseItem() : Item(NodeKind::UseItem) {}
AstChildren UseItem::children() const { return {&tree}; }

FunctionItem::FunctionItem() : Item(NodeKind::FunctionItem) {}
AstChildren FunctionItem::children() const {
    AstChildren result;
    appendChild(result, generic_parameters);
    appendChild(result, receiver);
    appendChildren(result, parameters);
    appendChild(result, return_type);
    appendChild(result, where_clause);
    appendChild(result, body);
    return result;
}

StructField::StructField() : AstNode(NodeKind::StructField) {}
AstChildren StructField::children() const { return {&type}; }

DeriveAttribute::DeriveAttribute() : AstNode(NodeKind::DeriveAttribute) {}
AstChildren DeriveAttribute::children() const { return {}; }

StructItem::StructItem() : Item(NodeKind::StructItem) {}
AstChildren StructItem::children() const {
    AstChildren result;
    appendChild(result, generic_parameters);
    appendChild(result, where_clause);
    appendChildren(result, attributes);
    appendChildren(result, fields);
    return result;
}

ConstantItem::ConstantItem() : Item(NodeKind::ConstantItem) {}
AstChildren ConstantItem::children() const { return {&type, &initializer}; }

AssociatedItem::AssociatedItem() : AstNode(NodeKind::AssociatedItem) {}
AstChildren AssociatedItem::children() const {
    AstChildren result;
    if (const auto* function = std::get_if<std::unique_ptr<FunctionItem>>(&value)) {
        appendChild(result, *function);
    } else if (const auto* constant = std::get_if<std::unique_ptr<ConstantItem>>(&value)) {
        appendChild(result, *constant);
    }
    return result;
}

ImplItem::ImplItem() : Item(NodeKind::ImplItem) {}
AstChildren ImplItem::children() const {
    AstChildren result;
    appendChild(result, generic_parameters);
    appendChild(result, target_type);
    appendChild(result, where_clause);
    appendChildren(result, items);
    return result;
}

EmptyStatement::EmptyStatement() : Statement(NodeKind::EmptyStatement) {}
AstChildren EmptyStatement::children() const { return {}; }

LetStatement::LetStatement() : Statement(NodeKind::LetStatement) {}
LetStatement::~LetStatement() = default;
AstChildren LetStatement::children() const {
    AstChildren result;
    appendChild(result, declared_type);
    appendChild(result, initializer);
    return result;
}

ExpressionStatement::ExpressionStatement() : Statement(NodeKind::ExpressionStatement) {}
ExpressionStatement::~ExpressionStatement() = default;
AstChildren ExpressionStatement::children() const {
    AstChildren result;
    appendChild(result, expression);
    return result;
}

Block::Block() : AstNode(NodeKind::Block) {}
Block::~Block() = default;
Block::Block(Block&&) noexcept = default;
Block& Block::operator=(Block&&) noexcept = default;
AstChildren Block::children() const {
    AstChildren result;
    appendChildren(result, statements);
    appendChild(result, tail);
    return result;
}

IntegerExpr::IntegerExpr() : Expr(NodeKind::IntegerExpr) {}
AstChildren IntegerExpr::children() const { return {}; }

BooleanExpr::BooleanExpr() : Expr(NodeKind::BooleanExpr) {}
AstChildren BooleanExpr::children() const { return {}; }

UnitExpr::UnitExpr() : Expr(NodeKind::UnitExpr) {}
AstChildren UnitExpr::children() const { return {}; }

PathExpr::PathExpr() : Expr(NodeKind::PathExpr) {}
AstChildren PathExpr::children() const { return {&path}; }

BlockExpr::BlockExpr() : Expr(NodeKind::BlockExpr) {}
AstChildren BlockExpr::children() const { return {&block}; }

IfExpr::IfExpr() : Expr(NodeKind::IfExpr) {}
AstChildren IfExpr::children() const {
    AstChildren result;
    appendChild(result, condition);
    appendChild(result, then_block);
    appendChild(result, else_branch);
    return result;
}

WhileExpr::WhileExpr() : Expr(NodeKind::WhileExpr) {}
AstChildren WhileExpr::children() const {
    AstChildren result;
    appendChild(result, condition);
    appendChild(result, body);
    return result;
}

LoopExpr::LoopExpr() : Expr(NodeKind::LoopExpr) {}
AstChildren LoopExpr::children() const { return {&body}; }

BreakExpr::BreakExpr() : Expr(NodeKind::BreakExpr) {}
AstChildren BreakExpr::children() const {
    AstChildren result;
    appendChild(result, value);
    return result;
}

ContinueExpr::ContinueExpr() : Expr(NodeKind::ContinueExpr) {}
AstChildren ContinueExpr::children() const { return {}; }

ReturnExpr::ReturnExpr() : Expr(NodeKind::ReturnExpr) {}
AstChildren ReturnExpr::children() const {
    AstChildren result;
    appendChild(result, value);
    return result;
}

UnaryExpr::UnaryExpr() : Expr(NodeKind::UnaryExpr) {}
AstChildren UnaryExpr::children() const {
    AstChildren result;
    appendChild(result, operand);
    return result;
}

BinaryExpr::BinaryExpr() : Expr(NodeKind::BinaryExpr) {}
AstChildren BinaryExpr::children() const {
    AstChildren result;
    appendChild(result, left);
    appendChild(result, right);
    return result;
}

AssignmentExpr::AssignmentExpr() : Expr(NodeKind::AssignmentExpr) {}
AstChildren AssignmentExpr::children() const {
    AstChildren result;
    appendChild(result, place);
    appendChild(result, value);
    return result;
}

CastExpr::CastExpr() : Expr(NodeKind::CastExpr) {}
AstChildren CastExpr::children() const {
    AstChildren result;
    appendChild(result, value);
    appendChild(result, target_type);
    return result;
}

CallExpr::CallExpr() : Expr(NodeKind::CallExpr) {}
AstChildren CallExpr::children() const {
    AstChildren result;
    appendChild(result, callee);
    appendChildren(result, arguments);
    return result;
}

MethodCallExpr::MethodCallExpr() : Expr(NodeKind::MethodCallExpr) {}
AstChildren MethodCallExpr::children() const {
    AstChildren result;
    appendChild(result, receiver);
    appendChild(result, method);
    appendChildren(result, arguments);
    return result;
}

FieldAccessExpr::FieldAccessExpr() : Expr(NodeKind::FieldAccessExpr) {}
AstChildren FieldAccessExpr::children() const {
    AstChildren result;
    appendChild(result, base);
    return result;
}

IndexExpr::IndexExpr() : Expr(NodeKind::IndexExpr) {}
AstChildren IndexExpr::children() const {
    AstChildren result;
    appendChild(result, base);
    appendChild(result, index);
    return result;
}

ArrayListExpr::ArrayListExpr() : Expr(NodeKind::ArrayListExpr) {}
AstChildren ArrayListExpr::children() const {
    AstChildren result;
    appendChildren(result, elements);
    return result;
}

ArrayRepeatExpr::ArrayRepeatExpr() : Expr(NodeKind::ArrayRepeatExpr) {}
AstChildren ArrayRepeatExpr::children() const {
    AstChildren result;
    appendChild(result, element);
    appendChild(result, count);
    return result;
}

StructInitializerField::StructInitializerField() : AstNode(NodeKind::StructInitializerField) {}
StructInitializerField::~StructInitializerField() = default;
StructInitializerField::StructInitializerField(StructInitializerField&&) noexcept = default;
StructInitializerField& StructInitializerField::operator=(StructInitializerField&&) noexcept = default;
AstChildren StructInitializerField::children() const {
    AstChildren result;
    appendChild(result, value);
    return result;
}

StructInitExpr::StructInitExpr() : Expr(NodeKind::StructInitExpr) {}
AstChildren StructInitExpr::children() const {
    AstChildren result;
    appendChild(result, path);
    appendChildren(result, fields);
    return result;
}

PathSegment::PathSegment() : AstNode(NodeKind::PathSegment) {}
PathSegment::~PathSegment() = default;
PathSegment::PathSegment(PathSegment&&) noexcept = default;
PathSegment& PathSegment::operator=(PathSegment&&) noexcept = default;
AstChildren PathSegment::children() const {
    AstChildren result;
    appendChildren(result, generic_arguments);
    return result;
}

Path::Path() : AstNode(NodeKind::Path) {}
AstChildren Path::children() const {
    AstChildren result;
    appendChildren(result, segments);
    return result;
}

ConstValueSyntax::ConstValueSyntax() : AstNode(NodeKind::ConstValueSyntax) {}
ConstValueSyntax::~ConstValueSyntax() = default;
ConstValueSyntax::ConstValueSyntax(ConstValueSyntax&&) noexcept = default;
ConstValueSyntax& ConstValueSyntax::operator=(ConstValueSyntax&&) noexcept = default;
AstChildren ConstValueSyntax::children() const {
    AstChildren result;
    if (kind == ConstValueKind::Path) appendChild(result, path);
    if (kind == ConstValueKind::Negate) appendChild(result, operand);
    return result;
}

TypeSyntax::TypeSyntax() : AstNode(NodeKind::TypeSyntax) {}
TypeSyntax::~TypeSyntax() = default;
TypeSyntax::TypeSyntax(TypeSyntax&&) noexcept = default;
TypeSyntax& TypeSyntax::operator=(TypeSyntax&&) noexcept = default;
AstChildren TypeSyntax::children() const {
    AstChildren result;
    switch (kind) {
    case TypeSyntaxKind::Unit:
        break;
    case TypeSyntaxKind::Path:
        appendChild(result, path);
        break;
    case TypeSyntaxKind::Reference:
        appendChild(result, referent);
        break;
    case TypeSyntaxKind::Array:
        appendChild(result, array_element);
        appendChild(result, length);
        break;
    }
    return result;
}

GenericArgumentSyntax::GenericArgumentSyntax() : AstNode(NodeKind::GenericArgumentSyntax) {}
AstChildren GenericArgumentSyntax::children() const {
    if (kind == GenericArgumentKind::Type) return {&type};
    return {};
}

AstChildren Program::children() const {
    AstChildren result;
    appendChildren(result, items);
    return result;
}

}  // namespace rx::ast
