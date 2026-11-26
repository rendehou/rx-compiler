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

        // Ir函数编号
        std::unordered_map<ast::NodeId, FunctionId> functions_;


        // 第一遍收集函数签名、类型、常量等
        void collectDeclarations(const ast::Program& program);

        // 第二遍生成函数体
        void generateFunctions(const ast::Program& program);

        void generateFunction(
            const ast::FunctionItem& function,
            FunctionId id
        );

        IRResult generateBlock(
            const ast::Block& block,
            FunctionContext& context
        );

        IRResult generateStatement(
            const ast::Statement& statement,
            FunctionContext& context
        );

        IRResult generateExpr(
            const ast::Expr& expr,
            FunctionContext& context
        );

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

        IRResult generateLet(
            const ast::LetStatement& statement,
            FunctionContext& context
        );


        //辅助函数
        // 取得一个表达式的存储位置
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

        // 显式 Clone
        IRResult emitClone(
            const IRValue& value,
            const semantic::Type& type,
            FunctionContext& context
        );

        // Rx 类型转换为 LLVM IR 类型
        IRType lowerType(const semantic::Type& type);

        std::size_t sizeOf(const semantic::Type& type);//判断占位数
        std::size_t alignOf(const semantic::Type& type);// 调整位数
    };

}