#include "rx/semantic/declaration.hpp"
#include "rx/semantic/type.hpp"
#include "rx/ast/item.hpp"
#include "rx/semantic/collector.hpp"
#include <unordered_map>
#include <unordered_set>

namespace rx::semantic {
    
    // 结构体布局检查状态
    enum class structState {
        Unvisited,
        Evaluating,
        Evaluated
    };

    // 提前声明，供 checkTypeLayout 递归调用
    bool checkStructState(
        const std::string& name,
        std::unordered_map<std::string, structState>& states
    );

    // 检查结构体字段类型
    bool checkTypeLayout(
        const Type& type,
        std::unordered_map<std::string, structState>& states
    ) {
        switch (type.kind) {

            case TypeKind::Struct:// 重点检查，递归检查字段
                return checkStructState(type.struct_name, states);

            case TypeKind::Array:// 继续检查数组元素类型
                if (!type.inner) return false;
                return checkTypeLayout(*type.inner, states);

            case TypeKind::Reference:
            case TypeKind::Box:
            case TypeKind::Vec:// 间接存储，可以停止递归，返回没问题
                return true;

            case TypeKind::Error:
                return false;

            default:
                return true;
        }
    }

    // DFS检查结构体是否有无限递归的定义
    bool checkStructState(
        const std::string& name,
        std::unordered_map<std::string, structState>& states
    ) {
        auto state_it = states.find(name);

        if (state_it != states.end()) {

            if (state_it->second == structState::Evaluating) {
                // 当前递归路径中再次遇到这个结构体，形成环
                return false;
            }

            if (state_it->second == structState::Evaluated) {
                return true;
            }
        }

        // 根据结构体名称找到它的AST节点
        auto struct_it = struct_map.find(name);

        if (struct_it == struct_map.end() || !struct_it->second) {
            return false;
        }

        // 开始检查之前标记正在访问
        states[name] = structState::Evaluating;

        // 逐个检查结构体的字段类型
        for (const auto& field : struct_it->second->fields) {

            if (!field || !field->type) {
                states[name] = structState::Unvisited;
                return false;
            }

            Type type = resolveType(field->type);

            if (type.isError() || !checkTypeLayout(type, states)) {
                states[name] = structState::Unvisited;
                return false;
            }
        }

        states[name] = structState::Evaluated;

        return true;
    }

    bool checkFunctionDeclaration(//检查参数名是否重复、参数类型是否合法、返回类型是否合法
        const std::shared_ptr<rx::ast::FunctionItem>& function
    ) {
        std::unordered_set<std::string> parameter_names;
    
        for (const auto& parameter : function->parameters) {
            if (parameter_names.find(parameter->name) != parameter_names.end()) {
                return false; // 判定失败，参数名字重了
            }

            parameter_names.insert(parameter->name);

            Type type = resolveType(parameter->type);
            if (type.isError()) return false; // 参数类型不合法
        }

        // 检查返回类型；没有显式返回类型时默认为 ()
        if (function->return_type) {
            Type type = resolveType(function->return_type);
            if (type.isError()) return false;
        }

        return true;
    }

    bool checkStructDeclaration( 
        const std::shared_ptr<rx::ast::StructItem>& structure
    ) {//struct字段名是否重复、字段类型是否合法
        
        std::unordered_set<std::string> field_names;
        for (const auto& field : structure->fields) {
            if (field_names.find(field->name) != field_names.end()) {
                return false; // 判定失败，字段名字重复
            }

            field_names.insert(field->name);

            Type type = resolveType(field->type);
            if (type.isError()) return false; //类型不合法
        }
        
        return true;
    }

    bool checkConstantDeclaration(
        const std::shared_ptr<rx::ast::ConstantItem>& constant
    ) {
        Type type = resolveType(constant->type);

        if (type.isError()) return false; //常量声明类型不合法

        return true;
    }

    bool checkDeclarations(
        const std::shared_ptr<rx::ast::Program>& program
    ) {//常量声明是否合法

        if (!program) return false;
        for (const auto& item : program->items) {
            if (auto function = std::dynamic_pointer_cast<rx::ast::FunctionItem>(item)) {
                if (!checkFunctionDeclaration(function)) return false;
            }
            else if (auto structure = std::dynamic_pointer_cast<rx::ast::StructItem>(item)) {
                if (!checkStructDeclaration(structure)) return false;
            }
            else if (auto constant = std::dynamic_pointer_cast<rx::ast::ConstantItem>(item)) {
                if (!checkConstantDeclaration(constant)) return false;
            }
        }

        // 检查所有结构体是否存在无限递归布局
        std::unordered_map<std::string, structState> states;
        for (const auto& item : program->items) {

            auto structure = std::dynamic_pointer_cast<rx::ast::StructItem>(item);

            if (!structure) continue;

            if (!checkStructState(structure->name, states)) {
                return false;
            }
        }

        return true;
    }

}
