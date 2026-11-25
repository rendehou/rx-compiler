
#pragma once

#include "rx/semantic/info.hpp"

#include <string>
#include <unordered_map>

namespace rx::semantic {

    struct LocalSymbol {
        Type type;
        bool mutable_binding = false;
        BindingId binding_id = 0;
    };

    class Scope {
    public:
        explicit Scope(Scope* parent = nullptr);

        void define(
            const std::string& name,
            const LocalSymbol& symbol
        );

        LocalSymbol* find(
            const std::string& name
        );

        const LocalSymbol* find(
            const std::string& name
        ) const;

        LocalSymbol* findCurrent(
            const std::string& name
        );

    private:
        Scope* parent_ = nullptr;

        std::unordered_map<std::string, LocalSymbol> variables_;
    };

}
