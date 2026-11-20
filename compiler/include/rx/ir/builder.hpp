#pragma once

#include "rx/ir/ir.hpp"

namespace rx::ir {

    class IRBuilder {//builder负责生成llvm ir代码
    public:
        IRBuilder(IRModule& module, FunctionId function);

        // 基本块管理
        BlockId createBlock(const std::string& name);
        void setInsertPoint(BlockId block);
        bool isTerminated() const;

        // 计算
        IRValue createBinary(//二元运算
            IROpcode opcode,
            const IRValue& left,
            const IRValue& right
        );

        IRValue createICmp(//比较
            IRPredicate predicate,
            const IRValue& left,
            const IRValue& right
        );

        IRValue createCast(//类型转换
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

        IRValue createGEP(//gep指令
            const IRType& source_type,
            const IRValue& address,//起始地址
            const std::vector<IRValue>& indices//gep的所有索引
        );

        // 函数调用
        IRValue createCall(
            const IRValue& callee,//被调用函数名
            const IRType& return_type,//返回类型
            const std::vector<IRValue>& arguments//参数列表
        );

        // 控制流
        void createBr(BlockId target);//无条件跳转

        void createCondBr(//条件跳转
            const IRValue& condition,//条件
            BlockId true_block,//满足条件的跳转位置
            BlockId false_block
        );

        IRValue createPhi(
            const IRType& type,
            const std::vector<IRPhiIncoming>& incoming//不同phi分支
        );

        void createRet();
        void createRet(const IRValue& value);
        void createUnreachable();

    private:
        IRModule& module_;//当前程序
        FunctionId function_;//当前函数

        BlockId current_block_ = 0;//当前基本块
        ValueId next_value_ = 0;//下一个产生ssa结果的编号

        IRValue makeTemporary(const IRType& type);// 分配ssa结果编号
        void appendInstruction(const IRInstruction& inst);// 将操作存储
    };

}