#pragma once

#include "rx/ast/program.hpp"
#include "rx/semantic/symbol.hpp"
#include <unordered_map>
#include <memory>

namespace rx::semantic {
    extern std::unordered_map<std::string,std::shared_ptr<rx::ast::StructItem>> struct_map;
    extern std::unordered_map<std::string, ValueSymbol> value_map;
    extern std::unordered_map<std::string,std::unordered_map<std::string,ValueSymbol>> associated_map;
    bool collectItems(
        const std::shared_ptr<rx::ast::Program>& program
    );
}