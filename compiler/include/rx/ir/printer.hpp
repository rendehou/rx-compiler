#pragma once

#include <ostream>
#include "rx/ir/ir.hpp"

namespace rx::ir {

    class IRPrinter {
    public:
        void print(
            const IRModule& module,
            std::ostream& out
        ) const;

    private:
        void printType(
            const IRType& type,
            std::ostream& out
        ) const;

        void printValue(
            const IRValue& value,
            std::ostream& out
        ) const;

        void printInstruction(
            const IRInstruction& instruction,
            std::ostream& out
        ) const;

        void printFunction(
            const IRFunction& function,
            std::ostream& out
        ) const;
    };

}