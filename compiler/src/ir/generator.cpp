#include "ir/generator.hpp"
#include <stdexcept>

namespace rx::ir{

    IRType IRGenerator::lowerType(
        const semantic::Type& type
    ) {
        IRType result;

        switch (type.kind) {

            case semantic::TypeKind::I32:
            case semantic::TypeKind::U32:
            case semantic::TypeKind::ISize:
            case semantic::TypeKind::USize:
                result.kind = IRTypeKind::Integer;
                result.bits = 32;
                break;

            case semantic::TypeKind::Bool:
                result.kind = IRTypeKind::Integer;
                result.bits = 1;
                break;

            case semantic::TypeKind::Unit:
            case semantic::TypeKind::Never:
                result.kind = IRTypeKind::Void;
                break;

            case semantic::TypeKind::Struct:
                result.kind = IRTypeKind::NamedStruct;
                result.name = type.struct_name;
                break;

            case semantic::TypeKind::Reference:
            case semantic::TypeKind::Box:
            case semantic::TypeKind::Vec:
                result.kind = IRTypeKind::Pointer;
                break;

            case semantic::TypeKind::Array:
                if (!type.inner) {
                    throw std::runtime_error("Invalid array type");
                }

                result.kind = IRTypeKind::Array;
                result.element = std::make_shared<IRType>(
                    lowerType(*type.inner)
                );
                result.length = type.array_length;
                break;

            case semantic::TypeKind::Error:
                throw std::runtime_error("Cannot lower error type");
        }

        return result;
    }

    void IRGenerator::collectDeclarations(
        const ast::Program& program
    ) {
        // 清空上一轮生成的数据,ir程序可能多轮
        functions_.clear();
        module_.type_defs.clear();
        module_.functions.clear();
        module_.globals.clear();

        // 收集结构体类型定义
        for (const auto& item : program.items) {

            auto structure =
                std::dynamic_pointer_cast<ast::StructItem>(item);

            if (!structure) {
                continue;
            }

            IRTypeDef type_def;
            type_def.name = structure->name;

            // 收集结构体字段类型
            for (const auto& field : structure->fields) {

                semantic::Type field_type =
                    semantic::resolveType(
                        field->type,
                        structure->name
                    );

                if (field_type.isError()) {
                    throw std::runtime_error(
                        "Invalid struct field type"
                    );
                }

                type_def.fields.push_back(
                    lowerType(field_type)
                );
            }

            module_.type_defs.push_back(type_def);
        }

        // 收集普通函数签名
        for (const auto& item : program.items) {

            auto function =
                std::dynamic_pointer_cast<ast::FunctionItem>(item);

            if (!function) {
                continue;
            }

            IRFunction ir_function;

            // 保存函数名称
            ir_function.name = function->name;

            // 解析返回类型
            semantic::Type return_type;

            if (function->return_type) {
                return_type = semantic::resolveType(
                    function->return_type
                );
            }
            else {
                return_type = semantic::Type::makeUnit();
            }

            if (return_type.isError()) {
                throw std::runtime_error(
                    "Invalid function return type"
                );
            }

            ir_function.return_type = lowerType(return_type);

            // 收集参数
            for (std::size_t i = 0;
                i < function->parameters.size();
                ++i) {

                const auto& parameter = function->parameters[i];

                if (!parameter || !parameter->type) {
                    throw std::runtime_error(
                        "Invalid function parameter"
                    );
                }

                semantic::Type parameter_type =
                    semantic::resolveType(parameter->type);

                if (parameter_type.isError()) {
                    throw std::runtime_error(
                        "Invalid parameter type"
                    );
                }

                IRValue ir_parameter;

                ir_parameter.kind = IRValueKind::Parameter;
                ir_parameter.type = lowerType(parameter_type);
                ir_parameter.id = i;

                ir_function.parameters.push_back(ir_parameter);
            }

            // 分配 FunctionId
            FunctionId id = module_.functions.size();

            // 记录 AST 函数和 IR 函数的对应关系
            functions_[function->node_id] = id;

            // 保存函数
            module_.functions.push_back(ir_function);
        }
    }

    IRResult IRGenerator::generateInteger(
        const ast::IntegerExpr& expr,
        FunctionContext& context
    ) {
        // 获取 Semantic 已经确定的整数类型
        auto type_it = info_.expr_types.find(expr.node_id);

        if (type_it == info_.expr_types.end() ||
            !type_it->second.isInteger()) {
            throw std::runtime_error(
                "Missing integer expression type"
            );
        }

        // 转换成 IR 类型
        IRType type = lowerType(type_it->second);

        // 去掉整数中的下划线和类型后缀
        std::string spelling = expr.spelling;

        const std::string suffixes[] = {// 注意不一定是i32
            "usize", "isize", "i32", "u32"
        };

        for (const std::string& suffix : suffixes) {
            if (spelling.size() >= suffix.size() &&
                spelling.compare(spelling.size() - suffix.size(), suffix.size(), suffix ) == 0) {

                spelling.erase( spelling.size() - suffix.size());
                break;
            }
        }

        std::string digits;

        for (char c : spelling) {
            if (c != '_') {
                digits.push_back(c);
            }
        }

        // 根据前缀判断进制
        int base = 10;

        if (digits.size() > 2 && digits[0] == '0') {
            if (digits[1] == 'x' || digits[1] == 'X') {
                base = 16;
                digits.erase(0, 2);
            }
            else if (digits[1] == 'b' || digits[1] == 'B') {
                base = 2;
                digits.erase(0, 2);
            }
            else if (digits[1] == 'o' || digits[1] == 'O') {
                base = 8;
                digits.erase(0, 2);
            }
        }

        // 解析整数数值
        std::size_t position = 0;
        unsigned long long number = 0;

        try {
            number = std::stoull(digits, &position, base);
        }
        catch (...) {
            throw std::runtime_error(
                "Invalid integer literal"
            );
        }

        if (digits.empty() || position != digits.size() ||
            number > 0xFFFFFFFFULL) {
            throw std::runtime_error(
                "Integer literal out of range"
            );
        }

        // 构造整数常量
        IRValue value;
        value.kind = IRValueKind::IntegerConstant;
        value.type = type;
        value.integer = static_cast<std::int64_t>(number);

        // 返回表达式结果
        IRResult result;
        result.has_value = true;
        result.value = value;

        return result;
    }


    IRResult IRGenerator::generateBoolean(
        const ast::BooleanExpr& expr,
        FunctionContext& context
    ) {
        // bool 在 LLVM IR 中是 i1
        IRType type;
        type.kind = IRTypeKind::Integer;
        type.bits = 1;

        // 创建布尔常量
        IRValue value;
        value.kind = IRValueKind::IntegerConstant;
        value.type = type;
        value.integer = expr.value ? 1 : 0;

        IRResult result;
        result.has_value = true;
        result.value = value;

        return result;
    }


    IRResult IRGenerator::generatePath(
        const ast::PathExpr& expr,
        FunctionContext& context
    ) {
        // 根据表达式 NodeId 找到 BindingId
        auto binding_it = info_.bindings.find(expr.node_id);

        if (binding_it == info_.bindings.end()) {
            throw std::runtime_error(
                "Path is not a local variable"
            );
        }

        semantic::BindingId binding_id = binding_it->second;
        // 根据 BindingId 找到变量的存储位置
        auto local_it = context.locals.find(binding_id);

        if (local_it == context.locals.end()) {
            throw std::runtime_error(
                "Local variable has no IR storage"
            );
        }

        // 读取变量的值
        return emitRead(local_it->second, context);
    }


    IRResult IRGenerator::generateLet(
        const ast::LetStatement& statement,
        FunctionContext& context
    ) {
        // 查找变量的 BindingId
        auto binding_it = info_.bindings.find(
            statement.node_id
        );

        if (binding_it == info_.bindings.end()) {
            throw std::runtime_error(
                "Missing let binding"
            );
        }

        semantic::BindingId binding_id = binding_it->second;

        // 获取变量的语义类型
        auto type_it = info_.binding_types.find(binding_id);

        if (type_it == info_.binding_types.end()) {
            throw std::runtime_error(
                "Missing variable type"
            );
        }

        // 转换为 IR 类型
        IRType type = lowerType(type_it->second);

        if (type.kind == IRTypeKind::Void) {
            throw std::runtime_error(
                "Void variable storage is not supported"
            );
        }

        // 如果有初始化表达式，先生成它
        IRResult initializer;

        if (statement.initializer) {
            initializer = generateExpr(
                *statement.initializer,
                context
            );

            // return、break 等导致无法继续执行
            if (!initializer.falls_through) {
                return initializer;
            }

            if (!initializer.has_value) {
                throw std::runtime_error(
                    "Initializer has no value"
                );
            }
        }

        // 为变量创建存储空间
        IRValue address = context.builder.createAlloca(type);

        IRPlace place;
        place.address = address;
        place.type = type;

        // 记录变量的存储位置
        if (context.locals.find(binding_id) !=
            context.locals.end()) {
            throw std::runtime_error(
                "Duplicate variable binding"
            );
        }

        context.locals[binding_id] = place;

        // 将初始化值写入存储位置
        if (statement.initializer) {
            emitWrite(
                place,
                initializer.value,
                context
            );
        }

        // let 语句自身不产生值
        IRResult result;
        return result;
    }

}