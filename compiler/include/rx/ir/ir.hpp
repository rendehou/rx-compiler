#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace rx::ir {

    using ValueId = std::size_t;
    using BlockId = std::size_t;
    using FunctionId = std::size_t;

    // LLVM IR 类型
    enum class IRTypeKind {
        Void, Integer, Pointer, Array, NamedStruct, LiteralStruct
    };

    struct IRType { // 潜在的兼容其他类
        IRTypeKind kind = IRTypeKind::Void;
        unsigned bits = 0;//整数位数

        std::shared_ptr<IRType> element;
        std::size_t length = 0;

        std::string name;//namedstruct,标准的定义
        std::vector<IRType> fields;//literalstruct，例如%a = alloca{i32，i32}，通过实际实例反补全定义
    };

    struct IRTypeDef { //struct
        std::string name;
        std::vector<IRType> fields;//用vector保存结构体各个字段的类型，按顺序。这里使用vector是因为顺序不同内存布局不同，paddling也是这个原因
    };

    // LLVM IR 操作数
    enum class IRValueKind {//temporary是ssa值
        None, Temporary, Parameter, IntegerConstant,
        Global, Null, Undef, ZeroInitializer
    };

    struct IRValue {
        IRValueKind kind = IRValueKind::None;
        IRType type;

        ValueId id = 0;//编译器内部编号
        std::int64_t integer = 0;
        std::string name;//调用的时候需要的名称
    };

    // LLVM IR 指令类别
    enum class IROpcode {
        Add, Sub, Mul, SDiv, UDiv, SRem, URem,
        And, Or, Xor, Shl, LShr, AShr,

        ICmp,
        Trunc, ZExt, SExt, BitCast, PtrToInt, IntToPtr,

        Alloca, Load, Store, GetElementPtr,
        ExtractValue, InsertValue,

        Call, Phi, Select,

        Br, CondBr, Ret, Unreachable
    };

    enum class IRPredicate {
        EQ, NE,
        SLT, SLE, SGT, SGE,
        ULT, ULE, UGT, UGE
    };

    struct IRPhiIncoming {//phi语句
        IRValue value;
        BlockId block = 0;
    };

    // 一条结构化 LLVM IR 指令
    struct IRInstruction {
        IROpcode opcode = IROpcode::Add;//执行什么指令操作

        //是否产生ssa结果
        bool has_result = false;
        IRValue result;

        std::vector<IRValue> operands;//指令使用的输入值

        // 不同指令需要的附加信息
        IRType source_type;// gep和alloca需要的地址类型
        IRPredicate predicate = IRPredicate::EQ;//比较，按照什么顺序比较
        std::vector<BlockId> targets;//依次跳转的分支
        std::vector<IRPhiIncoming> incoming;//分支合并，每个IRphiIncoming表示一个分支的一组信息

        IRValue callee;//
        std::size_t alignment = 0;//内存对齐
        bool inbounds = false;//
    };

    struct IRBasicBlock {
        BlockId id = 0;
        std::string name;
        std::vector<IRInstruction> instructions;
    };

    struct IRFunction {
        std::string name;
        IRType return_type;
        std::vector<IRValue> parameters;
        std::vector<IRBasicBlock> blocks;
        bool is_declaration = false;
    };

    struct IRGlobal { // 全局变量
        std::string name;
        IRType type;
        IRValue initializer;
        bool is_constant = false;
    };

    struct IRModule {
        std::string target_triple;
        std::string data_layout;

        std::vector<IRTypeDef> type_defs;
        std::vector<IRGlobal> globals;
        std::vector<IRFunction> functions;
    };

}