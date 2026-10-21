#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "rx/ast/item.hpp"

namespace rx::semantic {

enum class ValueSymbolKind {
    Function,
    Constant
};

struct ValueSymbol {
    ValueSymbolKind kind = ValueSymbolKind::Function;
    std::shared_ptr<ast::FunctionItem> function;
    std::shared_ptr<ast::ConstantItem> constant;
};

} // namespace rx::semantic