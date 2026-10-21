#include "rx/ast/item.hpp"
#include "rx/ast/program.hpp"
#include "rx/semantic/symbol.hpp"
#include <unordered_map>
#include <memory>

namespace rx::semantic{
    //value存值和函数，struct存结构体
    std::unordered_map<std::string,std::shared_ptr<rx::ast::StructItem>> struct_map;
    std::unordered_map<std::string,ValueSymbol> value_map;

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
        return true;
    }
}