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

}