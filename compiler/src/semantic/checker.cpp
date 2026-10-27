#pragma once

#include "rx/semantic/checker.hpp"
#include "rx/semantic/collector.hpp"

#include <memory>
#include <string>

namespace rx::semantic {


    // 整个函数检查阶段的入口
    bool Checker::checkProgram(
        const std::shared_ptr<ast::Program>& program
    ) {
        if(!program) {
            return false;
        }

        for (const auto& item : program->items) {
            auto function = std::dynamic_pointer_cast<ast::FunctionItem> (item);
            if (function) {
                if(!checkFunction(function,"")) {//先传空字符串，一会处理再放入selftype
                    return false;
                }
            }
        }
        return true;
    }

    bool Checker::checkFunction(
        const std::shared_ptr<ast::FunctionItem>& function,
        const std::string& self_type
    ) {

        if (!function || !function->body) {
            return false;
        }
        if (function->receiver && self_type.empty()) {//有receiver但没有self
            return false;
        }

        //获取返回类型，没有就是unit
        Type return_type = function->return_type ? resolveType(function->return_type, self_type) : Type::makeUnit();
        if (return_type.isError()) {
            return false;
        }

        // 创建函数作用域
        Scope function_scope;

        // 处理参数
        for (const auto& parameter : function->parameters) {
            if (!parameter || !parameter->type) {
                return false;
            }
            if (function_scope.findCurrent(parameter->name)) {
                return false;
            }

            Type parameter_type = resolveType(
                parameter->type, self_type
            );
            if (parameter_type.isError()) {
                return false;
            }
            function_scope.define(//在当前作用域定义函数参数
                parameter->name,
                LocalSymbol{
                    parameter_type,
                    parameter->mutable_binding
                }
            );
        }

        return true;
    }

    ExprResult Checker::checkBlock(
        const std::shared_ptr<ast::Block>& block,
        Scope& parent,
        CheckContext& context
    ){
        if (!block) {
            return {Type::makeError()};
        }

        //创建当前代码块作用域
        Scope scope(&parent);

        // 按顺序检查所有expr
        for (const auto& statement : block->statements) {
            if (!checkStatement(statement, scope, context)) {
                return {Type::makeError()};//表达式或类型检查失败
            }
        }

        //如果有，就检查tail expression，否则是空类型
        if (block->tail) {
            return checkExpr(block->tail, scope, context);
        }
        return {Type::makeUnit()};
    }

    bool Checker::checkStatement(
        const std::shared_ptr<ast::Statement>& statement,
        Scope& scope,
        CheckContext& context
    ) {
        
        if (!statement) {
            return false;
        } 
        if (std::dynamic_pointer_cast<ast::EmptyStatement>(statement)) {//空语句直接返回
            return true;
        }

        // checklet声明语句
        if (auto let = std::dynamic_pointer_cast<ast::LetStatement>(statement)) {
            if (let->name.empty()) {
                return false;
            }

            Type variable_type = Type::makeError();

            // 解析显式声明的类型
            if (let->declared_type) {
                variable_type = resolveType(
                    let->declared_type, context.self_type
                );
                if (variable_type.isError()) {
                    return false;
                }
            }

            // 检查let等号右侧的初始化表达式
            if (let->initializer) {
                ExprResult initializer = checkExpr(let->initializer, scope, context);

                if (initializer.type.isError()) {
                    return false;
                }
                if (let->declared_type) {//有显示类型，就比较
                    if (initializer.type != variable_type) {
                        return false;
                    }
                } else {
                    variable_type = initializer.type;
                }
            }

            if (variable_type.isError()) {
                return false;
            }

            // 定义变量
            scope.define(let->name,
                LocalSymbol{
                    variable_type,
                    let->mutable_binding
                }
            );
            return true;
        }

        // 表达式语句
        if (auto expression = std::dynamic_pointer_cast<ast::ExpressionStatement>(statement)) {
            ExprResult result = checkExpr(
                expression->expression, scope, context
            );

            return !result.type.isError();
        }

        return false;
    }

    ExprResult Checker::checkExpr(
        const std::shared_ptr<ast::Expr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr) {
            return {Type::makeError()};
        }
        // 以下几个种类可以直接解决
        // 具体数
        if (std::dynamic_pointer_cast<ast::IntegerExpr>(expr)) {
            return {Type::makeI32(), false, false};
        }

        // bool
        if (std::dynamic_pointer_cast<ast::BooleanExpr>(expr)) {
            return {Type::makeBool(), false, false};
        }

        // 空expression
        if (std::dynamic_pointer_cast<ast::UnitExpr>(expr)) {
            return {Type::makeUnit(), false, false};
        }

        // path
        if (auto path_expr = std::dynamic_pointer_cast<ast::PathExpr>(expr)) {
            return checkPath(
                path_expr,
                scope,
                context
            );
        }

        // 块语句
        if (auto block_expr = std::dynamic_pointer_cast<ast::BlockExpr>(expr)) {

            return checkBlock(
                block_expr->block,
                scope,
                context
            );
        }


        return {Type::makeError()};
    }

    ExprResult Checker::checkPath(
        const std::shared_ptr<ast::PathExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr || !expr->path || expr->path->segments.empty()) {
            return {Type::makeError()};
        }

        // 检验每一个路径非空
        for (const auto& segment : expr->path->segments) {
            if (!segment) {
                return {Type::makeError()};
            }
        }

        // 只有一段path
        if (expr->path->segments.size() == 1) {
            const std::string& name =
                expr->path->segments[0]->name;

            // 在当前scope将name当作变量查找
            if (auto symbol = scope.find(name)) {
                return {
                    symbol->type,
                    true,
                    symbol->mutable_binding
                };
            }
            // 在全局常量里继续找name的定义
            auto global = value_map.find(name);
            if (global != value_map.end() && global->second.kind == ValueSymbolKind::Constant && global->second.constant) {

                Type type = resolveType(
                    global->second.constant->type,
                    context.self_type
                );
                if (type.isError()) {
                    return {Type::makeError()};
                }

                return {type, false, false};
            }

            return {Type::makeError()};
        }

        // 多段path以后在这里继续处理

        return {Type::makeError()};
    }


    // 所有的check共用一套检查逻辑
    // 检查节点是否存在
    // 递归检查子表达式是否符合规范
    // 检查类型是否合法
    // 返回对应值

    // 检查单目运算符
    ExprResult Checker::checkUnary(
        const std::shared_ptr<ast::UnaryExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr || !expr->operand) {
            return {Type::makeError()};
        }

        ExprResult operand = checkExpr(expr->operand,scope,context);
        if (operand.type.isError()) {
            return {Type::makeError()};
        }

        switch (expr->op) {

            case ast::UnaryOperator::Negate: {
                if (operand.type.isSignedInteger()) {//整数取负
                    return {
                        operand.type,
                        false,
                        false
                    };
                }

                return {Type::makeError()};
            }

            case ast::UnaryOperator::Not: {
                //bool int，以及引用
                if (operand.type.isBool() || operand.type.isInteger()) {

                    return {
                        operand.type,
                        false,
                        false
                    };
                }

                // 也允许 &bool 或 &integer
                if (operand.type.isReference() &&
                    !operand.type.mutable_reference &&
                    operand.type.inner &&
                    (operand.type.inner->isBool() ||
                    operand.type.inner->isInteger())) {

                    return {
                        *operand.type.inner,
                        false,
                        false
                    };
                }

                return {Type::makeError()};
            }

            case ast::UnaryOperator::Dereference: {
                // 可以解引用
                if (operand.type.isReference() && operand.type.inner) {
                    return {
                        *operand.type.inner,
                        true,
                        operand.type.mutable_reference
                    };
                }

                // Box也可以解引用成内部类型
                if (operand.type.isBox() && operand.type.inner) {
                    return {
                        *operand.type.inner,
                        true,
                        operand.is_mutable
                    };
                }

                return {Type::makeError()};
            }

            case ast::UnaryOperator::BorrowShared: {
                return {
                    Type::makeReference(
                        operand.type,
                        false
                    ),
                    false,
                    false
                };
            }

            case ast::UnaryOperator::BorrowMutable: {
                // 如果本来就是一个 place，则必须可修改
                if (operand.is_assignable && !operand.is_mutable) {

                    return {Type::makeError()};
                }

                // value expression可以通过临时值被借用
                return {
                    Type::makeReference(
                        operand.type,
                        true
                    ),
                    false,
                    false
                };
            }
        }
    }
    
} // namespace rx::semantic