#include "rx/semantic/checker.hpp"
#include "rx/semantic/collector.hpp"
#include "rx/semantic/constevaluator.hpp"

#include <memory>
#include <string>

namespace rx::semantic {


    // 整个函数检查阶段的入口
    bool Checker::checkProgram(
        const std::shared_ptr<ast::Program>& program
    ) {
        if (!program) {
            return false;
        }

        // 检查AST节点
        for (const auto& item : program->items) {
            if (!item) {
                return false;
            }
        }

        // 收集全局函数、常量、结构体
        if (!collectItems(program)) {
            return false;
        }

        // 检查全局声明
        if (!checkDeclarations(program)) {
            return false;
        }


        // 检查程序入口main
        auto main_it = value_map.find("main");

        if (main_it == value_map.end() ||
            main_it->second.kind != ValueSymbolKind::Function ||
            !main_it->second.function) {
            return false;
        }

        auto main_function = main_it->second.function;

        // main不能有普通参数或receiver
        if (main_function->receiver ||
            !main_function->parameters.empty()) {
            return false;
        }

        // main不能带泛型参数
        if (main_function->generic_parameters &&
            !main_function->generic_parameters->lifetimes.empty()) {
            return false;
        }

        // main返回值必须是Unit
        Type main_return = main_function->return_type
            ? resolveType(main_function->return_type)
            : Type::makeUnit();

        if (!main_return.isUnit()) {
            return false;
        }

        // 检查struct的derive是否合法
        for (const auto& item : program->items) {

            if (auto structure = std::dynamic_pointer_cast<ast::StructItem>(item)) {

                if (!checkStructDerives(structure)) {
                    return false;
                }
            }
        }

        // 检查函数体
        for (const auto& item : program->items) {

            if (auto function = std::dynamic_pointer_cast<ast::FunctionItem>(item)) {

                if (!checkFunction(function, "")) {
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

        // 有receiver时，必须处于某个impl中
        if (function->receiver && self_type.empty()) {
            return false;
        }

        // 检查Self对应的结构体是否存在
        if (!self_type.empty() &&
            struct_map.find(self_type) == struct_map.end()) {
            return false;
        }

        // 获取函数返回类型，没有就是Unit
        Type return_type = function->return_type ? resolveType(function->return_type, self_type) : Type::makeUnit();

        if (return_type.isError()) {
            return false;
        }

        // 创建函数作用域
        Scope function_scope;

        // 处理self接收者
        if (function->receiver) {

            Type receiver_type = Type::makeStruct(self_type);
            bool mutable_binding = false;

            switch (function->receiver->kind) {
                // self
                case ast::ReceiverKind::Value:
                    break;

                // mut self
                case ast::ReceiverKind::MutableValue:
                    mutable_binding = true;
                    break;

                // &self
                case ast::ReceiverKind::SharedReference:
                    receiver_type = Type::makeReference(
                        receiver_type,
                        false
                    );
                    break;

                // &mut self
                case ast::ReceiverKind::MutableReference:
                    receiver_type = Type::makeReference(
                        receiver_type,
                        true
                    );
                    break;
            }

            function_scope.define(
                "self",
                LocalSymbol{
                    receiver_type,
                    mutable_binding
                }
            );
        }

        // 处理普通参数
        for (const auto& parameter : function->parameters) {

            if (!parameter || !parameter->type ||
                parameter->name.empty()) {
                return false;
            }

            // 检查参数是否重名
            if (function_scope.findCurrent(parameter->name)) {
                return false;
            }

            Type parameter_type = resolveType(
                parameter->type,
                self_type
            );

            if (parameter_type.isError()) {
                return false;
            }

            // 将参数加入函数作用域
            function_scope.define(
                parameter->name,
                LocalSymbol{
                    parameter_type,
                    parameter->mutable_binding
                }
            );
        }

        // 创建当前函数的检查上下文
        CheckContext context;

        context.return_type = return_type;
        context.self_type = self_type;

        // 检查函数体
        ExprResult body_result = checkBlock(
            function->body,
            function_scope,
            context,
            &return_type
        );

        if (body_result.type.isError()) {
            return false;
        }

        // 函数体最终类型必须能赋给声明的返回类型
        if (!canAssignment(body_result.type, return_type)) {
            return false;
        }
        return true;
    }

    ExprResult Checker::checkBlock(
        const std::shared_ptr<ast::Block>& block,
        Scope& parent,
        CheckContext& context,
        const Type* expected_type
    ) {
        if (!block) {
            return {Type::makeError()};
        }

        // 创建当前代码块作用域
        Scope scope(&parent);

        // 记录当前Block是否已经发散
        bool fasan = false;

        // 按顺序检查所有Statement
        for (const auto& statement : block->statements) {

            ExprResult result = checkStatement(
                statement,
                scope,
                context
            );

            if (result.type.isError()) {
                return {Type::makeError()};
            }

            // 即使已经发散，也继续检查后面的语句
            if (result.type.isNever()) {
                fasan = true;
            }
        }

        // 检查tail expression
        if (block->tail) {

            ExprResult tail_result = checkExpr(
                block->tail,
                scope,
                context,
                expected_type
            );

            if (tail_result.type.isError()) {
                return {Type::makeError()};
            }

            // 前面的语句已经发散，或者tail本身发散
            if (fasan || tail_result.type.isNever()) {
                return {Type::makeNever()};
            }

            return {tail_result.type, false, false};
        }

        // 没有tail
        if (fasan) {
            return {Type::makeNever()};
        }

        return {Type::makeUnit()};
    }

    ExprResult Checker::checkStatement(
        const std::shared_ptr<ast::Statement>& statement,
        Scope& scope,
        CheckContext& context
    ) {
        if (!statement) {
            return {Type::makeError()};
        }

        // 空语句
        if (std::dynamic_pointer_cast<ast::EmptyStatement>(statement)) {
            return {Type::makeUnit()};
        }

        // let声明语句
        if (auto let = std::dynamic_pointer_cast<ast::LetStatement>(statement)) {
            if (let->name.empty()) {
                return {Type::makeError()};
            }

            Type variable_type = Type::makeError();
            bool initializer_never = false;

            // 解析显式声明的类型
            if (let->declared_type) {
                variable_type = resolveType(
                    let->declared_type,
                    context.self_type
                );

                if (variable_type.isError()) {
                    return {Type::makeError()};
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
                    return {Type::makeError()};
                }

                initializer_never = initializer.type.isNever();

                if (let->declared_type) {
                    // 有显示类型，就比较
                    if (!canAssignment(initializer.type, variable_type)) {
                        return {Type::makeError()};
                    }
                }
                else {
                    // 没有显式类型，通过初始化值推断
                    variable_type = initializer.type;
                }
            }

            // 没有类型，也没有初始化值
            if (variable_type.isError()) {
                return {Type::makeError()};
            }

            // 定义变量
            scope.define(
                let->name,
                LocalSymbol{
                    variable_type,
                    let->mutable_binding
                }
            );

            // 初始化表达式发散，整个let语句也发散
            if (initializer_never) {
                return {Type::makeNever()};
            }

            return {Type::makeUnit()};
        }

        // 表达式语句
        if (auto expression = std::dynamic_pointer_cast<ast::ExpressionStatement>(statement)) {

            if (!expression->expression) {
                return {Type::makeError()};
            }

            ExprResult result = checkExpr(
                expression->expression,
                scope,
                context
            );

            if (result.type.isError()) {
                return {Type::makeError()};
            }

            // Never表达式无论有没有分号，都会发散
            if (result.type.isNever()) {
                return {Type::makeNever()};
            }

            // 没有分号的非末尾表达式必须是Unit
            if (!expression->has_semicolon && !result.type.isUnit()) {
                return {Type::makeError()};
            }

            // 普通表达式语句不保留表达式的值
            return {Type::makeUnit()};
        }

        return {Type::makeError()};
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
            return checkPath(path_expr, scope, context);
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

        // 一元运算
        if (auto unary_expr = std::dynamic_pointer_cast<ast::UnaryExpr>(expr)) {
            return checkUnary(unary_expr, scope, context);
        }

        // 二元运算
        if (auto binary_expr = std::dynamic_pointer_cast<ast::BinaryExpr>(expr)) {
            return checkBinary(binary_expr, scope, context);
        }

        // 赋值表达式
        if (auto assignment_expr = std::dynamic_pointer_cast<ast::AssignmentExpr>(expr)) {
            return checkAssignment(assignment_expr, scope, context);
        }

        // 类型转换
        if (auto cast_expr = std::dynamic_pointer_cast<ast::CastExpr>(expr)) {
            return checkCast(cast_expr, scope, context);
        }

        // 字段访问
        if (auto field_expr = std::dynamic_pointer_cast<ast::FieldAccessExpr>(expr)) {
            return checkFieldAccess(field_expr, scope, context);
        }

        // 数组下标
        if (auto index_expr = std::dynamic_pointer_cast<ast::IndexExpr>(expr)) {
            return checkIndex(index_expr, scope, context);
        }

        // 数组列表
        if (auto array_list = std::dynamic_pointer_cast<ast::ArrayListExpr>(expr)) {
            return checkArrayList(array_list, scope, context);
        }

        // 数组重复
        if (auto array_repeat = std::dynamic_pointer_cast<ast::ArrayRepeatExpr>(expr)) {
            return checkArrayRepeat(array_repeat, scope, context);
        }

        // 结构体初始化
        if (auto struct_init = std::dynamic_pointer_cast<ast::StructInitExpr>(expr)) {
            return checkStructInit(struct_init, scope, context);
        }

        // if表达式
        if (auto if_expr = std::dynamic_pointer_cast<ast::IfExpr>(expr)) {
            return checkIf(if_expr, scope, context, expected_type);
        }

        // while循环
        if (auto while_expr = std::dynamic_pointer_cast<ast::WhileExpr>(expr)) {
            return checkWhile(while_expr, scope, context);
        }

        // loop循环
        if (auto loop_expr = std::dynamic_pointer_cast<ast::LoopExpr>(expr)) {
            return checkLoop(loop_expr, scope, context);
        }

        // return
        if (auto return_expr = std::dynamic_pointer_cast<ast::ReturnExpr>(expr)) {
            return checkReturn(return_expr, scope, context);
        }

        // break
        if (auto break_expr = std::dynamic_pointer_cast<ast::BreakExpr>(expr)) {
            return checkBreak(break_expr, scope, context);
        }

        // continue
        if (auto continue_expr = std::dynamic_pointer_cast<ast::ContinueExpr>(expr)) {
            return checkContinue(continue_expr, scope, context);
        }

        // 函数调用
        if (auto call_expr = std::dynamic_pointer_cast<ast::CallExpr>(expr)) {
            return checkCall(call_expr, scope, context);
        }

        // 方法调用
        if (auto method_expr = std::dynamic_pointer_cast<ast::MethodCallExpr>(expr)) {
            return checkMethodCall(method_expr, scope, context);
        }

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

                    bool blocked = operand.mutable_access_blocked || !operand.type.mutable_reference;
                    return {
                        *operand.type.inner,
                        true,
                        operand.type.mutable_reference && !operand.mutable_access_blocked,
                        blocked
                    };
                }

                // Box也可以解引用成内部类型
                if (operand.type.isBox() && operand.type.inner) {
                    return {
                        *operand.type.inner,
                        true,
                        operand.is_mutable && !operand.mutable_access_blocked,
                        operand.mutable_access_blocked
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

        return {Type::makeError()};
    }


    // 获取整数或 bool 的实际类型，允许一层共享引用 &T。
    bool Checker::getScalarOperandType(
        const Type& type,
        Type& result
    ) const {
        if (type.isInteger() || type.isBool()) {
            result = type;
            return true;
        }

        if (type.isReference() &&
            !type.mutable_reference &&
            type.inner &&
            (type.inner->isInteger() || type.inner->isBool())) {
            result = *type.inner;
            return true;
        }

        return false;
    }

    // 获取整数的实际类型，允许一层共享引用 &T。
    bool Checker::getIntegerOperandType(
        const Type& type,
        Type& result
    ) const {
        return getScalarOperandType(type, result) && result.isInteger();
    }

    // 判断类型本身或经过多层引用后，最终是否为整数或 bool。
    bool Checker::isOrderedType(const Type& type) const {
        const Type* current = &type;

        while (current->isReference()) {
            if (!current->inner) {
                return false;
            }

            current = current->inner.get();
        }

        return current->isInteger() || current->isBool();
    }

    // 判断左右两种类型能否进行大小比较。
    bool Checker::canCompareOrder(
        const Type& left,
        const Type& right
    ) const {
        if (left == right && isOrderedType(left)) {
            return true;
        }

        // 特殊情况：左侧是 &T，右侧是 &mut T，需要判断内层。
        if (left.isReference() && right.isReference() &&
            !left.mutable_reference && right.mutable_reference &&
            left.inner && right.inner &&
            *left.inner == *right.inner &&
            isOrderedType(*left.inner)) {
            return true;
        }

        return false;
    }

    // 判断类型是否支持 PartialEq。
    bool Checker::hasPartialEq(const Type& type) const {
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

        // 除了左右移位，这个赋值都应该类型相同
        if (expr->op != ast::AssignmentOperator::ShiftLeft && expr->op != ast::AssignmentOperator::ShiftRight) {
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

                if (!canAssignment(value.type, place.type)) {
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

                if (!getIntegerOperandType(value.type, right_type)) {
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
                if (!place.type.isInteger() && !place.type.isBool()) {
                    return {Type::makeError()};
                }

                if (value.type.isNever()) {
                    return {Type::makeUnit(), false, false};
                }

                Type right_type;

                if (!getScalarOperandType(value.type, right_type)) {
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

    ExprResult Checker::checkFieldAccess(//a.b，先检查，然后找a，直到找到一个struct，再在struct内部找是否存在方法
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

        Type current_type = base.type;//从这个开始不断解引用得到内层
        bool mutable_access;

        if (base.is_assignable) {//是实际存储位置
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
            if (!dereferenceForAccess(current_type, mutable_access, crossed_shared)) {
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

    // 判断能否通过解引用到达目标类型
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
    bool Checker::canAssignment(
        const Type& source,
        const Type& target
    ) const {
        if (source.isNever() || source == target) {
            return true;
        }

        // 允许一个引用的差别
        if (!source.isReference() || !target.isReference() || !source.inner || !target.inner) {
            return false;
        }
        // 都必须是可变引用
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
            } else if (!mutable_access_blocked) {//经过了一层引用就还是不能赋值
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

    
    // 判断类型是否实现Clone，要不实现，要不结构体derive且天生可以clone
    bool Checker::hasClone(const Type& type) const {

        std::vector<std::string> visiting;
        return hasClone(type, visiting);
    }

    bool Checker::hasClone(
        const Type& type,
        std::vector<std::string>& visiting
    ) const {

        if (type.isInteger() || type.isBool() || type.isUnit()) {
            return true;
        }

        // 共享引用可以Clone，可变引用不可以
        if (type.isReference()) {
            return type.inner && !type.mutable_reference;
        }

        // 数组、Box、Vec的内部类型必须支持Clone
        if (type.isArray() || type.isBox() || type.isVec()) {
            return type.inner && hasClone(*type.inner, visiting);
        }

        // 结构体必须derive Clone
        if (type.isStruct()) {

            auto structure = struct_map.find(type.struct_name);

            if (structure == struct_map.end() || !structure->second) {
                return false;
            }

            bool has_clone = false;

            for (const auto& attribute : structure->second->attributes) {

                if (!attribute) {
                    continue;
                }

                for (auto trait : attribute->traits) {
                    if (trait == ast::DeriveTrait::Clone) {
                        has_clone = true;
                    }
                }
            }

            if (!has_clone) {
                return false;
            }

            // 防止递归结构体导致无限递归
            for (const auto& name : visiting) {
                if (name == type.struct_name) {
                    return true;
                }
            }

            visiting.push_back(type.struct_name);

            // 所有字段也需要支持Clone
            for (const auto& field : structure->second->fields) {

                if (!field || !field->type) {
                    visiting.pop_back();
                    return false;
                }

                Type field_type = resolveType(
                    field->type,
                    type.struct_name
                );

                if (field_type.isError() ||
                    !hasClone(field_type, visiting)) {

                    visiting.pop_back();
                    return false;
                }
            }

            visiting.pop_back();

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

        // index是never类型不会产生正常的索引结果
        if (index.type.isNever()) {
            return {Type::makeNever()};
        }

        Type current_type = base.type;

        // 检查mutable权限
        bool mutable_access = base.is_assignable ? base.is_mutable : true;

        bool crossed_shared = base.mutable_access_blocked;

        if (crossed_shared) {
            mutable_access = false;
        }

        // 自动解引用，直到找到Array或Vec
        while (!current_type.isArray() && !current_type.isVec()) {

            if (!dereferenceForAccess(current_type, mutable_access, crossed_shared)) {
                return {Type::makeError()};
            }
        }

        // 检查inner，取得元素类型
        if (!current_type.inner) {
            return {Type::makeError()};
        }

        Type element_type = *current_type.inner;

        // Array索引继承修改权限
        if (current_type.isArray()) {
            return {
                element_type,
                true,
                mutable_access,
                crossed_shared
            };
        }

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


    ExprResult Checker::checkArrayList(
        const std::shared_ptr<ast::ArrayListExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr) {
            return {Type::makeError()};
        }

        Type element_type = Type::makeError();

        bool has_element = false;
        bool fasan = false;//判断是不是never类型

        std::vector<Type> previous_types;

        // 逐个检查数组元素
        for (const auto& element : expr->elements) {

            if (!element) {
                return {Type::makeError()};
            }

            ExprResult result = checkExpr(
                element,
                scope,
                context
            );

            if (result.type.isError()) {
                return {Type::makeError()};
            }

            // never不参与共同类型推断
            if (result.type.isNever()) {
                fasan = true;
                continue;
            }

            // 第一个元素直接作为初始类型
            if (!has_element) {
                element_type = result.type;
                has_element = true;
            }
            else {
                // 如果不能赋值，就尝试使用当前元素的类型作为新的共同类型
                if (!canAssignment(result.type, element_type) && canAssignment(element_type, result.type)) {

                    // 之前所有元素也必须能够转换为新类型
                    for (const auto& previous : previous_types) {
                        if (!canAssignment(previous, result.type)) {
                            return {Type::makeError()};
                        }
                    }

                    element_type = result.type;
                }
                else {
                    return {Type::makeError()};
                }
            }

            previous_types.push_back(result.type);
        }

        // 有元素发散，整个数组表达式发散
        if (fasan) {
            return {Type::makeNever()};
        }

        // 空数组无法推断元素类型
        if (!has_element) {
            return {Type::makeError()};
        }

        return {
            Type::makeArray(element_type, static_cast<std::uint64_t>(expr->elements.size())),
            false,
            false
        };
    }

    ExprResult Checker::checkArrayRepeat(
        const std::shared_ptr<ast::ArrayRepeatExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr || !expr->element || !expr->count) {
            return {Type::makeError()};
        }

        // 计算数组长度
        ConstEvaluator evaluator;
        ConstValue count_value;

        Type usize_type = Type::makeUSize();

        // 数组长度必须是编译期常量
        if (!evaluator.evaluateValue(expr->count, usize_type, count_value)) {
            return {Type::makeError()};
        }

        std::uint64_t count = count_value.unsigned_value;

        // 检查重复的元素
        ExprResult element = checkExpr(
            expr->element,
            scope,
            context
        );

        if (element.type.isError()) {
            return {Type::makeError()};
        }

        // 元素表达式发散，整个数组表达式发散
        if (element.type.isNever()) {
            return {Type::makeNever()};
        }

        // 重复次数大于1时，元素必须实现Copy
        if (count > 1 && !hasCopy(element.type)) {
            return {Type::makeError()};
        }

        return {
            Type::makeArray(element.type, count),
            false,
            false
        };
    }

    // 检查总体类型与path，和定义，检查每个字段
    ExprResult Checker::checkStructInit(
        const std::shared_ptr<ast::StructInitExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr || !expr->path || expr->path->segments.empty()) {
            return {Type::makeError()};
        }

        // 检查path
        for (const auto& segment : expr->path->segments) {
            if (!segment) {
                return {Type::makeError()};
            }
        }

        // 当前只支持单段结构体路径，多段我还没想好
        if (expr->path->segments.size() != 1) {
            return {Type::makeError()};
        }

        std::string struct_name = expr->path->segments[0]->name;

        // Self
        if (struct_name == "Self") {
            if (context.self_type.empty()) {
                return {Type::makeError()};
            }

            struct_name = context.self_type;
        }

        // 找struct定义
        auto structure = struct_map.find(struct_name);

        if (structure == struct_map.end() || !structure->second) {
            return {Type::makeError()};
        }

        std::vector<std::string> initialized_fields;

        bool has_never = false;

        // 检查每一个初始化字段
        for (const auto& initializer : expr->fields) {
            if (!initializer || initializer->name.empty() || !initializer->value) {
                return {Type::makeError()};
            }

            // 检查字段是否重复
            for (const auto& name : initialized_fields) {
                if (name == initializer->name) {
                    return {Type::makeError()};
                }
            }

            // 找字段定义
            std::shared_ptr<ast::StructField> field_definition;
            for (const auto& field : structure->second->fields) {

                if (!field || !field->type) {
                    return {Type::makeError()};
                }

                if (field->name == initializer->name) {
                    field_definition = field;
                    break;
                }
            }

            // 字段不存在
            if (!field_definition) {
                return {Type::makeError()};
            }

            // 解析字段类型
            Type field_type = resolveType(
                field_definition->type,
                struct_name
            );
            if (field_type.isError()) {
                return {Type::makeError()};
            }

            // 检查字段值
            ExprResult value = checkExpr(
                initializer->value,
                scope,
                context,
                &field_type
            );

            if (value.type.isError()) {
                return {Type::makeError()};
            }

            // 字段值必须能够赋给字段类型
            if (!canAssignment(value.type, field_type)) {
                return {Type::makeError()};
            }
            if (value.type.isNever()) {
                has_never = true;
            }

            initialized_fields.push_back(initializer->name);
        }

        // 检查是否所有字段都已经初始化
        for (const auto& field : structure->second->fields) {

            if (!field || !field->type || field->name.empty()) {
                return {Type::makeError()};
            }

            bool initialized = false;
            for (const auto& name : initialized_fields) {
                if (name == field->name) {
                    initialized = true;
                    break;
                }
            }

            if (!initialized) {
                return {Type::makeError()};
            }
        }

        // 如果字段初始化过程中发散，整个表达式也发散
        if (has_never) {
            return {Type::makeNever()};
        }
        return {Type::makeStruct(struct_name), false, false};
    }

    bool Checker::checkStructDerives(const std::shared_ptr<ast::StructItem>& structure) const {

        if (!structure) {
            return false;
        }

        bool has_copy = false;
        bool has_clone = false;

        std::vector<ast::DeriveTrait> traits;

        for (const auto& attribute : structure->attributes) {

            if (!attribute) {
                return false;
            }

            for (auto trait : attribute->traits) {

                // 检查derive是否重复
                for (auto appeared : traits) {
                    if (appeared == trait) {
                        return false;
                    }
                }

                traits.push_back(trait);

                if (trait == ast::DeriveTrait::Copy) {
                    has_copy = true;
                }
                if (trait == ast::DeriveTrait::Clone) {
                    has_clone = true;
                }
            }
        }

        // derive Copy必须同时能derive Clone
        if (has_copy && !has_clone) {
            return false;
        }

        Type struct_type = Type::makeStruct(structure->name);

        // 检查是否真的如derive声明
        if (has_clone && !hasClone(struct_type)) {
            return false;
        }
        if (has_copy && !hasCopy(struct_type)) {
            return false;
        }

        return true;
    }

    ExprResult Checker::checkIf(
        const std::shared_ptr<ast::IfExpr>& expr,
        Scope& scope,
        CheckContext& context,
        const Type* expected_type
    ) {
        if (!expr || !expr->condition || !expr->then_block) {
            return {Type::makeError()};
        }

        // 检查condition
        Type bool_type = Type::makeBool();

        ExprResult condition = checkExpr(
            expr->condition,
            scope,
            context,
            &bool_type
        );

        if (condition.type.isError()) {
            return {Type::makeError()};
        }

        if (!condition.type.isNever() && !condition.type.isBool()) {
            return {Type::makeError()};
        }

        // 没有else
        if (!expr->else_branch) {

            Type unit_type = Type::makeUnit();

            ExprResult then_result = checkBlock(
                expr->then_block,
                scope,
                context,
                &unit_type
            );

            if (then_result.type.isError()) {
                return {Type::makeError()};
            }

            // then必须是Unit或者Never
            if (!then_result.type.isNever() && !then_result.type.isUnit()) {
                return {Type::makeError()};
            }

            if (condition.type.isNever()) {
                return {Type::makeNever()};
            }

            if (expected_type && !canAssignment(unit_type, *expected_type)) {
                return {Type::makeError()};
            }

            return {Type::makeUnit(), false, false};
        }

        // 检查then
        ExprResult then_result = checkBlock(
            expr->then_block,
            scope,
            context,
            expected_type
        );

        if (then_result.type.isError()) {
            return {Type::makeError()};
        }

        // 检查else
        ExprResult else_result = checkExpr(
            expr->else_branch,
            scope,
            context,
            expected_type
        );

        if (else_result.type.isError()) {
            return {Type::makeError()};
        }

        // 两个分支都发散
        if (then_result.type.isNever() && else_result.type.isNever()) {
            return {Type::makeNever()};
        }

        // 有预期返回值的if，比如外层是赋值 let x = if xxx {1}
        if (expected_type) {

            if (!canAssignment(then_result.type, *expected_type) || !canAssignment(else_result.type, *expected_type)) {
                return {Type::makeError()};
            }

            if (condition.type.isNever()) {
                return {Type::makeNever()};
            }

            return {*expected_type, false, false};
        }

        Type result_type;

        // then发散，使用else的类型
        if (then_result.type.isNever()) {
            result_type = else_result.type;
        }

        // else发散，使用then的类型
        else if (else_result.type.isNever()) {
            result_type = then_result.type;
        }

        // 两个类型相同
        else if (then_result.type == else_result.type) {
            result_type = then_result.type;
        }

        // else可以赋给then
        else if (canAssignment(else_result.type, then_result.type)) {
            result_type = then_result.type;
        }

        // then可以赋给else
        else if (canAssignment(then_result.type, else_result.type)) {
            result_type = else_result.type;
        }

        else {//报错
            return {Type::makeError()};
        }

        if (condition.type.isNever()) {
            return {Type::makeNever()};
        }

        return {result_type, false, false};
    }

    // while 和 loop,loop的返回类型由break决定，while没有
    ExprResult Checker::checkWhile(
        const std::shared_ptr<ast::WhileExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr || !expr->condition || !expr->body) {
            return {Type::makeError()};
        }

        // 检查condition bool或never
        Type bool_type = Type::makeBool();

        ExprResult condition = checkExpr(
            expr->condition,
            scope,
            context,
            &bool_type
        );

        if (condition.type.isError()) {
            return {Type::makeError()};
        }

        if (!condition.type.isNever() && !condition.type.isBool()) {
            return {Type::makeError()};
        }

        // 进入while循环
        LoopContext loop;
        loop.allow_value_break = false;

        context.loops.push_back(loop);

        // 检查body
        Type unit_type = Type::makeUnit();

        ExprResult body = checkBlock(
            expr->body,
            scope,
            context,
            &unit_type
        );

        // 离开while循环
        context.loops.pop_back();

        if (body.type.isError()) {
            return {Type::makeError()};
        }

        // body必须是Unit或者Never
        if (!body.type.isNever() && !body.type.isUnit()) {
            return {Type::makeError()};
        }

        if (condition.type.isNever()) {
            return {Type::makeNever()};
        }

        return {Type::makeUnit(), false, false};
    }
    ExprResult Checker::checkLoop(
        const std::shared_ptr<ast::LoopExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr || !expr->body) {
            return {Type::makeError()};
        }

        // 进入loop循环
        LoopContext loop;
        loop.allow_value_break = true;

        context.loops.push_back(loop);

        // loop的body本身必须是Unit或者Never
        Type unit_type = Type::makeUnit();

        ExprResult body = checkBlock(
            expr->body,
            scope,
            context,
            &unit_type
        );

        // 先保存break类型，再退出当前loop
        std::vector<Type> break_types = context.loops.back().break_types;

        context.loops.pop_back();

        if (body.type.isError()) {
            return {Type::makeError()};
        }

        if (!body.type.isNever() && !body.type.isUnit()) {
            return {Type::makeError()};
        }

        // 没有任何正常break，loop永远不会正常结束
        if (break_types.empty()) {
            return {Type::makeNever()};
        }

        Type result_type = break_types[0];

        // 合并所有break的类型
        for (std::size_t i = 1; i < break_types.size(); ++i) {

            const Type& current = break_types[i];

            // 当前类型可以赋给已有类型
            if (canAssignment(current, result_type)) {
                continue;
            }

            // 已有类型可以赋给当前类型，则使用当前类型
            if (canAssignment(result_type, current)) {
                result_type = current;
                continue;
            }

            return {Type::makeError()};
        }

        return {result_type, false, false};
    }

    ExprResult Checker::checkReturn(
        const std::shared_ptr<ast::ReturnExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr) {
            return {Type::makeError()};
        }

        if (!expr->value) {

            // 只有返回Unit的函数可以直接return
            if (!context.return_type.isUnit()) {
                return {Type::makeError()};
            }

            return {Type::makeNever()};
        }

        // return value;
        ExprResult value = checkExpr(
            expr->value,
            scope,
            context,
            &context.return_type
        );

        if (value.type.isError()) {
            return {Type::makeError()};
        }

        // 返回值必须能够赋给函数声明的返回类型
        if (!canAssignment(value.type, context.return_type)) {
            return {Type::makeError()};
        }

        // return之后不会继续执行
        return {Type::makeNever()};
    }

    ExprResult Checker::checkBreak(
        const std::shared_ptr<ast::BreakExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr) {
            return {Type::makeError()};
        }

        // 不在循环内部不能break
        if (context.loops.empty()) {
            return {Type::makeError()};
        }

        LoopContext& loop = context.loops.back();

        if (expr->value) {

            // while不允许break value
            if (!loop.allow_value_break) {
                return {Type::makeError()};
            }

            ExprResult value = checkExpr(
                expr->value,
                scope,
                context,
                loop.expected_type
            );

            if (value.type.isError()) {
                return {Type::makeError()};
            }

            // break后面的value本身发散
            // 实际上break不会真正发生，所以不记录break类型
            if (value.type.isNever()) {
                return {Type::makeNever()};
            }

            loop.break_types.push_back(value.type);

            return {Type::makeNever()};
        }

        // 单个的break对loop来说相当于break ()
        if (loop.allow_value_break) {
            loop.break_types.push_back(Type::makeUnit());
        }

        // while里的break不需要记录类型，因为while固定为Unit
        return {Type::makeNever()};
    }
    
    ExprResult Checker::checkContinue(
        const std::shared_ptr<ast::ContinueExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr) {
            return {Type::makeError()};
        }

        // 不在循环内部不能continue
        if (context.loops.empty()) {
            return {Type::makeError()};
        }

        // continue之后当前路径用never占位
        return {Type::makeNever()};
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

            if (global != value_map.end() &&
                global->second.kind == ValueSymbolKind::Constant &&
                global->second.constant) {

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

         // 多段path，处理关联常量，例如 A::X 或 Self::X
        const auto& parts = expr->path->segments;

        if (parts.size() != 2 || parts[0]->name.empty() || 
            parts[1]->name.empty() || !parts[1]->generic_arguments.empty()) {
            return {Type::makeError()};
        }

        // 将第一段路径解析成类型
        auto type_path = std::make_shared<ast::Path>();
        type_path->segments.push_back(parts[0]);

        auto syntax = std::make_shared<ast::TypeSyntax>();
        syntax->kind = ast::TypeSyntaxKind::Path;
        syntax->path = type_path;

        Type type = resolveType(
            syntax,
            context.self_type
        );

        // 关联常量必须属于用户定义的struct
        if (!type.isStruct()) {
            return {Type::makeError()};
        }

        // 查找struct对应的关联成员
        auto map = associated_map.find(type.struct_name);

        if (map == associated_map.end()) {
            return {Type::makeError()};
        }

        // 查找对应名称
        auto member = map->second.find(parts[1]->name);

        if (member == map->second.end() || member->second.kind != ValueSymbolKind::Constant ||
            !member->second.constant || !member->second.constant->type) {
            return {Type::makeError()};
        }

        // 获取关联常量的类型
        Type result = resolveType(
            member->second.constant->type,
            type.struct_name
        );

        if (result.isError()) {
            return {Type::makeError()};
        }

        // 常量不是可以赋值的存储位置
        return {result, false, false};
    }

    // 检查函数的普通参数
    bool Checker::checkArguments(
        const std::vector<std::shared_ptr<ast::Expr>>& arguments,
        const std::vector<std::shared_ptr<ast::FunctionParameter>>& parameters,
        std::size_t start,
        const std::string& self_type,
        Scope& scope,
        CheckContext& context,
        bool& fasan
    ) {
        // 检查参数数量
        if (arguments.size() != parameters.size() + start) {
            return false;
        }

        // 逐个检查实参和形参
        for (std::size_t i = 0; i < parameters.size(); ++i) {

            const auto& parameter = parameters[i];

            if (!parameter || !parameter->type || !arguments[start + i]) {
                return false;
            }

            // 获取形参类型
            Type type = resolveType(
                parameter->type,
                self_type
            );

            if (type.isError()) {
                return false;
            }

            // 检查实参
            ExprResult argument = checkExpr(
                arguments[start + i],
                scope,
                context,
                &type
            );

            if (argument.type.isError()) {
                return false;
            }

            // 实参与形参类型相符
            if (!canAssignment(argument.type, type)) {
                return false;
            }

            if (argument.type.isNever()) {
                fasan = true;
            }
        }

        return true;
    }

    // 检查已经确定的函数调用
    ExprResult Checker::checkFunctionCall(
        const std::shared_ptr<ast::FunctionItem>& function,
        const std::vector<std::shared_ptr<ast::Expr>>& arguments,
        std::size_t start,
        const std::string& self_type,
        Scope& scope,
        CheckContext& context,
        bool fasan
    ) {
        if (!function) {
            return {Type::makeError()};
        }

        // 检查参数
        if (!checkArguments(
                arguments,
                function->parameters,
                start,
                self_type,
                scope,
                context,
                fasan)) {
            return {Type::makeError()};
        }

        // 参数中有Never，整个调用也不会返回
        if (fasan) {
            return {Type::makeNever()};
        }

        // 获取函数返回类型
        Type type = function->return_type ? resolveType(function->return_type, self_type) : Type::makeUnit();

        if (type.isError()) {
            return {Type::makeError()};
        }

        return {type, false, false};
    }

    // 包括普通函数，关联函数和builtin函数
    ExprResult Checker::checkCall(
        const std::shared_ptr<ast::CallExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr || !expr->callee) {
            return {Type::makeError()};
        }

        // 获取函数调用的路径
        auto path = std::dynamic_pointer_cast<ast::PathExpr>(
            expr->callee
        );

        if (!path || !path->path || path->path->segments.empty()) {
            return {Type::makeError()};
        }

        //存储路径的vector
        const auto& parts = path->path->segments;

        // 普通函数调用，例如 f()
        if (parts.size() == 1) {

            if (!parts[0] || parts[0]->name.empty()) {
                return {Type::makeError()};
            }

            std::string name = parts[0]->name;

            // 局部变量会覆盖同名函数
            if (scope.find(name)) {
                return {Type::makeError()};
            }

            // get_i32()
            if (name == "get_i32") {

                if (!expr->arguments.empty()) {
                    return {Type::makeError()};
                }

                return {Type::makeI32(), false, false};
            }

            // print_i32()和println_i32()
            if (name == "print_i32" || name == "println_i32") {

                if (expr->arguments.size() != 1) {
                    return {Type::makeError()};
                }

                Type type = Type::makeI32();

                ExprResult argument = checkExpr(
                    expr->arguments[0],
                    scope,
                    context,
                    &type
                );

                if (argument.type.isError() || !canAssignment(argument.type, type)) {
                    return {Type::makeError()};
                }
                if (argument.type.isNever()) {
                    return {Type::makeNever()};
                }

                return {Type::makeUnit(), false, false};
            }

            // 从全局符号表中寻找函数
            auto it = value_map.find(name);

            if (it == value_map.end() || it->second.kind != ValueSymbolKind::Function || !it->second.function) {
                return {Type::makeError()};
            }

            return checkFunctionCall(
                it->second.function,
                expr->arguments,
                0,
                "",
                scope,
                context
            );
        }

        // 关联调用
        if (parts.size() != 2 || !parts[0] || !parts[1]) {
            return {Type::makeError()};
        }

        // 获取关联调用所属的类型
        auto type_path = std::make_shared<ast::Path>();
        type_path->segments.push_back(parts[0]);

        auto syntax = std::make_shared<ast::TypeSyntax>();
        syntax->kind = ast::TypeSyntaxKind::Path;
        syntax->path = type_path;

        Type type = resolveType(
            syntax,
            context.self_type
        );

        if (type.isError()) {
            return {Type::makeError()};
        }

        std::string name = parts[1]->name;

        // Box::<T>::new(value)
        if (type.isBox() && name == "new") {

            if (!type.inner || expr->arguments.size() != 1) {
                return {Type::makeError()};
            }

            ExprResult argument = checkExpr(
                expr->arguments[0],
                scope,
                context,
                type.inner.get()
            );

            if (argument.type.isError() || !canAssignment(argument.type, *type.inner)) {
                return {Type::makeError()};
            }
            if (argument.type.isNever()) {
                return {Type::makeNever()};
            }

            return {type, false, false};
        }

        // Vec::<T>::new()
        if (type.isVec() && name == "new") {

            if (!expr->arguments.empty()) {
                return {Type::makeError()};
            }

            return {type, false, false};
        }

        // 用户定义struct的关联函数
        if (type.isStruct()) {

            auto map = associated_map.find(type.struct_name);

            if (map != associated_map.end()) {
                auto member = map->second.find(name);

                if (member != map->second.end()) {

                    if (member->second.kind != ValueSymbolKind::Function || !member->second.function) {
                        return {Type::makeError()};
                    }

                    auto function = member->second.function;

                    // 没有self，直接检查普通参数
                    if (!function->receiver) {
                        return checkFunctionCall(
                            function,
                            expr->arguments,
                            0,
                            type.struct_name,
                            scope,
                            context
                        );
                    }

                    // 有self时，第一个参数传入self
                    if (expr->arguments.empty()) {
                        return {Type::makeError()};
                    }

                    Type self = Type::makeStruct(
                        type.struct_name
                    );

                    if (function->receiver->kind == ast::ReceiverKind::SharedReference) {

                        self = Type::makeReference(
                            self,
                            false
                        );
                    }

                    else if (function->receiver->kind == ast::ReceiverKind::MutableReference) {

                        self = Type::makeReference(
                            self,
                            true
                        );
                    }

                    ExprResult argument = checkExpr(
                        expr->arguments[0],
                        scope,
                        context,
                        &self
                    );

                    if (argument.type.isError() || !canAssignment(argument.type, self)) {
                        return {Type::makeError()};
                    }

                    // self检查完以后，普通参数从1开始（第0个是self）
                    return checkFunctionCall(
                        function,
                        expr->arguments,
                        1,
                        type.struct_name,
                        scope,
                        context,
                        argument.type.isNever()
                    );
                }
            }
        }

        // 自动生成的Clone
        if (name == "clone" && hasClone(type)) {

            if (expr->arguments.size() != 1) {
                return {Type::makeError()};
            }

            Type ref = Type::makeReference(
                type,
                false
            );

            ExprResult argument = checkExpr(
                expr->arguments[0],
                scope,
                context,
                &ref
            );

            if (argument.type.isError() ||
                !canAssignment(argument.type, ref)) {
                return {Type::makeError()};
            }

            if (argument.type.isNever()) {
                return {Type::makeNever()};
            }

            return {type, false, false};
        }

        return {Type::makeError()};
    }

    ExprResult Checker::checkMethodCall(
        const std::shared_ptr<ast::MethodCallExpr>& expr,
        Scope& scope,
        CheckContext& context
    ) {
        if (!expr || !expr->receiver || !expr->method || expr->method->name.empty()) {
            return {Type::makeError()};
        }

        // 先检查receiver
        ExprResult receiver = checkExpr(
            expr->receiver,
            scope,
            context
        );

        if (receiver.type.isError()) {
            return {Type::makeError()};
        }

        std::string name = expr->method->name;

        Type current_type = receiver.type;

        // 判断当前receiver能不能提供可变访问
        bool mutable_access;

        if (receiver.is_assignable) {
            mutable_access = receiver.is_mutable;
        } else {
            mutable_access = true;
        }

        bool crossed_shared = receiver.mutable_access_blocked;

        if (crossed_shared) {
            mutable_access = false;
        }

        // 不断自动解引用Reference和Box寻找方法
        while (true) {

            // 每一层按照 T、&T、&mut T 的顺序查找
            std::vector<Type> candidates;

            candidates.push_back(current_type);
            candidates.push_back(
                Type::makeReference(
                    current_type,
                    false
                )
            );
            candidates.push_back(
                Type::makeReference(
                    current_type,
                    true
                )
            );

            for (std::size_t i = 0; i < candidates.size(); ++i) {

                const Type& candidate = candidates[i];

                // 查找是否有定义
                std::string self_type;

                if (candidate.isStruct()) {
                    self_type = candidate.struct_name;
                }

                else if (candidate.isReference() && candidate.inner &&
                        candidate.inner->isStruct()) {

                    self_type = candidate.inner->struct_name;
                }

                if (!self_type.empty()) {

                    auto map = associated_map.find(self_type);
                    if (map != associated_map.end()) {

                        auto member = map->second.find(name);

                        if (member != map->second.end() && member->second.kind == ValueSymbolKind::Function &&
                            member->second.function) {

                            auto function = member->second.function;

                            // 没有self的关联函数不能使用点调用
                            if (function->receiver) {

                                // 判断需要什么样的self
                                Type self = Type::makeStruct(
                                    self_type
                                );
                                switch (function->receiver->kind) {

                                    case ast::ReceiverKind::Value:
                                    case ast::ReceiverKind::MutableValue:
                                        break;

                                    case ast::ReceiverKind::SharedReference:
                                        self = Type::makeReference(
                                            self,
                                            false
                                        );
                                        break;

                                    case ast::ReceiverKind::MutableReference:
                                        self = Type::makeReference(
                                            self,
                                            true
                                        );
                                        break;
                                }

                                // 当前候选类型就是这个方法需要的self
                                if (self == candidate) {

                                    // 自动生成&mut T时，原位置必须可以修改
                                    if (i == 2 && !mutable_access) {
                                        return {Type::makeError()};
                                    }

                                    // 已有&mut不能再套&
                                    if (i == 0 && candidate.isReference() &&
                                        candidate.mutable_reference && crossed_shared) {

                                        return {Type::makeError()};
                                    }

                                    // 检查普通参数
                                    return checkFunctionCall(
                                        function,
                                        expr->arguments,
                                        0,
                                        self_type,
                                        scope,
                                        context
                                    );
                                }
                            }
                        }
                    }
                }

                // clone
                if (name == "clone" &&
                    candidate.isReference() &&
                    !candidate.mutable_reference &&
                    candidate.inner &&
                    hasClone(*candidate.inner)) {

                    if (!expr->arguments.empty()) {
                        return {Type::makeError()};
                    }

                    return {
                        *candidate.inner,
                        false,
                        false
                    };
                }

                // 数组的len()
                if (name == "len" &&
                    candidate.isReference() &&
                    !candidate.mutable_reference &&
                    candidate.inner &&
                    candidate.inner->isArray()) {

                    if (!expr->arguments.empty()) {
                        return {Type::makeError()};
                    }

                    return {
                        Type::makeUSize(),
                        false,
                        false
                    };
                }

                // Vec的四种方法
                if (candidate.isReference() &&
                    candidate.inner &&
                    candidate.inner->isVec()) {

                    Type vec = *candidate.inner;

                    if (!vec.inner) {
                        return {Type::makeError()};
                    }

                    // len(&self)
                    if (name == "len" &&
                        !candidate.mutable_reference) {

                        if (!expr->arguments.empty()) {
                            return {Type::makeError()};
                        }

                        return {
                            Type::makeUSize(),
                            false,
                            false
                        };
                    }

                    // is_empty(&self)
                    if (name == "is_empty" &&
                        !candidate.mutable_reference) {

                        if (!expr->arguments.empty()) {
                            return {Type::makeError()};
                        }

                        return {
                            Type::makeBool(),
                            false,
                            false
                        };
                    }

                    // push(&mut self, value: T)
                    if (name == "push" &&
                        candidate.mutable_reference) {

                        // 自动借用成&mut Vec时必须可以修改
                        if (i == 2 && !mutable_access) {
                            return {Type::makeError()};
                        }

                        // &mut外层不能有&
                        if (i == 0 && crossed_shared) {
                            return {Type::makeError()};
                        }

                        if (expr->arguments.size() != 1) {
                            return {Type::makeError()};
                        }

                        Type type = *vec.inner;

                        ExprResult argument = checkExpr(
                            expr->arguments[0],
                            scope,
                            context,
                            &type
                        );

                        if (argument.type.isError() || !canAssignment(argument.type, type)) {
                            return {Type::makeError()};
                        }

                        if (argument.type.isNever()) {
                            return {Type::makeNever()};
                        }

                        return {
                            Type::makeUnit(),
                            false,
                            false
                        };
                    }

                    // remove(&mut self, index: usize)
                    if (name == "remove" && candidate.mutable_reference) {

                        if (i == 2 && !mutable_access) {
                            return {Type::makeError()};
                        }

                        if (i == 0 && crossed_shared) {
                            return {Type::makeError()};
                        }

                        if (expr->arguments.size() != 1) {
                            return {Type::makeError()};
                        }

                        Type usize_type = Type::makeUSize();

                        ExprResult argument = checkExpr(
                            expr->arguments[0],
                            scope,
                            context,
                            &usize_type
                        );

                        if (argument.type.isError() || !canAssignment(argument.type, usize_type)) {
                            return {Type::makeError()};
                        }

                        if (argument.type.isNever()) {
                            return {Type::makeNever()};
                        }

                        return {
                            *vec.inner,
                            false,
                            false
                        };
                    }
                }
            }

            // 当前这一层找不到，自动解引用一层继续找
            if (!dereferenceForAccess(
                    current_type,
                    mutable_access,
                    crossed_shared)) {
                break;
            }
        }

        // 所有候选类型都找不到方法
        return {Type::makeError()};
    }

} // namespace rx::semantic