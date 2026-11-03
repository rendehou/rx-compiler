#include "rx/semantic/collector.hpp"
#include "rx/semantic/type.hpp"

//collector负责存变量，函数，结构体名字并判断重复
namespace rx::semantic{

    //
    
    std::unordered_map<std::string,std::shared_ptr<rx::ast::StructItem>> struct_map;
    std::unordered_map<std::string, ValueSymbol> value_map;
    std::unordered_map< std::string, std::unordered_map<std::string, ValueSymbol>> associated_map;

    bool collectItems (
        const std::shared_ptr<rx::ast::Program> &program
    ) {
        value_map.clear();
        struct_map.clear();

        for(const auto& item : program->items) {

            if (auto function = std::dynamic_pointer_cast<rx::ast::FunctionItem>(item)) {//function

                if (value_map.find(function->name) != value_map.end()) {
                    return false;//判定失败，名字重了
                }

                // 存入函数
                ValueSymbol symbol;
                symbol.kind = ValueSymbolKind::Function;
                symbol.function = function;
                value_map[function->name] = symbol;

                continue;
            }

            if (auto const_value = std::dynamic_pointer_cast<rx::ast::ConstantItem>(item)) {//function

                if (value_map.find(const_value->name) != value_map.end()) {
                    return false;//判定失败，名字重了
                }

                // 存入函数
                ValueSymbol symbol;
                symbol.kind = ValueSymbolKind::Constant;
                symbol.constant = const_value;
                value_map[const_value->name] = symbol;

                continue;
            }

            if (auto structure = std::dynamic_pointer_cast<rx::ast::StructItem>(item)) {//function

                if (struct_map.find(structure->name) != struct_map.end()) {
                    return false;//判定失败，名字重了
                }

                // 存入函数
                struct_map[structure->name] = structure;

                continue;
            }
        }   

        
        // 第二次扫描收集impl中的关联函数和常量
        for (const auto& item : program->items) {

            auto implementation = std::dynamic_pointer_cast<rx::ast::ImplItem>(item);
            if (!implementation) {
                continue;
            }
            if (!implementation->target_type) {
                return false;
            }

            // 检查impl目标类型
            if (implementation->target_type->kind == rx::ast::TypeSyntaxKind::Path) {

                if (!implementation->target_type->path ||
                    implementation->target_type->path->segments.empty()) {
                    return false;
                }

                for (const auto& segment : implementation->target_type->path->segments) {

                    if (!segment) {
                        return false;
                    }
                }
            }

            Type target_type = resolveType(
                implementation->target_type
            );

            // impl只能作用于用户定义的结构体
            if (!target_type.isStruct()) {
                return false;
            }

            if (struct_map.find(target_type.struct_name) == struct_map.end()) {
                return false;
            }

            // 获取当前结构体的关联成员
            auto& members = associated_map[target_type.struct_name];

            for (const auto& associated_item : implementation->items) {

                if (!associated_item) {
                    return false;
                }

                // 关联函数
                if (auto function = std::get_if<
                    std::shared_ptr<rx::ast::FunctionItem>
                >(&associated_item->value)) {

                    if (!*function || (*function)->name.empty()) {
                        return false;
                    }

                    // 检查关联名称是否重复
                    if (members.find((*function)->name) != members.end()) {
                        return false;
                    }

                    ValueSymbol symbol;
                    symbol.kind = ValueSymbolKind::Function;
                    symbol.function = *function;

                    members[(*function)->name] = symbol;
                }

                // 关联常量
                else if (auto constant = std::get_if<
                    std::shared_ptr<rx::ast::ConstantItem>
                >(&associated_item->value)) {

                    if (!*constant || (*constant)->name.empty()) {
                        return false;
                    }

                    // 函数和常量共享关联命名空间
                    if (members.find((*constant)->name) != members.end()) {
                        return false;
                    }

                    ValueSymbol symbol;
                    symbol.kind = ValueSymbolKind::Constant;
                    symbol.constant = *constant;

                    members[(*constant)->name] = symbol;
                }

                else {
                    return false;
                }
            }
        }


        return true;
    }
}
