#include "rx/semantic/declaration.hpp"
#include "rx/semantic/type.hpp"
#include "rx/ast/item.hpp"

#include <unordered_set>

namespace rx::semantic {

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

        return true;
    }

}
