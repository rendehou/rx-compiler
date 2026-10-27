#pragma once

#include "rx/ast/program.hpp"
#include "rx/ast/statement.hpp"
#include "rx/ast/expression.hpp"

#include "rx/semantic/type.hpp"
#include "rx/semantic/scope.hpp"

#include <memory>
#include <string>

namespace rx::semantic {

    struct ExprResult {
        Type type;

        bool is_assignable = false;//能否作左值
        bool is_mutable = false;//能否mut

    };

    struct CheckContext {
        // 当前函数返回类型
        Type return_type = Type::makeUnit();

        std::string self_type;
    };

    class Checker {
    public:
        // 整个函数体检查阶段的入口
        bool checkProgram(
            const std::shared_ptr<ast::Program>& program
        );

    private:

        bool checkFunction(
            const std::shared_ptr<ast::FunctionItem>& function,
            const std::string& self_type
        );

        ExprResult checkBlock(
            const std::shared_ptr<ast::Block>& block,
            Scope& parent,
            CheckContext& context
        );

        bool checkStatement(
            const std::shared_ptr<ast::Statement>& statement,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkExpr(
            const std::shared_ptr<ast::Expr>& expr,
            Scope& scope,
            CheckContext& context
        );


        // 具体的Expr
        ExprResult checkUnary(
            const std::shared_ptr<ast::UnaryExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkBinary(
            const std::shared_ptr<ast::BinaryExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkAssignment(
            const std::shared_ptr<ast::AssignmentExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkCast(
            const std::shared_ptr<ast::CastExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        // 函数调用
        ExprResult checkCall(
            const std::shared_ptr<ast::CallExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkMethodCall(
            const std::shared_ptr<ast::MethodCallExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        // 数组和结构体
        ExprResult checkFieldAccess(
            const std::shared_ptr<ast::FieldAccessExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkIndex(
            const std::shared_ptr<ast::IndexExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkArrayList(
            const std::shared_ptr<ast::ArrayListExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkArrayRepeat(
            const std::shared_ptr<ast::ArrayRepeatExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkStructInit(
            const std::shared_ptr<ast::StructInitExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        // 条件和循环
        ExprResult checkIf(
            const std::shared_ptr<ast::IfExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkWhile(
            const std::shared_ptr<ast::WhileExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkLoop(
            const std::shared_ptr<ast::LoopExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkReturn(
            const std::shared_ptr<ast::ReturnExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkBreak(
            const std::shared_ptr<ast::BreakExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkContinue(
            const std::shared_ptr<ast::ContinueExpr>& expr,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkPath(
            const std::shared_ptr<ast::PathExpr>& expr,
            Scope& scope,
            CheckContext& context
        );
    };

} // namespace rx::semantic