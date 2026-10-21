#pragma once

#include <any>
#include <memory>

#include "ParserBaseVisitor.h"
#include "rx/ast/program.hpp"

namespace rx::frontend {

class AstBuilder : public ParserBaseVisitor {
public:
    
    std::shared_ptr<ast::Program>
    build(Parser::CrateContext* ctx);

    std::any visitCrate(
        Parser::CrateContext* ctx
    ) override;


    // 第一部分Item，包括function，struct，const，impl，use
    //这些都先处理最外层的表达式，例如整个function或者整个struct表达式
    std::any visitFunctionDefinition(
        Parser::FunctionDefinitionContext* ctx
    ) override;

    std::any visitStructDefinition(
        Parser::StructDefinitionContext* ctx
    ) override;

    std::any visitConstantItem(
        Parser::ConstantItemContext* ctx
    ) override;

    std::any visitInherentImpl(
        Parser::InherentImplContext* ctx
    ) override;

    std::any visitUseDeclaration(
        Parser::UseDeclarationContext* ctx
    ) override;


    // 接着的部分，Function / Struct / Impl的内部结构，包括参数，生命周期，等
    std::any visitGenericParams(//泛形参数列表
        Parser::GenericParamsContext* ctx
    ) override;

    std::any visitLifetimeParam(//生命周期，不需要处理
        Parser::LifetimeParamContext* ctx
    ) override;

    std::any visitSelfParam(//函数自身，self，mutself等
        Parser::SelfParamContext* ctx
    ) override;

    std::any visitFunctionParam(//普通参数
        Parser::FunctionParamContext* ctx
    ) override;

    std::any visitWhereClause(//where语句
        Parser::WhereClauseContext* ctx
    ) override;

    std::any visitWhereClauseItem(//where语句里面的具体约束
        Parser::WhereClauseItemContext* ctx
    ) override;

    std::any visitStructField(//结构体的一个声明或者字段
        Parser::StructFieldContext* ctx
    ) override;

    std::any visitOuterAttribute(//外部声明，例如默认derive等
        Parser::OuterAttributeContext* ctx
    ) override;

    std::any visitAssociatedItem(//函数实现impl中的一个实现
        Parser::AssociatedItemContext* ctx
    ) override;


    //第三部分，type与path

    std::any visitTypeRef(//处理所有的类型，包括括号和其他杂乱格式
        Parser::TypeRefContext* ctx
    ) override;

    std::any visitReferenceType(//类型引用，是上一个的子函数
        Parser::ReferenceTypeContext* ctx
    ) override;

    std::any visitArrayType(//数组类型
        Parser::ArrayTypeContext* ctx
    ) override;

    std::any visitTypePath(//类型的完整路径
        Parser::TypePathContext* ctx
    ) override;

    std::any visitTypePathSegment(//是上一个的子函数，操作一个路径
        Parser::TypePathSegmentContext* ctx
    ) override;

    std::any visitPathInExpression(//处理表达式的完整路径
        Parser::PathInExpressionContext* ctx
    ) override;

    std::any visitPathExprSegment(//类似的，处理表达式路径的单片段
        Parser::PathExprSegmentContext* ctx
    ) override;

    std::any visitGenericArg(//处理单个泛型,泛型也就是对类设置变量
        Parser::GenericArgContext* ctx
    ) override;

    std::any visitClosedCastType(//特殊函数，在有两个<<的时候负责说明，消除歧义
        Parser::ClosedCastTypeContext* ctx
    ) override;

    std::any visitConstValue(//处理常数值
        Parser::ConstValueContext* ctx
    ) override;


    //block与statement的解析函数

    std::any visitBlockExpression(//块表达式
        Parser::BlockExpressionContext* ctx
    ) override;

    std::any visitStatement(//语句表达
        Parser::StatementContext* ctx
    ) override;

    std::any visitLetStatement(//let
        Parser::LetStatementContext* ctx
    ) override;


    //block和if的表达式

    std::any visitExpressionWithBlock(
        Parser::ExpressionWithBlockContext* ctx
    ) override;

    std::any visitIfExpression(
        Parser::IfExpressionContext* ctx
    ) override;


    //普通运算表达式，按顺序匹配

    std::any visitAssignmentExpression(
        Parser::AssignmentExpressionContext* ctx
    ) override;

    std::any visitLogicalOrExpression(
        Parser::LogicalOrExpressionContext* ctx
    ) override;

    std::any visitLogicalAndExpression(
        Parser::LogicalAndExpressionContext* ctx
    ) override;

    std::any visitComparisonExpression(
        Parser::ComparisonExpressionContext* ctx
    ) override;

    std::any visitBitOrExpression(
        Parser::BitOrExpressionContext* ctx
    ) override;

    std::any visitBitXorExpression(
        Parser::BitXorExpressionContext* ctx
    ) override;

    std::any visitBitAndExpression(
        Parser::BitAndExpressionContext* ctx
    ) override;

    std::any visitShiftExpression(
        Parser::ShiftExpressionContext* ctx
    ) override;

    std::any visitAdditiveExpression(
        Parser::AdditiveExpressionContext* ctx
    ) override;

    std::any visitMultiplicativeExpression(
        Parser::MultiplicativeExpressionContext* ctx
    ) override;

    std::any visitCastExpression(
        Parser::CastExpressionContext* ctx
    ) override;

    std::any visitUnaryExpression(
        Parser::UnaryExpressionContext* ctx
    ) override;

    std::any visitPostfixExpression(
        Parser::PostfixExpressionContext* ctx
    ) override;


    //没有块语句的表达式

    std::any visitNonBlockPrimary(//非代码块，例如关键字break等，
        Parser::NonBlockPrimaryContext* ctx
    ) override;

    std::any visitLiteralExpression(//字面量
        Parser::LiteralExpressionContext* ctx
    ) override;

    std::any visitArrayExpression(//数组表达式
        Parser::ArrayExpressionContext* ctx
    ) override;

    std::any visitStructExprField(//结构体初始化的字段
        Parser::StructExprFieldContext* ctx
    ) override;


    //use与别名,与最开始的部分是平行关系

    std::any visitUseTree(
        Parser::UseTreeContext* ctx
    ) override;

    
    
    //接下来是一些语法细节
    //这些函数用于消除< 与 <<的歧义
    std::any visitClosedBitOrExpression(
        Parser::ClosedBitOrExpressionContext* ctx
    ) override;

    std::any visitClosedBitXorExpression(
        Parser::ClosedBitXorExpressionContext* ctx
    ) override;

    std::any visitClosedBitAndExpression(
        Parser::ClosedBitAndExpressionContext* ctx
    ) override;

    std::any visitClosedShiftExpression(
        Parser::ClosedShiftExpressionContext* ctx
    ) override;

    std::any visitClosedAdditiveExpression(
        Parser::ClosedAdditiveExpressionContext* ctx
    ) override;

    std::any visitClosedMultiplicativeExpression(
        Parser::ClosedMultiplicativeExpressionContext* ctx
    ) override;

    std::any visitClosedCastExpression(
        Parser::ClosedCastExpressionContext* ctx
    ) override;


    // statement expression ,本质是给块首语句上标记，用于消除{}后没有分号的歧义。判定过程和普通expression层层进相同
    std::any visitStatementAssignmentExpression(
        Parser::StatementAssignmentExpressionContext* ctx
    ) override;

    std::any visitStatementLogicalOrExpression(
        Parser::StatementLogicalOrExpressionContext* ctx
    ) override;

    std::any visitStatementLogicalAndExpression(
        Parser::StatementLogicalAndExpressionContext* ctx
    ) override;

    std::any visitStatementComparisonExpression(
        Parser::StatementComparisonExpressionContext* ctx
    ) override;

    std::any visitStatementBitOrExpression(
        Parser::StatementBitOrExpressionContext* ctx
    ) override;

    std::any visitStatementClosedBitOrExpression(
        Parser::StatementClosedBitOrExpressionContext* ctx
    ) override;

    std::any visitStatementBitXorExpression(
        Parser::StatementBitXorExpressionContext* ctx
    ) override;

    std::any visitStatementClosedBitXorExpression(
        Parser::StatementClosedBitXorExpressionContext* ctx
    ) override;

    std::any visitStatementBitAndExpression(
        Parser::StatementBitAndExpressionContext* ctx
    ) override;

    std::any visitStatementClosedBitAndExpression(
        Parser::StatementClosedBitAndExpressionContext* ctx
    ) override;

    std::any visitStatementShiftExpression(
        Parser::StatementShiftExpressionContext* ctx
    ) override;

    std::any visitStatementClosedShiftExpression(
        Parser::StatementClosedShiftExpressionContext* ctx
    ) override;

    std::any visitStatementAdditiveExpression(
        Parser::StatementAdditiveExpressionContext* ctx
    ) override;

    std::any visitStatementClosedAdditiveExpression(
        Parser::StatementClosedAdditiveExpressionContext* ctx
    ) override;

    std::any visitStatementMultiplicativeExpression(
        Parser::StatementMultiplicativeExpressionContext* ctx
    ) override;

    std::any visitStatementClosedMultiplicativeExpression(
        Parser::StatementClosedMultiplicativeExpressionContext* ctx
    ) override;

    std::any visitStatementCastExpression(
        Parser::StatementCastExpressionContext* ctx
    ) override;

    std::any visitStatementClosedCastExpression(
        Parser::StatementClosedCastExpressionContext* ctx
    ) override;

    std::any visitStatementUnaryExpression(
        Parser::StatementUnaryExpressionContext* ctx
    ) override;

    std::any visitStatementPostfixExpression(
        Parser::StatementPostfixExpressionContext* ctx
    ) override;
    
    //condition，消除歧义，
    std::any visitConditionAssignmentExpression(
        Parser::ConditionAssignmentExpressionContext* ctx
    ) override;

    std::any visitConditionLogicalOrExpression(
        Parser::ConditionLogicalOrExpressionContext* ctx
    ) override;

    std::any visitConditionLogicalAndExpression(
        Parser::ConditionLogicalAndExpressionContext* ctx
    ) override;

    std::any visitConditionComparisonExpression(
        Parser::ConditionComparisonExpressionContext* ctx
    ) override;

    std::any visitConditionBitOrExpression(
        Parser::ConditionBitOrExpressionContext* ctx
    ) override;

    std::any visitConditionClosedBitOrExpression(
        Parser::ConditionClosedBitOrExpressionContext* ctx
    ) override;

    std::any visitConditionBitXorExpression(
        Parser::ConditionBitXorExpressionContext* ctx
    ) override;

    std::any visitConditionClosedBitXorExpression(
        Parser::ConditionClosedBitXorExpressionContext* ctx
    ) override;

    std::any visitConditionBitAndExpression(
        Parser::ConditionBitAndExpressionContext* ctx
    ) override;

    std::any visitConditionClosedBitAndExpression(
        Parser::ConditionClosedBitAndExpressionContext* ctx
    ) override;

    std::any visitConditionShiftExpression(
        Parser::ConditionShiftExpressionContext* ctx
    ) override;

    std::any visitConditionClosedShiftExpression(
        Parser::ConditionClosedShiftExpressionContext* ctx
    ) override;

    std::any visitConditionAdditiveExpression(
        Parser::ConditionAdditiveExpressionContext* ctx
    ) override;

    std::any visitConditionClosedAdditiveExpression(
        Parser::ConditionClosedAdditiveExpressionContext* ctx
    ) override;

    std::any visitConditionMultiplicativeExpression(
        Parser::ConditionMultiplicativeExpressionContext* ctx
    ) override;

    std::any visitConditionClosedMultiplicativeExpression(
        Parser::ConditionClosedMultiplicativeExpressionContext* ctx
    ) override;

    std::any visitConditionCastExpression(
        Parser::ConditionCastExpressionContext* ctx
    ) override;

    std::any visitConditionClosedCastExpression(
        Parser::ConditionClosedCastExpressionContext* ctx
    ) override;

    std::any visitConditionUnaryExpression(
        Parser::ConditionUnaryExpressionContext* ctx
    ) override;

    std::any visitConditionPostfixExpression(
        Parser::ConditionPostfixExpressionContext* ctx
    ) override;

    std::any visitConditionPrimary(
        Parser::ConditionPrimaryContext* ctx
    ) override;

    std::any visitConditionPrimaryWithoutBareBlock(
        Parser::ConditionPrimaryWithoutBareBlockContext* ctx
    ) override;
private:
    //这是类型转换函数，使用的时候是asNode（实际想要的类）
    template <typename T>
    std::shared_ptr<T> asNode(const std::any& value) {
        return std::dynamic_pointer_cast<T>(
            std::any_cast<std::shared_ptr<ast::AstNode>>(value)
        );
    }
};

} // namespace rx::frontend