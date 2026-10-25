#include "rx/semantic/scope.hpp"

namespace rx::semantic {

    Scope::Scope(Scope* parent) : parent_(parent) {}

    void Scope::define(//定义一个变量
        const std::string& name,
        const LocalSymbol& symbol
    ) {
        variables_[name] = symbol;
    }

    LocalSymbol* Scope::find(//在所有定义域找
        const std::string& name
    ) {
        auto symbol = findCurrent(name);

        if (symbol) {
            return symbol;
        }
        if (parent_) {//向前面的作用域找
            return parent_->find(name);
        }

        return nullptr;
    }

    const LocalSymbol* Scope::find(
        const std::string& name
    ) const {
        auto it = variables_.find(name);

        if (it != variables_.end()) {
            return &it->second;
        }
        if (parent_) {
            return parent_->find(name);
        }

        return nullptr;
    }

    LocalSymbol* Scope::findCurrent(//在当前定义域找
        const std::string& name
    ) {
        auto it = variables_.find(name);

        if (it == variables_.end()) {
            return nullptr;
        }

        return &it->second;
    }
}