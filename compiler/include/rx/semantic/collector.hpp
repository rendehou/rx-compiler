#pragma once

#include "rx/ast/program.hpp"
#include "rx/semantic/symbol.hpp"
#include <unordered_map>
#include <memory>

namespace rx::semantic {
    std::unordered_map<std::string,std::shared_ptr<rx::ast::StructItem>> struct_map;
    std::unordered_map<std::string, ValueSymbol> value_map;
    bool collectItems(
        const std::shared_ptr<rx::ast::Program>& program
    );
}