#pragma once

#include <unordered_map>

#include "rx/ast/program.hpp"
#include "rx/ast/expression.hpp"
#include "rx/ast/statement.hpp"

#include "rx/ir/context.hpp"

namespace rx::ir {

    class IRGenerator {
    public:
        explicit IRGenerator(const semantic::SemanticInfo& info);

        // 生成整个模块
        IRModule generateProgram(const ast::Program& program);

    private:
        // 固定的语义信息
        const semantic::SemanticInfo& info_;

        // 正在生成的 IR 模块
        IRModule module_;

        // AST 函数声明 -> IR 函数编号
        std::unordered_map<const ast::FunctionItem*, FunctionId> functions_;

        // ========== 顶层声明 ==========

        // 第一遍收集函数签名、类型、常量等
        void collectDeclarations(const ast::Program& program);

        // 第二遍生成函数体
        void generateFunctions(const ast::Program& program);

        void generateFunction(
            const ast::FunctionItem& function,
            FunctionId id
        );

        // ========== AST 统一入口 ==========

        IRResult generateBlock(
            const ast::Block& block,
            FunctionContext& context
        );

        IRResult generateStatement(
            const ast::Statement& statement,
            FunctionContext& context
        );

        // 默认按“求值”方式生成表达式
        IRResult generateExpr(
            const ast::Expr& expr,
            FunctionContext& context
        );

        // ========== 具体表达式 ==========

        IRResult generateInteger(
            const ast::IntegerExpr& expr,
            FunctionContext& context
        );

        IRResult generateBoolean(
            const ast::BooleanExpr& expr,
            FunctionContext& context
        );

        IRResult generatePath(
            const ast::PathExpr& expr,
            FunctionContext& context
        );

        IRResult generateUnary(
            const ast::UnaryExpr& expr,
            FunctionContext& context
        );

        IRResult generateBinary(
            const ast::BinaryExpr& expr,
            FunctionContext& context
        );

        IRResult generateCast(
            const ast::CastExpr& expr,
            FunctionContext& context
        );

        IRResult generateAssignment(
            const ast::AssignmentExpr& expr,
            FunctionContext& context
        );

        // 控制流
        IRResult generateIf(
            const ast::IfExpr& expr,
            FunctionContext& context
        );

        IRResult generateWhile(
            const ast::WhileExpr& expr,
            FunctionContext& context
        );

        IRResult generateLoop(
            const ast::LoopExpr& expr,
            FunctionContext& context
        );

        IRResult generateBreak(
            const ast::BreakExpr& expr,
            FunctionContext& context
        );

        IRResult generateContinue(
            const ast::ContinueExpr& expr,
            FunctionContext& context
        );

        IRResult generateReturn(
            const ast::ReturnExpr& expr,
            FunctionContext& context
        );

        // 调用
        IRResult generateCall(
            const ast::CallExpr& expr,
            FunctionContext& context
        );

        IRResult generateMethodCall(
            const ast::MethodCallExpr& expr,
            FunctionContext& context
        );

        // 复合类型
        IRResult generateArrayList(
            const ast::ArrayListExpr& expr,
            FunctionContext& context
        );

        IRResult generateArrayRepeat(
            const ast::ArrayRepeatExpr& expr,
            FunctionContext& context
        );

        IRResult generateStructInit(
            const ast::StructInitExpr& expr,
            FunctionContext& context
        );

        // ========== 语句 ==========

        IRResult generateLet(
            const ast::LetStatement& statement,
            FunctionContext& context
        );

        // ========== 共通操作 ==========

        // 取得一个表达式代表的存储位置
        PlaceResult emitPlace(
            const ast::Expr& expr,
            FunctionContext& context
        );

        // 从存储位置读取值
        IRResult emitRead(
            const IRPlace& place,
            FunctionContext& context
        );

        // 向存储位置写入值
        void emitWrite(
            const IRPlace& place,
            const IRValue& value,
            FunctionContext& context
        );

        // 根据 Rx 类型语义执行值传递
        void emitTransfer(
            const IRPlace& target,
            const IRValue& value,
            const semantic::Type& type,
            FunctionContext& context
        );

        // 显式 Clone
        IRResult emitClone(
            const IRValue& value,
            const semantic::Type& type,
            FunctionContext& context
        );

        // Rx 类型转换为 LLVM IR 类型
        IRType lowerType(const semantic::Type& type);

        // 类型布局
        std::size_t sizeOf(const semantic::Type& type);
        std::size_t alignOf(const semantic::Type& type);
    };

}