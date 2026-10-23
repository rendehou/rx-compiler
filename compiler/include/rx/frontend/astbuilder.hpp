#pragma once

#include <any>
#include <memory>

#include "RxParserBaseVisitor.h"
#include "rx/ast/program.hpp"

namespace rx::frontend {

class AstBuilder : public RxParserBaseVisitor {
public:
    
    std::shared_ptr<ast::Program>
    build(RxParser::CrateContext* ctx);

    std::any visitCrate(
        RxParser::CrateContext* ctx
    ) override;


    // 第一部分Item，包括function，struct，const，impl，use
    //这些都先处理最外层的表达式，例如整个function或者整个struct表达式
    std::any visitFunctionDefinition(
        RxParser::FunctionDefinitionContext* ctx
    ) override;

    std::any visitStructDefinition(
        RxParser::StructDefinitionContext* ctx
    ) override;

    std::any visitConstantItem(
        RxParser::ConstantItemContext* ctx
    ) override;

    std::any visitInherentImpl(
        RxParser::InherentImplContext* ctx
    ) override;

    std::any visitUseDeclaration(
        RxParser::UseDeclarationContext* ctx
    ) override;


    // 接着的部分，Function / Struct / Impl的内部结构，包括参数，生命周期，等
    std::any visitGenericParams(//泛形参数列表
        RxParser::GenericParamsContext* ctx
    ) override;

    std::any visitLifetimeParam(//生命周期，不需要处理
        RxParser::LifetimeParamContext* ctx
    ) override;

    std::any visitSelfParam(//函数自身，self，mutself等
        RxParser::SelfParamContext* ctx
    ) override;

    std::any visitFunctionParam(//普通参数
        RxParser::FunctionParamContext* ctx
    ) override;

    std::any visitWhereClause(//where语句
        RxParser::WhereClauseContext* ctx
    ) override;

    std::any visitWhereClauseItem(//where语句里面的具体约束
        RxParser::WhereClauseItemContext* ctx
    ) override;

    std::any visitStructField(//结构体的一个声明或者字段
        RxParser::StructFieldContext* ctx
    ) override;

    std::any visitOuterAttribute(//外部声明，例如默认derive等
        RxParser::OuterAttributeContext* ctx
    ) override;

    std::any visitAssociatedItem(//函数实现impl中的一个实现
        RxParser::AssociatedItemContext* ctx
    ) override;


    //第三部分，type与path

    std::any visitTypeRef(//处理所有的类型，包括括号和其他杂乱格式
        RxParser::TypeRefContext* ctx
    ) override;

    std::any visitReferenceType(//类型引用，是上一个的子函数
        RxParser::ReferenceTypeContext* ctx
    ) override;

    std::any visitArrayType(//数组类型
        RxParser::ArrayTypeContext* ctx
    ) override;

    std::any visitTypePath(//类型的完整路径
        RxParser::TypePathContext* ctx
    ) override;

    std::any visitTypePathSegment(//是上一个的子函数，操作一个路径
        RxParser::TypePathSegmentContext* ctx
    ) override;

    std::any visitPathInExpression(//处理表达式的完整路径
        RxParser::PathInExpressionContext* ctx
    ) override;

    std::any visitPathExprSegment(//类似的，处理表达式路径的单片段
        RxParser::PathExprSegmentContext* ctx
    ) override;

    std::any visitGenericArg(//处理单个泛型,泛型也就是对类设置变量
        RxParser::GenericArgContext* ctx
    ) override;

    std::any visitClosedCastType(//特殊函数，在有两个<<的时候负责说明，消除歧义
        RxParser::ClosedCastTypeContext* ctx
    ) override;

    std::any visitConstValue(//处理常数值
        RxParser::ConstValueContext* ctx
    ) override;


    //block与statement的解析函数

    std::any visitBlockExpression(//块表达式
        RxParser::BlockExpressionContext* ctx
    ) override;

    std::any visitStatement(//语句表达
        RxParser::StatementContext* ctx
    ) override;

    std::any visitLetStatement(//let
        RxParser::LetStatementContext* ctx
    ) override;


    //block和if的表达式

    std::any visitExpressionWithBlock(
        RxParser::ExpressionWithBlockContext* ctx
    ) override;

    std::any visitIfExpression(
        RxParser::IfExpressionContext* ctx
    ) override;


    //普通运算表达式，按顺序匹配

    std::any visitAssignmentExpression(
        RxParser::AssignmentExpressionContext* ctx
    ) override;

    std::any visitLogicalOrExpression(
        RxParser::LogicalOrExpressionContext* ctx
    ) override;

    std::any visitLogicalAndExpression(
        RxParser::LogicalAndExpressionContext* ctx
    ) override;

    std::any visitComparisonExpression(
        RxParser::ComparisonExpressionContext* ctx
    ) override;

    std::any visitBitOrExpression(
        RxParser::BitOrExpressionContext* ctx
    ) override;

    std::any visitBitXorExpression(
        RxParser::BitXorExpressionContext* ctx
    ) override;

    std::any visitBitAndExpression(
        RxParser::BitAndExpressionContext* ctx
    ) override;

    std::any visitShiftExpression(
        RxParser::ShiftExpressionContext* ctx
    ) override;

    std::any visitAdditiveExpression(
        RxParser::AdditiveExpressionContext* ctx
    ) override;

    std::any visitMultiplicativeExpression(
        RxParser::MultiplicativeExpressionContext* ctx
    ) override;

    std::any visitCastExpression(
        RxParser::CastExpressionContext* ctx
    ) override;

    std::any visitUnaryExpression(
        RxParser::UnaryExpressionContext* ctx
    ) override;

    std::any visitPostfixExpression(
        RxParser::PostfixExpressionContext* ctx
    ) override;


    //没有块语句的表达式

    std::any visitNonBlockPrimary(//非代码块，例如关键字break等，
        RxParser::NonBlockPrimaryContext* ctx
    ) override;

    std::any visitLiteralExpression(//字面量
        RxParser::LiteralExpressionContext* ctx
    ) override;

    std::any visitArrayExpression(//数组表达式
        RxParser::ArrayExpressionContext* ctx
    ) override;

    std::any visitStructExprField(//结构体初始化的字段
        RxParser::StructExprFieldContext* ctx
    ) override;


    //use与别名,与最开始的部分是平行关系

    std::any visitUseTree(
        RxParser::UseTreeContext* ctx
    ) override;

    
    
    //接下来是一些语法细节
    //这些函数用于消除< 与 <<的歧义
    std::any visitClosedBitOrExpression(
        RxParser::ClosedBitOrExpressionContext* ctx
    ) override;

    std::any visitClosedBitXorExpression(
        RxParser::ClosedBitXorExpressionContext* ctx
    ) override;

    std::any visitClosedBitAndExpression(
        RxParser::ClosedBitAndExpressionContext* ctx
    ) override;

    std::any visitClosedShiftExpression(
        RxParser::ClosedShiftExpressionContext* ctx
    ) override;

    std::any visitClosedAdditiveExpression(
        RxParser::ClosedAdditiveExpressionContext* ctx
    ) override;

    std::any visitClosedMultiplicativeExpression(
        RxParser::ClosedMultiplicativeExpressionContext* ctx
    ) override;

    std::any visitClosedCastExpression(
        RxParser::ClosedCastExpressionContext* ctx
    ) override;


    // statement expression ,本质是给块首语句上标记，用于消除{}后没有分号的歧义。判定过程和普通expression层层进相同
    std::any visitStatementAssignmentExpression(
        RxParser::StatementAssignmentExpressionContext* ctx
    ) override;

    std::any visitStatementLogicalOrExpression(
        RxParser::StatementLogicalOrExpressionContext* ctx
    ) override;

    std::any visitStatementLogicalAndExpression(
        RxParser::StatementLogicalAndExpressionContext* ctx
    ) override;

    std::any visitStatementComparisonExpression(
        RxParser::StatementComparisonExpressionContext* ctx
    ) override;

    std::any visitStatementBitOrExpression(
        RxParser::StatementBitOrExpressionContext* ctx
    ) override;

    std::any visitStatementClosedBitOrExpression(
        RxParser::StatementClosedBitOrExpressionContext* ctx
    ) override;

    std::any visitStatementBitXorExpression(
        RxParser::StatementBitXorExpressionContext* ctx
    ) override;

    std::any visitStatementClosedBitXorExpression(
        RxParser::StatementClosedBitXorExpressionContext* ctx
    ) override;

    std::any visitStatementBitAndExpression(
        RxParser::StatementBitAndExpressionContext* ctx
    ) override;

    std::any visitStatementClosedBitAndExpression(
        RxParser::StatementClosedBitAndExpressionContext* ctx
    ) override;

    std::any visitStatementShiftExpression(
        RxParser::StatementShiftExpressionContext* ctx
    ) override;

    std::any visitStatementClosedShiftExpression(
        RxParser::StatementClosedShiftExpressionContext* ctx
    ) override;

    std::any visitStatementAdditiveExpression(
        RxParser::StatementAdditiveExpressionContext* ctx
    ) override;

    std::any visitStatementClosedAdditiveExpression(
        RxParser::StatementClosedAdditiveExpressionContext* ctx
    ) override;

    std::any visitStatementMultiplicativeExpression(
        RxParser::StatementMultiplicativeExpressionContext* ctx
    ) override;

    std::any visitStatementClosedMultiplicativeExpression(
        RxParser::StatementClosedMultiplicativeExpressionContext* ctx
    ) override;

    std::any visitStatementCastExpression(
        RxParser::StatementCastExpressionContext* ctx
    ) override;

    std::any visitStatementClosedCastExpression(
        RxParser::StatementClosedCastExpressionContext* ctx
    ) override;

    std::any visitStatementUnaryExpression(
        RxParser::StatementUnaryExpressionContext* ctx
    ) override;

    std::any visitStatementPostfixExpression(
        RxParser::StatementPostfixExpressionContext* ctx
    ) override;
    
    //condition，消除歧义，
    std::any visitConditionAssignmentExpression(
        RxParser::ConditionAssignmentExpressionContext* ctx
    ) override;

    std::any visitConditionLogicalOrExpression(
        RxParser::ConditionLogicalOrExpressionContext* ctx
    ) override;

    std::any visitConditionLogicalAndExpression(
        RxParser::ConditionLogicalAndExpressionContext* ctx
    ) override;

    std::any visitConditionComparisonExpression(
        RxParser::ConditionComparisonExpressionContext* ctx
    ) override;

    std::any visitConditionBitOrExpression(
        RxParser::ConditionBitOrExpressionContext* ctx
    ) override;

    std::any visitConditionClosedBitOrExpression(
        RxParser::ConditionClosedBitOrExpressionContext* ctx
    ) override;

    std::any visitConditionBitXorExpression(
        RxParser::ConditionBitXorExpressionContext* ctx
    ) override;

    std::any visitConditionClosedBitXorExpression(
        RxParser::ConditionClosedBitXorExpressionContext* ctx
    ) override;

    std::any visitConditionBitAndExpression(
        RxParser::ConditionBitAndExpressionContext* ctx
    ) override;

    std::any visitConditionClosedBitAndExpression(
        RxParser::ConditionClosedBitAndExpressionContext* ctx
    ) override;

    std::any visitConditionShiftExpression(
        RxParser::ConditionShiftExpressionContext* ctx
    ) override;

    std::any visitConditionClosedShiftExpression(
        RxParser::ConditionClosedShiftExpressionContext* ctx
    ) override;

    std::any visitConditionAdditiveExpression(
        RxParser::ConditionAdditiveExpressionContext* ctx
    ) override;

    std::any visitConditionClosedAdditiveExpression(
        RxParser::ConditionClosedAdditiveExpressionContext* ctx
    ) override;

    std::any visitConditionMultiplicativeExpression(
        RxParser::ConditionMultiplicativeExpressionContext* ctx
    ) override;

    std::any visitConditionClosedMultiplicativeExpression(
        RxParser::ConditionClosedMultiplicativeExpressionContext* ctx
    ) override;

    std::any visitConditionCastExpression(
        RxParser::ConditionCastExpressionContext* ctx
    ) override;

    std::any visitConditionClosedCastExpression(
        RxParser::ConditionClosedCastExpressionContext* ctx
    ) override;

    std::any visitConditionUnaryExpression(
        RxParser::ConditionUnaryExpressionContext* ctx
    ) override;

    std::any visitConditionPostfixExpression(
        RxParser::ConditionPostfixExpressionContext* ctx
    ) override;

    std::any visitConditionPrimary(
        RxParser::ConditionPrimaryContext* ctx
    ) override;

    std::any visitConditionPrimaryWithoutBareBlock(
        RxParser::ConditionPrimaryWithoutBareBlockContext* ctx
    ) override;

    // conditionBreak，限制 break 值的开头不能是裸代码块
    std::any visitConditionBreakAssignmentExpression(
        RxParser::ConditionBreakAssignmentExpressionContext* ctx
    ) override;

    std::any visitConditionBreakLogicalOrExpression(
        RxParser::ConditionBreakLogicalOrExpressionContext* ctx
    ) override;

    std::any visitConditionBreakLogicalAndExpression(
        RxParser::ConditionBreakLogicalAndExpressionContext* ctx
    ) override;

    std::any visitConditionBreakComparisonExpression(
        RxParser::ConditionBreakComparisonExpressionContext* ctx
    ) override;

    std::any visitConditionBreakBitOrExpression(
        RxParser::ConditionBreakBitOrExpressionContext* ctx
    ) override;

    std::any visitConditionBreakClosedBitOrExpression(
        RxParser::ConditionBreakClosedBitOrExpressionContext* ctx
    ) override;

    std::any visitConditionBreakBitXorExpression(
        RxParser::ConditionBreakBitXorExpressionContext* ctx
    ) override;

    std::any visitConditionBreakClosedBitXorExpression(
        RxParser::ConditionBreakClosedBitXorExpressionContext* ctx
    ) override;

    std::any visitConditionBreakBitAndExpression(
        RxParser::ConditionBreakBitAndExpressionContext* ctx
    ) override;

    std::any visitConditionBreakClosedBitAndExpression(
        RxParser::ConditionBreakClosedBitAndExpressionContext* ctx
    ) override;

    std::any visitConditionBreakShiftExpression(
        RxParser::ConditionBreakShiftExpressionContext* ctx
    ) override;

    std::any visitConditionBreakClosedShiftExpression(
        RxParser::ConditionBreakClosedShiftExpressionContext* ctx
    ) override;

    std::any visitConditionBreakAdditiveExpression(
        RxParser::ConditionBreakAdditiveExpressionContext* ctx
    ) override;

    std::any visitConditionBreakClosedAdditiveExpression(
        RxParser::ConditionBreakClosedAdditiveExpressionContext* ctx
    ) override;

    std::any visitConditionBreakMultiplicativeExpression(
        RxParser::ConditionBreakMultiplicativeExpressionContext* ctx
    ) override;

    std::any visitConditionBreakClosedMultiplicativeExpression(
        RxParser::ConditionBreakClosedMultiplicativeExpressionContext* ctx
    ) override;

    std::any visitConditionBreakCastExpression(
        RxParser::ConditionBreakCastExpressionContext* ctx
    ) override;

    std::any visitConditionBreakClosedCastExpression(
        RxParser::ConditionBreakClosedCastExpressionContext* ctx
    ) override;

    std::any visitConditionBreakUnaryExpression(
        RxParser::ConditionBreakUnaryExpressionContext* ctx
    ) override;

    std::any visitConditionBreakPostfixExpression(
        RxParser::ConditionBreakPostfixExpressionContext* ctx
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