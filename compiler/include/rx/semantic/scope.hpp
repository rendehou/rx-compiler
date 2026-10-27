#pragma once

#include "rx/semantic/type.hpp"

#include <string>
#include <unordered_map>

namespace rx::semantic {

    struct LocalSymbol {
        Type type;
        bool mutable_binding = false;//是否可移动
    };

    class Scope {//作用域类，通过链表套unordered map模拟不同作用域
    public:
        explicit Scope(Scope* parent = nullptr);

        void define(//定义一个变量
            const std::string& name,
            const LocalSymbol& symbol
        );

        LocalSymbol* find(//在所有定义域找
            const std::string& name
        );

        const LocalSymbol* find(
            const std::string& name
        ) const;

        LocalSymbol* findCurrent(//在当前定义域找
            const std::string& name
        );

    private:
        Scope* parent_ = nullptr;

        std::unordered_map<std::string, LocalSymbol> variables_;
    };

}