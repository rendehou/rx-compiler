#pragma once

#include "rx/ast/program.hpp"
#include "rx/ast/statement.hpp"
#include "rx/ast/expression.hpp"

#include "rx/semantic/type.hpp"
#include "rx/semantic/scope.hpp"

#include <memory>
#include <string>
#include <vector>

namespace rx::semantic {

    struct ExprResult {
        Type type;

        bool is_assignable = false;//能否作左值
        bool is_mutable = false;//能否mut，只有写mut才能赋值，因为引用可以有很多副本，但赋值相当于操作变量所有权，只有一个
        bool mutable_access_blocked = false;// 判断是否因为只读而失去赋值能力
    };

    struct CheckContext {
        // 当前函数返回类型
        Type return_type = Type::makeUnit();

        std::string self_type;
        std::vector<LoopContext> loops;
    };
    struct LoopContext {
        bool allow_value_break = false;
        const Type* expected_type = nullptr;
        
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
            CheckContext& context,
            const Type* expected_type = nullptr
        );

        bool checkStatement(
            const std::shared_ptr<ast::Statement>& statement,
            Scope& scope,
            CheckContext& context
        );

        ExprResult checkExpr(
            const std::shared_ptr<ast::Expr>& expr,
            Scope& scope,
            CheckContext& context,
            const Type* expected_type =nullptr
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
        //辅助函数
        // 获取整数或bool的实际类型，允许一层共享引用 &T
        bool getScalarOperandType(
            const Type& type,
            Type& result
        ) const;

        // 去掉外壳，保留最内层类型，允许共享引用 &T
        bool getIntegerOperandType(
            const Type& type,
            Type& result
        ) const;

        // 判断类型本身或引用指向的最终类型是否为整数或bool
        bool isOrderedType(const Type& type) const;

        // 判断左右两种类型能否进行大小比较
        bool canCompareOrder(
            const Type& left,
            const Type& right
        ) const;

        // 判断类型是否支持 PartialEq
        bool hasPartialEq(const Type& type) const;

        // 判断能否通过内置解引用到达目标类型
        bool canReachByDereference(
            const Type& from,
            const Type& target,
            bool require_mutable
        ) const;

        // 判断右侧类型能否赋给左侧类型
        bool canAssignment(
            const Type& source,
            const Type& target
        ) const;
        
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

        // 自动解引用一层Reference或Box，并更新修改权限
        bool dereferenceForAccess(
            Type& type,
            bool& mutable_access,
            bool& mutable_access_blocked
        ) const;

        // 判断类型是否实现Copy
        bool hasCopy(const Type& type) const;


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
            CheckContext& context,
            const Type* expected_type = nullptr
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