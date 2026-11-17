#pragma once

#include "rx/ir/ir.hpp"

namespace rx::ir {

    class IRBuilder {
    public:
        IRBuilder(IRModule& module, FunctionId function);

        // 基本块管理
        BlockId createBlock(const std::string& name);
        void setInsertPoint(BlockId block);
        bool isTerminated() const;

        // 计算
        IRValue createBinary(
            IROpcode opcode,
            const IRValue& left,
            const IRValue& right
        );

        IRValue createICmp(
            IRPredicate predicate,
            const IRValue& left,
            const IRValue& right
        );

        IRValue createCast(
            IROpcode opcode,
            const IRValue& value,
            const IRType& target_type
        );

        // 内存
        IRValue createAlloca(const IRType& type);
        IRValue createLoad(
            const IRType& type,
            const IRValue& address
        );

        void createStore(
            const IRValue& value,
            const IRValue& address
        );

        IRValue createGEP(
            const IRType& source_type,
            const IRValue& address,
            const std::vector<IRValue>& indices
        );

        // 函数调用
        IRValue createCall(
            const IRValue& callee,
            const IRType& return_type,
            const std::vector<IRValue>& arguments
        );

        // 控制流
        void createBr(BlockId target);

        void createCondBr(
            const IRValue& condition,
            BlockId true_block,
            BlockId false_block
        );

        IRValue createPhi(
            const IRType& type,
            const std::vector<IRPhiIncoming>& incoming
        );

        void createRet();
        void createRet(const IRValue& value);
        void createUnreachable();

    private:
        IRModule& module_;
        FunctionId function_;

        BlockId current_block_ = 0;
        ValueId next_value_ = 0;
    };

}