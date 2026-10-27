#pragma once

#include "rx/semantic/checker.hpp"

#include <memory>
#include <string>

namespace rx::semantic {


    // 整个函数检查阶段的入口
    bool checkProgram(
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

    bool checkFunction(
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

    ExprResult checkBlock(
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

    bool checkStatement(
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
                if (let->declared_type) {//写了显示类型，就比较
                    if (initializer.type != variable_type) {
                        return false;
                    }
                } else {
                    variable_type = initializer.type;
                }
            }

            // 没有类型，也没有初始化值
            if (variable_type.isError()) {
                return false;
            }

            // 检查完成后定义变量
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

    ExprResult checkExpr(
        const std::shared_ptr<ast::Expr>& expr,
        Scope& scope,
        CheckContext& context
    ) {

    }


} // namespace rx::semantic