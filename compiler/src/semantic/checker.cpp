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
        CheckContext& context,
        const Type* expected_type
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
            return checkExpr(block->tail, scope, context, expected_type);
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
                ExprResult initializer = checkExpr(
                    let->initializer,
                    scope,
                    context,
                    let->declared_type ? &variable_type : nullptr
                );

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
        CheckContext& context,
        const Type* expected_type
    ) {
        if (!expr) {
            return {Type::makeError()};
        }
        // 以下几个种类可以直接解决
        // 具体数
        if (auto integer = std::dynamic_pointer_cast<ast::IntegerExpr>(expr)) {
            const std::string& spelling = integer->spelling;

            Type integer_type = Type::makeI32();

            // 显式后缀优先于预期类型
            if (spelling.size() >= 5 &&
                spelling.compare(spelling.size() - 5, 5, "usize") == 0) {
                integer_type = Type::makeUSize();
            }
            else if (spelling.size() >= 5 &&
                spelling.compare(spelling.size() - 5, 5, "isize") == 0) {
                integer_type = Type::makeISize();
            }
            else if (spelling.size() >= 3 &&
                spelling.compare(spelling.size() - 3, 3, "u32") == 0) {
                integer_type = Type::makeU32();
            }
            else if (spelling.size() >= 3 &&
                spelling.compare(spelling.size() - 3, 3, "i32") == 0) {
                integer_type = Type::makeI32();
            }
            // 没有显式后缀时，使用预期类型
            else if (expected_type && expected_type->isInteger()) {
                integer_type = *expected_type;
            }

            return {integer_type, false, false};
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
                context,
                expected_type
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
                    return {operand.type, false, false};
                }

                return {Type::makeError()};
            }

            case ast::UnaryOperator::Not: {
                //bool int，以及引用
                if (operand.type.isBool() || operand.type.isInteger()) {

                    return {operand.type, false, false };
                }

                // 也允许 &bool 或 &integer
                if (operand.type.isReference() &&
                    !operand.type.mutable_reference &&
                    operand.type.inner &&
                    (operand.type.inner->isBool() ||
                    operand.type.inner->isInteger())) {

                    return { *operand.type.inner, false, false};
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
                    Type::makeReference(operand.type, false),
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


    //内部判断是否可以比较，这里是辅助函数
    bool canOrder(const Type& lhs, const Type& rhs) {
        // 两侧类型完全相同，并且最终是整数或 bool
        if (lhs == rhs) {
            const Type* current = &lhs;

            while (current->isReference()) {
                if (!current->inner) {
                    return false;
                }
                current = current->inner.get();
            }

            if (current->isInteger() || current->isBool()) {
                return true;
            }
        }
        // 特殊情况：左边是 &T，右边是 &mut T
        if (lhs.isReference() && rhs.isReference() &&
            !lhs.mutable_reference && rhs.mutable_reference &&
            lhs.inner && rhs.inner &&
            *lhs.inner == *rhs.inner) {
            return true;
        }

        return false;
    }

    // 判断类型本身或经过多层引用后，最终是否为整数或 bool
    bool isOrderedType(const Type& type) {
        const Type* current = &type;

        while (current->isReference()) {
            if (!current->inner) {
                return false;
            }

            current = current->inner.get();
        }

        return current->isInteger() || current->isBool();
    }

    // 判断左右两种类型能否进行大小比较
    bool canCompareOrder(const Type& left, const Type& right) {
        if (left == right && isOrderedType(left)) {
            return true;
        }

        // 特殊情况：左侧是 &T，右侧是 &mut，需要判断内层
        if (left.isReference() && right.isReference() &&
            !left.mutable_reference && right.mutable_reference &&
            left.inner && right.inner &&
            *left.inner == *right.inner &&
            isOrderedType(*left.inner)) {
            return true;
        }

        return false;
    }

    // 判断类型是否支持 PartialEq
    bool hasPartialEq(const Type& type) {
        if (type.isInteger() || type.isBool()) {
            return true;
        }

        // 引用、数组、Box、Vec 的内部类型也必须支持 PartialEq
        if (type.isReference() || type.isArray() || type.isBox() || type.isVec()) {
            if (!type.inner) {
                return false;
            }

            return hasPartialEq(*type.inner);
        }

        // 结构体必须声明 derive(PartialEq)
        if (type.isStruct()) {
            auto it = struct_map.find(type.struct_name);

            if (it == struct_map.end() || !it->second) {
                return false;
            }

            for (const auto& attribute : it->second->attributes) {
                if (!attribute) {
                    continue;
                }

                for (auto trait : attribute->traits) {
                    if (trait == ast::DeriveTrait::PartialEq) {
                        return true;
                    }
                }
            }

            return false;
        }

        return false;
    }

    ExprResult Checker::checkBinary(
        const std::shared_ptr<ast::BinaryExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr || !expr->left || !expr->right) {
            return {Type::makeError()};
        }

        // 递归检查左右子表达式
        ExprResult left = checkExpr(expr->left, scope, context);
        if (left.type.isError()) {
            return {Type::makeError()};
        }

        ExprResult right = checkExpr(expr->right, scope, context);
        if (right.type.isError()) {
            return {Type::makeError()};
        }

        switch (expr->op) {

            case ast::BinaryOperator::Add:
            case ast::BinaryOperator::Subtract:
            case ast::BinaryOperator::Multiply:
            case ast::BinaryOperator::Divide:
            case ast::BinaryOperator::Remainder: {
                // 只接受整数或一层共享引用 &整数
                Type left_type;
                Type right_type;

                if (!getIntegerOperandType(left.type, left_type) ||
                    !getIntegerOperandType(right.type, right_type)) {
                    return {Type::makeError()};
                }

                // 两侧必须是相同的整数类型
                if (left_type != right_type) {
                    return {Type::makeError()};
                }

                return {left_type, false, false};
            }

            case ast::BinaryOperator::BitAnd:
            case ast::BinaryOperator::BitOr:
            case ast::BinaryOperator::BitXor: {
                // 允许整数或bool本身，或者一层共享引用 &T，但不允许& mut
                Type left_type;
                Type right_type;

                if (!getScalarOperandType(left.type, left_type) ||
                    !getScalarOperandType(right.type, right_type)) {
                    return {Type::makeError()};
                }

                // 两侧必须是相同的整数或 bool 类型
                if (left_type != right_type) {
                    return {Type::makeError()};
                }

                return {left_type, false, false};
            }

            case ast::BinaryOperator::ShiftLeft:
            case ast::BinaryOperator::ShiftRight: {
                Type left_type;
                Type right_type;

                if (!getIntegerOperandType(left.type, left_type) ||
                    !getIntegerOperandType(right.type, right_type)) {
                    return {Type::makeError()};
                }

                // 两侧都必须是整数，但类型可以不同
                return {left_type, false, false};
            }

            case ast::BinaryOperator::LogicalAnd:
            case ast::BinaryOperator::LogicalOr: {
                // 必须是 bool 本身，不接受引用
                if (!left.type.isBool() || !right.type.isBool()) {
                    return {Type::makeError()};
                }

                return {Type::makeBool(), false, false};
            }

            case ast::BinaryOperator::Equal:
            case ast::BinaryOperator::NotEqual: {

                if (left.type != right.type) {
                    return {Type::makeError()};
                }

                // 判断类型是否实现 PartialEq，与eq有关
                // 结构体必须显式derive PartialEq
                if (!hasPartialEq(left.type)) {
                    return {Type::makeError()};
                }

                return {Type::makeBool(), false, false};
            }

            case ast::BinaryOperator::Less:
            case ast::BinaryOperator::LessEqual:
            case ast::BinaryOperator::Greater:
            case ast::BinaryOperator::GreaterEqual: {

                // 判断类型本身或引用指向的最终类型是否为支持大小比较的整数/bool
                if (!canCompareOrder(left.type, right.type)) {
                    return {Type::makeError()};
                }

                return {Type::makeBool(), false, false};
            }
        }

        return {Type::makeError()};
    }
         ExprResult Checker::checkAssignment(
        const std::shared_ptr<ast::AssignmentExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr || !expr->place || !expr->value) {
            return {Type::makeError()};
        }

        // 检查左侧
        ExprResult place = checkExpr(
            expr->place,
            scope,
            context
        );

        if (place.type.isError() || !place.is_assignable || !place.is_mutable) {
            return {Type::makeError()};
        }

        // 检查右侧
        const Type* expected_value_type = nullptr;

        if (expr->op != ast::AssignmentOperator::ShiftLeft &&
            expr->op != ast::AssignmentOperator::ShiftRight) {
            expected_value_type = &place.type;
        }

        ExprResult value = checkExpr(
            expr->value,
            scope,
            context,
            expected_value_type
        );

        if (value.type.isError()) {
            return {Type::makeError()};
        }

        switch (expr->op) {
            // 普通赋值
            case ast::AssignmentOperator::Assign: {
                if (!isAssignmentCompatible(value.type, place.type)) {
                    return {Type::makeError()};
                }

                return {Type::makeUnit(), false, false};
            }

            // 算术复合赋值
            case ast::AssignmentOperator::Add:
            case ast::AssignmentOperator::Subtract:
            case ast::AssignmentOperator::Multiply:
            case ast::AssignmentOperator::Divide:
            case ast::AssignmentOperator::Remainder: {
                if (!place.type.isInteger()) {
                    return {Type::makeError()};
                }

                // 右侧永不返回时，不会实际执行运算
                if (value.type.isNever()) {
                    return {Type::makeUnit(), false, false};
                }

                Type right_type;

                if (!getIntegerOperandType(
                        value.type,
                        right_type)) {
                    return {Type::makeError()};
                }

                // 两侧必须是相同的整数类型
                if (right_type != place.type) {
                    return {Type::makeError()};
                }

                return {Type::makeUnit(), false, false};
            }

            // 位运算复合赋值
            case ast::AssignmentOperator::BitAnd:
            case ast::AssignmentOperator::BitOr:
            case ast::AssignmentOperator::BitXor: {
                if (!place.type.isInteger() &&
                    !place.type.isBool()) {
                    return {Type::makeError()};
                }

                if (value.type.isNever()) {
                    return {Type::makeUnit(), false, false};
                }

                Type right_type;

                if (!getScalarOperandType(
                        value.type,
                        right_type)) {
                    return {Type::makeError()};
                }

                // 两侧必须是相同的整数或bool类型
                if (right_type != place.type) {
                    return {Type::makeError()};
                }

                return {Type::makeUnit(), false, false};
            }

            // 移位复合赋值
            case ast::AssignmentOperator::ShiftLeft:
            case ast::AssignmentOperator::ShiftRight: {
                if (!place.type.isInteger()) {
                    return {Type::makeError()};
                }

                if (value.type.isNever()) {
                    return {Type::makeUnit(), false, false};
                }

                Type right_type;

                if (!getIntegerOperandType(value.type, right_type)) {
                    return {Type::makeError()};
                }

                // 右侧必须是整数，但不要求与左侧同类型
                return {Type::makeUnit(), false, false};
            }
        }

        return {Type::makeError()};
    }

    ExprResult Checker::checkCast(
        const std::shared_ptr<ast::CastExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr || !expr->value || !expr->target_type) {
            return {Type::makeError()};
        }

        // 检查待转换的表达式
        ExprResult value = checkExpr(
            expr->value, scope, context
        );

        if (value.type.isError()) {
            return {Type::makeError()};
        }

        // 解析目标类型
        Type target_type = resolveType(
            expr->target_type,
            context.self_type
        );

        if (target_type.isError()) {
            return {Type::makeError()};
        }

        // 目标类型必须是整数
        if (!target_type.isInteger()) {
            return {Type::makeError()};
        }

        // 源类型必须是整数或bool
        if (!value.type.isInteger() && !value.type.isBool()) {
            return {Type::makeError()};
        }

        // 返回转换后的类型
        return {target_type, false, false};
    }

    ExprResult Checker::checkFieldAccess(//a.b
        const std::shared_ptr<ast::FieldAccessExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr || !expr->base || expr->field.empty()) {
            return {Type::makeError()};
        }

        // 检查base
        ExprResult base = checkExpr(
            expr->base,
            scope,
            context
        );

        if (base.type.isError()) {
            return {Type::makeError()};
        }

        Type current_type = base.type;

        // 普通对象：是否可修改来自base本身
        // value expression：会形成temporary，可以修改
        bool mutable_access;

        if (base.is_assignable) {
            mutable_access = base.is_mutable;
        } else {
            mutable_access = true;
        }

        // 记录是否经过共享引用
        bool crossed_shared = base.mutable_access_blocked;

        if (crossed_shared) {
            mutable_access = false;
        }

        // 不断自动解引用，直到找到struct
        while (!current_type.isStruct()) {

            if (!dereferenceForAccess(
                    current_type,
                    mutable_access,
                    crossed_shared)) {
                return {Type::makeError()};
            }
        }

        // 找struct定义
        auto structure = struct_map.find(current_type.struct_name);
        if (structure == struct_map.end() || !structure->second) {
            return {Type::makeError()};
        }

        // 找字段，structure是存储struct里面字段的map
        for (const auto& field : structure->second->fields) {

            if (!field || !field->type) {
                continue;
            }

            if (field->name == expr->field) {

                Type field_type = resolveType(
                    field->type,
                    current_type.struct_name
                );

                if (field_type.isError()) {
                    return {Type::makeError()};
                }

                return {
                    field_type,
                    true,
                    mutable_access,
                    crossed_shared
                };
            }
        }

        // 字段不存在
        return {Type::makeError()};
    }
        // 判断能否通过内置解引用到达目标类型
    bool Checker::canReachByDereference(
        const Type& from,
        const Type& target,
        bool require_mutable
    ) const {
        Type current = from;

        while (true) {
            if (current == target) {
                return true;
            }

            if (current.isReference() && current.inner) {
                if (require_mutable && !current.mutable_reference) {
                    return false;
                }

                current = *current.inner;
                continue;
            }

            if (current.isBox() && current.inner) {
                current = *current.inner;
                continue;
            }

            return false;
        }
}


    // 判断右侧类型能否赋给左侧类型
    bool Checker::isAssignmentCompatible(
        const Type& source,
        const Type& target
    ) const {
        if (source.isNever() || source == target) {
            return true;
        }

        // 只允许引用之间的自动借用调整
        if (!source.isReference() || !target.isReference() ||
            !source.inner || !target.inner) {
            return false;
        }

        // 目标需要可变引用时，源也必须是可变引用
        if (target.mutable_reference && !source.mutable_reference) {
            return false;
        }

        return canReachByDereference(
            *source.inner,
            *target.inner,
            target.mutable_reference
        );
    }


    // 自动解引用一层，并更新修改权限
    bool Checker::dereferenceForAccess(
        Type& type,
        bool& mutable_access,
        bool& mutable_access_blocked
    ) const {
        // Reference
        if (type.isReference() && type.inner) {

            if (!type.mutable_reference) {
                mutable_access = false;
                mutable_access_blocked = true;
            } else if (!mutable_access_blocked) {
                mutable_access = true;
            }

            type = *type.inner;
            return true;
        }

        // Box
        if (type.isBox() && type.inner) {
            type = *type.inner;
            return true;
        }

        return false;
    }


    // 判断类型是否实现Copy
    bool Checker::hasCopy(const Type& type) const {

        if (type.isInteger() || type.isBool() || type.isUnit()) {
            return true;
        }

        // 共享引用可以Copy，可变引用不可以
        if (type.isReference()) {
            return type.inner && !type.mutable_reference;
        }

        // 数组的元素必须实现Copy
        if (type.isArray()) {
            return type.inner && hasCopy(*type.inner);
        }

        // Box和Vec不支持Copy
        if (type.isBox() || type.isVec()) {
            return false;
        }

        // 结构体必须同时derive Copy和Clone
        if (type.isStruct()) {
            auto structure = struct_map.find(type.struct_name);

            if (structure == struct_map.end() || !structure->second) {
                return false;
            }

            bool has_copy = false;
            bool has_clone = false;

            for (const auto& attribute : structure->second->attributes) {
                if (!attribute) {
                    continue;
                }

                for (auto trait : attribute->traits) {
                    if (trait == ast::DeriveTrait::Copy) {
                        has_copy = true;
                    }

                    if (trait == ast::DeriveTrait::Clone) {
                        has_clone = true;
                    }
                }
            }

            if (!has_copy || !has_clone) {
                return false;
            }

            // 所有字段也必须实现Copy
            for (const auto& field : structure->second->fields) {
                if (!field || !field->type) {
                    return false;
                }

                Type field_type = resolveType(
                    field->type,
                    type.struct_name
                );

                if (field_type.isError() || !hasCopy(field_type)) {
                    return false;
                }
            }

            return true;
        }

        return false;
    }

    ExprResult Checker::checkIndex(
        const std::shared_ptr<ast::IndexExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr || !expr->base || !expr->index) {
            return {Type::makeError()};
        }

        // 检查base
        ExprResult base = checkExpr(
            expr->base,
            scope,
            context
        );

        if (base.type.isError()) {
            return {Type::makeError()};
        }

        // 检查index
        // 注意：这里需要给checkExpr增加expected_type支持，
        // 使无后缀的整数索引按照usize推断

        Type usize_type = Type::makeUSize();

        ExprResult index = checkExpr(
            expr->index,
            scope,
            context,
            &usize_type
        );

        if (index.type.isError()) {
            return {Type::makeError()};
        }

        // index必须是usize
        if (!index.type.isNever() && index.type != usize_type) {
            return {Type::makeError()};
        }

        // never类型不会产生正常的索引结果
        if (index.type.isNever()) {
            return {Type::makeNever()};
        }

        Type current_type = base.type;

        // 普通place继承原来的修改权限，临时值可以修改
        bool mutable_access = base.is_assignable
            ? base.is_mutable
            : true;

        bool crossed_shared = base.mutable_access_blocked;

        if (crossed_shared) {
            mutable_access = false;
        }

        // 自动解引用，直到找到Array或Vec
        while (!current_type.isArray() && !current_type.isVec()) {

            if (!dereferenceForAccess(
                    current_type,
                    mutable_access,
                    crossed_shared)) {
                return {Type::makeError()};
            }
        }

        // 检查inner，取得元素类型
        if (!current_type.inner) {
            return {Type::makeError()};
        }

        Type element_type = *current_type.inner;

        // Array索引不额外引入借用，直接继承原来的修改权限
        if (current_type.isArray()) {
            return {
                element_type,
                true,
                mutable_access,
                crossed_shared
            };
        }

        // Vec索引会隐式借用Vec
        // 如果Vec本身不能修改，那么通过索引得到的元素也不能进行修改
        if (current_type.isVec()) {

            if (!mutable_access) {
                crossed_shared = true;
            }

            return {
                element_type,
                true,
                mutable_access,
                crossed_shared
            };
        }

        return {Type::makeError()};
    }
} // namespace rx::semantic