#include "ir/builder.hpp"
#include "ir/generator.hpp"
#include <stdexcept>

namespace rx::ir{

    IRValue IRBuilder::createBinary(
        IROpcode opcode,
        const IRValue& left,
        const IRValue& right
    ) {
        // 检查操作数
        if (left.type.kind != IRTypeKind::Integer ||
            right.type.kind != IRTypeKind::Integer ||
            left.type.bits != right.type.bits ||
            left.type.bits == 0) {
            throw std::runtime_error("Invalid binary operand types");
        }
        // 创建指令
        IRInstruction inst;
        inst.opcode = opcode;
        // 读入操作数
        inst.operands = {left, right};

        // 结果是ssa
        IRValue result = makeTemporary(left.type);
        inst.has_result = true;
        inst.result = result;

        appendInstruction(inst);

        return result;
    }

    IRValue IRBuilder::createICmp(
        IRPredicate predicate,
        const IRValue& left,
        const IRValue& right
    ) {
        //检查操作数
        if (left.type.kind != IRTypeKind::Integer ||
            right.type.kind != IRTypeKind::Integer ||
            left.type.bits != right.type.bits ||
            left.type.bits == 0) {
            throw std::runtime_error("Invalid ICmp operand types");
        }

        IRInstruction inst;
        inst.opcode = IROpcode::ICmp;

        inst.operands = {left, right};
        inst.predicate = predicate;

        // 注意比较结果是bool，也就是i1
        IRType type;
        type.kind = IRTypeKind::Integer;
        type.bits = 1;

        IRValue result = makeTemporary(type);
        inst.has_result = true;
        inst.result = result;

        appendInstruction(inst);

        return result;
    }
    
    IRValue IRBuilder::createCast(//类型转换
        IROpcode opcode,
        const IRValue& value,
        const IRType& target_type
    ) {
        switch (opcode) {

            case IROpcode::Trunc://保留低位，需要原始位数多余修改后位数
                if (value.type.kind != IRTypeKind::Integer ||
                    target_type.kind != IRTypeKind::Integer ||
                    value.type.bits <= target_type.bits) {
                    throw std::runtime_error("Invalid trunc types");
                }
                break;

            case IROpcode::ZExt:
            case IROpcode::SExt://这两个指令是向高位补的，所以要少于
                if (value.type.kind != IRTypeKind::Integer ||
                    target_type.kind != IRTypeKind::Integer ||
                    value.type.bits >= target_type.bits) {
                    throw std::runtime_error("Invalid trunc types");
                }
                break;

            case IROpcode::BitCast:
                //要求位宽必须一致
                if (value.type.kind == IRTypeKind::Integer &&
                    target_type.kind == IRTypeKind::Integer &&
                    value.type.bits == target_type.bits) {
                    break;
                }
                if (value.type.kind == IRTypeKind::Pointer &&
                    target_type.kind == IRTypeKind::Pointer) {
                    break;
                }
                throw std::runtime_error("Invalid BitCast types");

            case IROpcode::PtrToInt:
                // 检查源类型必须为指针
                if (value.type.kind != IRTypeKind::Pointer ||
                    target_type.kind != IRTypeKind::Integer){
                    throw std::runtime_error("Invalid ptrtoint types");
                }
                break;

            case IROpcode::IntToPtr:
                // 检查源类型必须为整数
                if (value.type.kind != IRTypeKind::Integer ||
                    target_type.kind != IRTypeKind::Pointer) {
                    throw std::runtime_error("Invalid inttoptr types");
                }
                break;

            default:
                throw std::runtime_error("Invalid cast opcode");
        }

        IRInstruction inst;
        inst.opcode = opcode;

        inst.operands = {value};

        IRValue result = makeTemporary(target_type);
        inst.has_result = true;
        inst.result = result;

        appendInstruction(inst);

        return result;
    }

    IRValue IRBuilder::createAlloca(const IRType& type) { //分配一个i32内存并返回指针

        if (type.kind == IRTypeKind::Void ||
        (type.kind == IRTypeKind::Integer && type.bits == 0)) {
            throw std::runtime_error("Invalid alloca type");
        }

        IRInstruction inst;
        inst.opcode = IROpcode::Alloca;

        inst.source_type = type;

        IRType ptype;
        ptype.kind = IRTypeKind::Pointer;

        IRValue result = makeTemporary(ptype);//注意返回指针
        inst.has_result = true;
        inst.result = result;

        appendInstruction(inst);

        return result;
    }

    IRValue IRBuilder::createLoad(//读取adress中的地址
        const IRType& type,
        const IRValue& address
    ) {
        IRInstruction inst;
        inst.opcode = IROpcode::Load;

        //传入oprand
        inst.operands = {address};

        IRValue result = makeTemporary(type);
        inst.has_result = true;
        inst.result = result;

        appendInstruction(inst);

        return result;
    }

    void IRBuilder::createStore(
        const IRValue& value,
        const IRValue& address
    ) {
        
        if (address.type.kind != IRTypeKind::Pointer) {
            throw std::runtime_error("Store address must be a pointer");
        }

        IRInstruction inst;
        inst.opcode = IROpcode::Store;

        inst.operands = {value, address};

        inst.has_result = false;

        appendInstruction(inst);
    }

    // 辅助函数
    IRValue IRBuilder::makeTemporary(const IRType& type) {

        IRValue result;

        result.kind = IRValueKind::Temporary;
        result.type = type;
        result.id = next_value_++;

        return result;
    }
    void IRBuilder::appendInstruction(const IRInstruction& inst) {

        //检查函数和块是否存在
        if (function_ >= module_.functions.size()) {
            throw std::runtime_error("Invalid function");
        }
        IRFunction& function = module_.functions[function_];
        if (current_block_ >= function.blocks.size()) {
            throw std::runtime_error("Invalid basic block");
        }

        IRBasicBlock& block = function.blocks[current_block_];

        if (isTerminated()) {// 块是否结束了
            throw std::runtime_error("Basic block already terminated");
        }

        block.instructions.push_back(inst);
    }
}