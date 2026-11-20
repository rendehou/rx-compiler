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

    IRValue IRBuilder::createGEP(
        const IRType& source_type,//原元素类型
        const IRValue& address,
        const std::vector<IRValue>& indices//代表索引条目
    ) {
        //是一个代表位置的指针，格式是[原元素类型]，数组起始地址，跳过几个完整数组，数组中的下标等
        if (address.type.kind != IRTypeKind::Pointer) {
            throw std::runtime_error("Invalid GEP address");
        }

        if (source_type.kind == IRTypeKind::Void || indices.empty()) {
            throw std::runtime_error("Invalid GEP source type or indices");
        }

        for (const IRValue& index : indices) {//index需要是integer
            if (index.type.kind != IRTypeKind::Integer) {
                throw std::runtime_error("Invalid GEP index");
            }
        }

        IRInstruction inst;
        inst.opcode = IROpcode::GetElementPtr;
        inst.source_type = source_type;

        //塞入首地址和所有索引条目
        inst.operands.push_back(address);
        for (const IRValue& index : indices) {
            inst.operands.push_back(index);
        }

        IRType type;
        type.kind = IRTypeKind::Pointer;

        IRValue result = makeTemporary(type);
        inst.has_result = true;
        inst.result = result;

        appendInstruction(inst);

        return result;
    }

    IRValue IRBuilder::createCall(
        const IRValue& callee,
        const IRType& return_type,
        const std::vector<IRValue>& arguments
    ) {
        if (callee.kind != IRValueKind::Global &&
            callee.type.kind != IRTypeKind::Pointer) {
            throw std::runtime_error("Invalid callee");
        }

        IRInstruction inst;
        inst.opcode = IROpcode::Call;
        inst.callee = callee;
        inst.operands = arguments;
        inst.source_type = return_type;

        if (return_type.kind == IRTypeKind::Void) {//特别的是void无返回内容
            inst.has_result = false;
            appendInstruction(inst);
            return IRValue();
        }

        IRValue result = makeTemporary(return_type);
        inst.has_result = true;
        inst.result = result;

        appendInstruction(inst);

        return result;
    }

    void IRBuilder::createBr(BlockId target) {

        IRInstruction inst;
        inst.opcode = IROpcode::Br;
        inst.targets = {target};
        inst.has_result = false;

        appendInstruction(inst);
    }

    void IRBuilder::createCondBr(
        const IRValue& condition,
        BlockId true_block,
        BlockId false_block
    ) {
        if (condition.type.kind != IRTypeKind::Integer || condition.type.bits != 1) {
            throw std::runtime_error("Branch condition must be i1");
        }

        IRInstruction inst;
        inst.opcode = IROpcode::CondBr;
        inst.operands = {condition};
        inst.targets = {true_block, false_block};
        inst.has_result = false;

        appendInstruction(inst);
    }

    IRValue IRBuilder::createPhi(
        const IRType& type,
        const std::vector<IRPhiIncoming>& incoming
    ) {
        if (function_ >= module_.functions.size()) {
            throw std::runtime_error("Invalid function");
        }
        IRFunction& function = module_.functions[function_];
        if (current_block_ >= function.blocks.size()) {
            throw std::runtime_error("Invalid basic block");
        }

        if (isTerminated()) {
            throw std::runtime_error("Basic block already terminated");
        }

        if (type.kind == IRTypeKind::Void || incoming.empty()) {
            throw std::runtime_error("Invalid phi type or incoming");
        }

        for (const IRPhiIncoming& item : incoming) {//检查phi的各路结果
            if (item.block >= function.blocks.size()) {
                throw std::runtime_error("Invalid phi incoming block");
            }

            if (item.value.type.kind != type.kind ||
                (type.kind == IRTypeKind::Integer &&
                item.value.type.bits != type.bits)) {
                throw std::runtime_error("Phi type mismatch");
            }
        }

        IRInstruction inst;
        inst.opcode = IROpcode::Phi;
        inst.incoming = incoming;

        IRValue result = makeTemporary(type);
        inst.has_result = true;
        inst.result = result;

        IRBasicBlock& block = function.blocks[current_block_];

        std::size_t pos = 0;
        while (pos < block.instructions.size() && block.instructions[pos].opcode == IROpcode::Phi) {
            ++pos;
        }

        block.instructions.insert(
            block.instructions.begin() + pos,
            inst
        );

        return result;
    }

    void IRBuilder::createRet() {
        
        if (function_ >= module_.functions.size()) {
            throw std::runtime_error("Invalid function");
        }

        if (module_.functions[function_].return_type.kind != IRTypeKind::Void) {
            throw std::runtime_error("Function must return a value");
        }

        IRInstruction inst;
        inst.opcode = IROpcode::Ret;
        inst.has_result = false;

        appendInstruction(inst);
    }


    void IRBuilder::createRet(const IRValue& value) {
        if (function_ >= module_.functions.size()) {
            throw std::runtime_error("Invalid function");
        }

        const IRType& return_type = module_.functions[function_].return_type;

        if (return_type.kind == IRTypeKind::Void ||
            return_type.kind != value.type.kind ||
            (return_type.kind == IRTypeKind::Integer &&
            return_type.bits != value.type.bits) ||
            (return_type.kind == IRTypeKind::NamedStruct &&
            return_type.name != value.type.name)) {
            throw std::runtime_error("Return type mismatch");
        }

        IRInstruction inst;
        inst.opcode = IROpcode::Ret;
        inst.operands = {value};
        inst.has_result = false;

        appendInstruction(inst);
    }


    void IRBuilder::createUnreachable() {
        IRInstruction inst;
        inst.opcode = IROpcode::Unreachable;
        inst.has_result = false;

        appendInstruction(inst);
    }

    // 辅助函数
    IRValue IRBuilder::makeTemporary(const IRType& type) {//登记SSA的编号

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