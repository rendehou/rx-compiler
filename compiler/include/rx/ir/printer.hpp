#pragma once

#include "rx/ir/ir.hpp"

#include <ostream>

namespace rx::ir {

    class IRPrinter {
    public:
        explicit IRPrinter(const IRModule& module);

        // 将当前 IRModule 输出为 LLVM IR 文本
        void print(std::ostream& out) const;

    private:
        const IRModule& module_;

        // 类型、值、指令的输出
        void printType(std::ostream& out, const IRType& type) const;
        void printValue(std::ostream& out, const IRValue& value) const;
        void printTypedValue(std::ostream& out, const IRValue& value) const;
        void printInstruction(std::ostream& out, const IRInstruction& inst) const;

        // Module 的各个组成部分
        void printTypeDef(std::ostream& out, const IRTypeDef& def) const;
        void printGlobal(std::ostream& out, const IRGlobal& global) const;
        void printFunction(std::ostream& out, const IRFunction& function) const;
    };

} // namespace rx::ir
