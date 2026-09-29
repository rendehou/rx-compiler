#pragma once
#include <memory>
#include <rx/ast/expression.hpp>
#include <rx/ast/item.hpp>
#include <rx/ast/program.hpp>
#include <rx/ast/source_span.hpp>
#include <rx/ast/statement.hpp>
#include <rx/ast/type_syntax.hpp>
#include "rx/frontend/parser.hpp"
// Responsibility:
// - Declare conversion from real RxParser Context objects to ast::Program.
// - Provide private helpers for Item, TypeSyntax, Statement, Block, and Expr.
// - Perform structural validation and operator mapping only.
// Never perform name lookup, type checking, or retain Context pointers in AST.

namespace rx::frontend {
    class AstBuilder {
        public:
            std::unique_ptr<ast::Program> build(RxParser::CrateContext* context);
        private:
            // Item 与 use 树。
            std::unique_ptr<ast::Item> buildItem(RxParser::ItemContext* context);
            std::unique_ptr<ast::Item> buildUseDeclaration(RxParser::UseDeclarationContext* context);
            ast::UseTree buildUseTree(RxParser::UseTreeContext* context);
            std::unique_ptr<ast::FunctionItem> buildFunction(RxParser::FunctionDefinitionContext* context);
            std::unique_ptr<ast::StructItem> buildStructDefinition(RxParser::StructDefinitionContext* context);
            std::unique_ptr<ast::ConstantItem> buildConstantItem(RxParser::ConstantItemContext* context);
            std::unique_ptr<ast::ImplItem> buildInherentImpl(RxParser::InherentImplContext* context);

            // 泛型、where 子句与函数签名细节。
            ast::GenericParameters buildGenericParameters(RxParser::GenericParamsContext* context);
            ast::WhereClause buildWhereClause(RxParser::WhereClauseContext* context);
            ast::Receiver buildReceiver(RxParser::SelfParamContext* context);
            ast::FunctionParameter buildFunctionParameter(RxParser::FunctionParamContext* context);
            ast::DeriveAttribute buildOuterAttribute(RxParser::OuterAttributeContext* context);

            // 类型语法、常量语法与路径。
            ast::TypeSyntax buildTypeRef(RxParser::TypeRefContext* context);
            ast::TypeSyntax buildReferenceType(RxParser::ReferenceTypeContext* context);
            ast::TypeSyntax buildArrayType(RxParser::ArrayTypeContext* context);
            ast::TypeSyntax buildClosedCastType(RxParser::ClosedCastTypeContext* context);
            ast::Path buildTypePath(RxParser::TypePathContext* context);
            ast::PathSegment buildTypePathSegment(RxParser::TypePathSegmentContext* context);
            std::vector<ast::GenericArgumentSyntax> buildGenericArgs(RxParser::GenericArgsContext* context);
            ast::ConstValueSyntax buildConstValue(RxParser::ConstValueContext* context);
            ast::ConstValueSyntax buildMagnitude(RxParser::MagnitudeContext* context);
            ast::Path buildPathInExpression(RxParser::PathInExpressionContext* context);
            ast::PathSegment buildPathExprSegment(RxParser::PathExprSegmentContext* context);

            // 代码块与语句。
            ast::Block buildBlock(RxParser::BlockExpressionContext* context);
            std::unique_ptr<ast::Statement> buildStatement(RxParser::StatementContext* context);
            std::unique_ptr<ast::LetStatement> buildLetStatement(RxParser::LetStatementContext* context);

            // 普通表达式,expression下的细节
            std::unique_ptr<ast::Expr> buildExpression(RxParser::ExpressionContext* context);
            std::unique_ptr<ast::Expr> buildAssignmentExpression(RxParser::AssignmentExpressionContext* context);
            std::unique_ptr<ast::Expr> buildLogicalOrExpression(RxParser::LogicalOrExpressionContext* context);
            std::unique_ptr<ast::Expr> buildLogicalAndExpression(RxParser::LogicalAndExpressionContext* context);
            std::unique_ptr<ast::Expr> buildComparisonExpression(RxParser::ComparisonExpressionContext* context);
            std::unique_ptr<ast::Expr> buildBitOrExpression(RxParser::BitOrExpressionContext* context);
            std::unique_ptr<ast::Expr> buildClosedBitOrExpression(RxParser::ClosedBitOrExpressionContext* context);
            std::unique_ptr<ast::Expr> buildBitXorExpression(RxParser::BitXorExpressionContext* context);
            std::unique_ptr<ast::Expr> buildClosedBitXorExpression(RxParser::ClosedBitXorExpressionContext* context);
            std::unique_ptr<ast::Expr> buildBitAndExpression(RxParser::BitAndExpressionContext* context);
            std::unique_ptr<ast::Expr> buildClosedBitAndExpression(RxParser::ClosedBitAndExpressionContext* context);
            std::unique_ptr<ast::Expr> buildShiftExpression(RxParser::ShiftExpressionContext* context);
            std::unique_ptr<ast::Expr> buildClosedShiftExpression(RxParser::ClosedShiftExpressionContext* context);
            std::unique_ptr<ast::Expr> buildAdditiveExpression(RxParser::AdditiveExpressionContext* context);
            std::unique_ptr<ast::Expr> buildClosedAdditiveExpression(RxParser::ClosedAdditiveExpressionContext* context);
            std::unique_ptr<ast::Expr> buildMultiplicativeExpression(RxParser::MultiplicativeExpressionContext* context);
            std::unique_ptr<ast::Expr> buildClosedMultiplicativeExpression(RxParser::ClosedMultiplicativeExpressionContext* context);
            std::unique_ptr<ast::Expr> buildCastExpression(RxParser::CastExpressionContext* context);
            std::unique_ptr<ast::Expr> buildClosedCastExpression(RxParser::ClosedCastExpressionContext* context);
            std::unique_ptr<ast::Expr> buildUnaryExpression(RxParser::UnaryExpressionContext* context);
            std::unique_ptr<ast::Expr> buildPostfixExpression(RxParser::PostfixExpressionContext* context);
            std::unique_ptr<ast::Expr> buildPrimaryExpression(RxParser::PrimaryExpressionContext* context);
            std::unique_ptr<ast::Expr> buildNonBlockPrimary(RxParser::NonBlockPrimaryContext* context);
            std::unique_ptr<ast::Expr> buildExpressionWithBlock(RxParser::ExpressionWithBlockContext* context);
            std::unique_ptr<ast::Expr> buildIfExpression(RxParser::IfExpressionContext* context);

            // condition 前缀的表达式层级。
            std::unique_ptr<ast::Expr> buildConditionExpression(RxParser::ConditionExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionAssignmentExpression(RxParser::ConditionAssignmentExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionLogicalOrExpression(RxParser::ConditionLogicalOrExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionLogicalAndExpression(RxParser::ConditionLogicalAndExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionComparisonExpression(RxParser::ConditionComparisonExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBitOrExpression(RxParser::ConditionBitOrExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionClosedBitOrExpression(RxParser::ConditionClosedBitOrExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBitXorExpression(RxParser::ConditionBitXorExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionClosedBitXorExpression(RxParser::ConditionClosedBitXorExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBitAndExpression(RxParser::ConditionBitAndExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionClosedBitAndExpression(RxParser::ConditionClosedBitAndExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionShiftExpression(RxParser::ConditionShiftExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionClosedShiftExpression(RxParser::ConditionClosedShiftExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionAdditiveExpression(RxParser::ConditionAdditiveExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionClosedAdditiveExpression(RxParser::ConditionClosedAdditiveExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionMultiplicativeExpression(RxParser::ConditionMultiplicativeExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionClosedMultiplicativeExpression(RxParser::ConditionClosedMultiplicativeExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionCastExpression(RxParser::ConditionCastExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionClosedCastExpression(RxParser::ConditionClosedCastExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionUnaryExpression(RxParser::ConditionUnaryExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionPostfixExpression(RxParser::ConditionPostfixExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionPrimary(RxParser::ConditionPrimaryContext* context);
            std::unique_ptr<ast::Expr> buildConditionPrimaryWithoutBareBlock(RxParser::ConditionPrimaryWithoutBareBlockContext* context);

            // BREAK 操作数专用的 conditionBreak 层级。
            std::unique_ptr<ast::Expr> buildConditionBreakExpression(RxParser::ConditionBreakExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakAssignmentExpression(RxParser::ConditionBreakAssignmentExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakLogicalOrExpression(RxParser::ConditionBreakLogicalOrExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakLogicalAndExpression(RxParser::ConditionBreakLogicalAndExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakComparisonExpression(RxParser::ConditionBreakComparisonExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakBitOrExpression(RxParser::ConditionBreakBitOrExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakClosedBitOrExpression(RxParser::ConditionBreakClosedBitOrExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakBitXorExpression(RxParser::ConditionBreakBitXorExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakClosedBitXorExpression(RxParser::ConditionBreakClosedBitXorExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakBitAndExpression(RxParser::ConditionBreakBitAndExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakClosedBitAndExpression(RxParser::ConditionBreakClosedBitAndExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakShiftExpression(RxParser::ConditionBreakShiftExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakClosedShiftExpression(RxParser::ConditionBreakClosedShiftExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakAdditiveExpression(RxParser::ConditionBreakAdditiveExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakClosedAdditiveExpression(RxParser::ConditionBreakClosedAdditiveExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakMultiplicativeExpression(RxParser::ConditionBreakMultiplicativeExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakClosedMultiplicativeExpression(RxParser::ConditionBreakClosedMultiplicativeExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakCastExpression(RxParser::ConditionBreakCastExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakClosedCastExpression(RxParser::ConditionBreakClosedCastExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakUnaryExpression(RxParser::ConditionBreakUnaryExpressionContext* context);
            std::unique_ptr<ast::Expr> buildConditionBreakPostfixExpression(RxParser::ConditionBreakPostfixExpressionContext* context);

            // statement 前缀的表达式层级。
            std::unique_ptr<ast::Expr> buildStatementExpression(RxParser::StatementExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementAssignmentExpression(RxParser::StatementAssignmentExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementLogicalOrExpression(RxParser::StatementLogicalOrExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementLogicalAndExpression(RxParser::StatementLogicalAndExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementComparisonExpression(RxParser::StatementComparisonExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementBitOrExpression(RxParser::StatementBitOrExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementClosedBitOrExpression(RxParser::StatementClosedBitOrExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementBitXorExpression(RxParser::StatementBitXorExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementClosedBitXorExpression(RxParser::StatementClosedBitXorExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementBitAndExpression(RxParser::StatementBitAndExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementClosedBitAndExpression(RxParser::StatementClosedBitAndExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementShiftExpression(RxParser::StatementShiftExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementClosedShiftExpression(RxParser::StatementClosedShiftExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementAdditiveExpression(RxParser::StatementAdditiveExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementClosedAdditiveExpression(RxParser::StatementClosedAdditiveExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementMultiplicativeExpression(RxParser::StatementMultiplicativeExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementClosedMultiplicativeExpression(RxParser::StatementClosedMultiplicativeExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementCastExpression(RxParser::StatementCastExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementClosedCastExpression(RxParser::StatementClosedCastExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementUnaryExpression(RxParser::StatementUnaryExpressionContext* context);
            std::unique_ptr<ast::Expr> buildStatementPostfixExpression(RxParser::StatementPostfixExpressionContext* context);

            // 共享的原子表达式、后缀应用与运算符映射。
            std::unique_ptr<ast::Expr> buildLiteral(RxParser::LiteralExpressionContext* context);
            std::unique_ptr<ast::Expr> buildArrayExpression(RxParser::ArrayExpressionContext* context);
            std::unique_ptr<ast::Expr> applyPostfixSuffixes(std::unique_ptr<ast::Expr> base,
                                                            const std::vector<RxParser::PostfixSuffixContext*>& suffixes);
            std::unique_ptr<ast::Expr> applyDotSuffix(std::unique_ptr<ast::Expr> base,
                                                      RxParser::DotSuffixContext* suffix);
            ast::AssignmentOperator mapAssignmentOperator(RxParser::AssignmentOperatorContext* context) const;
            ast::UnaryOperator mapUnaryOperator(RxParser::UnaryOperatorContext* context) const;
            std::optional<ast::BinaryOperator> mapBinaryOperator(antlr4::tree::ParseTree* child) const;

            // 左结合的二元运算链折叠：按源码顺序逐个孩子处理，
            // 操作数交给 operandBuilder，运算符交给 mapBinaryOperator。
            template <typename OperandBuilder>
            std::unique_ptr<ast::Expr> buildBinaryChain(antlr4::tree::ParseTree* context,
                                                        OperandBuilder&& operandBuilder);
    };
}
