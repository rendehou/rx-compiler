#include "rx/ir/printer.hpp"

#include <stdexcept>

namespace rx::ir {

    namespace {

        // 统一输出 LLVM 符号，避免名字中包含特殊字符导致语法错误
        void printSymbol(std::ostream& out, char prefix, const std::string& name) {
            if (name.empty()) {
                throw std::runtime_error("Empty LLVM symbol name");
            }

            const char* hex = "0123456789ABCDEF";
            out << prefix << '"';
            for (unsigned char c : name) {
                if (c >= 32 && c <= 126 && c != '"' && c != '\\') {
                    out << static_cast<char>(c);
                } else {
                    out << '\\' << hex[c >> 4] << hex[c & 15];
                }
            }
            out << '"';
        }

        const char* binaryName(IROpcode opcode) {
            switch (opcode) {
                case IROpcode::Add:  return "add";
                case IROpcode::Sub:  return "sub";
                case IROpcode::Mul:  return "mul";
                case IROpcode::SDiv: return "sdiv";
                case IROpcode::UDiv: return "udiv";
                case IROpcode::SRem: return "srem";
                case IROpcode::URem: return "urem";
                case IROpcode::And:  return "and";
                case IROpcode::Or:   return "or";
                case IROpcode::Xor:  return "xor";
                case IROpcode::Shl:  return "shl";
                case IROpcode::LShr: return "lshr";
                case IROpcode::AShr: return "ashr";
                default: throw std::runtime_error("Invalid binary opcode");
            }
        }

        const char* castName(IROpcode opcode) {
            switch (opcode) {
                case IROpcode::Trunc:    return "trunc";
                case IROpcode::ZExt:     return "zext";
                case IROpcode::SExt:     return "sext";
                case IROpcode::BitCast:  return "bitcast";
                case IROpcode::PtrToInt: return "ptrtoint";
                case IROpcode::IntToPtr: return "inttoptr";
                default: throw std::runtime_error("Invalid cast opcode");
            }
        }

        const char* predicateName(IRPredicate predicate) {
            switch (predicate) {
                case IRPredicate::EQ:  return "eq";
                case IRPredicate::NE:  return "ne";
                case IRPredicate::SLT: return "slt";
                case IRPredicate::SLE: return "sle";
                case IRPredicate::SGT: return "sgt";
                case IRPredicate::SGE: return "sge";
                case IRPredicate::ULT: return "ult";
                case IRPredicate::ULE: return "ule";
                case IRPredicate::UGT: return "ugt";
                case IRPredicate::UGE: return "uge";
            }
            throw std::runtime_error("Invalid ICmp predicate");
        }

        void checkOperands(const IRInstruction& inst, std::size_t count) {
            if (inst.operands.size() != count) {
                throw std::runtime_error("Invalid instruction operand count");
            }
        }

        // extractvalue / insertvalue 约定：末尾的 operands 是常量字段索引
        void printAggregateIndices(std::ostream& out,
                                   const std::vector<IRValue>& operands,
                                   std::size_t start) {
            if (start >= operands.size()) {
                throw std::runtime_error("Missing aggregate index");
            }
            for (std::size_t i = start; i < operands.size(); ++i) {
                if (operands[i].kind != IRValueKind::IntegerConstant ||
                    operands[i].integer < 0) {
                    throw std::runtime_error("Aggregate index must be a nonnegative constant");
                }
                out << ", " << operands[i].integer;
            }
        }

    } // namespace

    IRPrinter::IRPrinter(const IRModule& module) : module_(module) {}

    void IRPrinter::printType(std::ostream& out, const IRType& type) const {
        switch (type.kind) {
            case IRTypeKind::Void:
                out << "void";
                break;
            case IRTypeKind::Integer:
                if (type.bits == 0) {
                    throw std::runtime_error("Invalid integer type");
                }
                out << 'i' << type.bits;
                break;
            case IRTypeKind::Pointer:
                out << "ptr";
                break;
            case IRTypeKind::Array:
                if (!type.element) {
                    throw std::runtime_error("Missing array element type");
                }
                out << '[' << type.length << " x ";
                printType(out, *type.element);
                out << ']';
                break;
            case IRTypeKind::NamedStruct:
                printSymbol(out, '%', type.name);
                break;
            case IRTypeKind::LiteralStruct:
                out << "{ ";
                for (std::size_t i = 0; i < type.fields.size(); ++i) {
                    if (i != 0) out << ", ";
                    printType(out, type.fields[i]);
                }
                out << " }";
                break;
        }
    }

    void IRPrinter::printValue(std::ostream& out, const IRValue& value) const {
        switch (value.kind) {
            case IRValueKind::Temporary:
            case IRValueKind::Parameter:
                // 使用带字母的名字，避免 LLVM 纯数字临时值的编号约束
                out << "%v" << value.id;
                break;
            case IRValueKind::IntegerConstant:
                out << value.integer;
                break;
            case IRValueKind::Global:
                printSymbol(out, '@', value.name);
                break;
            case IRValueKind::Null:
                out << "null";
                break;
            case IRValueKind::Undef:
                out << "undef";
                break;
            case IRValueKind::ZeroInitializer:
                out << "zeroinitializer";
                break;
            case IRValueKind::None:
                throw std::runtime_error("Cannot print an empty IRValue");
        }
    }

    void IRPrinter::printTypedValue(std::ostream& out, const IRValue& value) const {
        printType(out, value.type);
        out << ' ';
        printValue(out, value);
    }

    void IRPrinter::printInstruction(std::ostream& out, const IRInstruction& inst) const {
        if (inst.has_result) {
            printValue(out, inst.result);
            out << " = ";
        }

        switch (inst.opcode) {
            // 二元整数运算
            case IROpcode::Add: case IROpcode::Sub: case IROpcode::Mul:
            case IROpcode::SDiv: case IROpcode::UDiv:
            case IROpcode::SRem: case IROpcode::URem:
            case IROpcode::And: case IROpcode::Or: case IROpcode::Xor:
            case IROpcode::Shl: case IROpcode::LShr: case IROpcode::AShr:
                checkOperands(inst, 2);
                out << binaryName(inst.opcode) << ' ';
                printTypedValue(out, inst.operands[0]);
                out << ", ";
                printValue(out, inst.operands[1]);
                break;

            // 整数比较
            case IROpcode::ICmp:
                checkOperands(inst, 2);
                out << "icmp " << predicateName(inst.predicate) << ' ';
                printTypedValue(out, inst.operands[0]);
                out << ", ";
                printValue(out, inst.operands[1]);
                break;

            // 类型转换
            case IROpcode::Trunc: case IROpcode::ZExt: case IROpcode::SExt:
            case IROpcode::BitCast: case IROpcode::PtrToInt: case IROpcode::IntToPtr:
                checkOperands(inst, 1);
                out << castName(inst.opcode) << ' ';
                printTypedValue(out, inst.operands[0]);
                out << " to ";
                printType(out, inst.result.type);
                break;

            // 内存指令
            case IROpcode::Alloca:
                checkOperands(inst, 0);
                out << "alloca ";
                printType(out, inst.source_type);
                if (inst.alignment != 0) out << ", align " << inst.alignment;
                break;

            case IROpcode::Load:
                checkOperands(inst, 1);
                out << "load ";
                printType(out, inst.result.type);
                out << ", ";
                printTypedValue(out, inst.operands[0]);
                if (inst.alignment != 0) out << ", align " << inst.alignment;
                break;

            case IROpcode::Store:
                checkOperands(inst, 2);
                out << "store ";
                printTypedValue(out, inst.operands[0]);
                out << ", ";
                printTypedValue(out, inst.operands[1]);
                if (inst.alignment != 0) out << ", align " << inst.alignment;
                break;

            case IROpcode::GetElementPtr:
                if (inst.operands.size() < 2) {
                    throw std::runtime_error("GEP requires an address and indices");
                }
                out << "getelementptr ";
                if (inst.inbounds) out << "inbounds ";
                printType(out, inst.source_type);
                for (const IRValue& operand : inst.operands) {
                    out << ", ";
                    printTypedValue(out, operand);
                }
                break;

            // 聚合值操作：operands = {aggregate, index...} / {aggregate, value, index...}
            case IROpcode::ExtractValue:
                out << "extractvalue ";
                if (inst.operands.empty()) throw std::runtime_error("Missing aggregate");
                printTypedValue(out, inst.operands[0]);
                printAggregateIndices(out, inst.operands, 1);
                break;

            case IROpcode::InsertValue:
                if (inst.operands.size() < 3) throw std::runtime_error("Missing insertvalue operands");
                out << "insertvalue ";
                printTypedValue(out, inst.operands[0]);
                out << ", ";
                printTypedValue(out, inst.operands[1]);
                printAggregateIndices(out, inst.operands, 2);
                break;

            // 函数调用：source_type 保存返回类型，void 调用也能输出
            case IROpcode::Call:
                out << "call ";
                printType(out, inst.source_type);
                out << ' ';
                printValue(out, inst.callee);
                out << '(';
                for (std::size_t i = 0; i < inst.operands.size(); ++i) {
                    if (i != 0) out << ", ";
                    printTypedValue(out, inst.operands[i]);
                }
                out << ')';
                break;

            case IROpcode::Phi:
                out << "phi ";
                printType(out, inst.result.type);
                out << ' ';
                for (std::size_t i = 0; i < inst.incoming.size(); ++i) {
                    if (i != 0) out << ", ";
                    out << "[ ";
                    printValue(out, inst.incoming[i].value);
                    out << ", %bb" << inst.incoming[i].block << " ]";
                }
                break;

            case IROpcode::Select:
                checkOperands(inst, 3);
                out << "select ";
                printTypedValue(out, inst.operands[0]);
                out << ", ";
                printTypedValue(out, inst.operands[1]);
                out << ", ";
                printTypedValue(out, inst.operands[2]);
                break;

            // 控制流指令
            case IROpcode::Br:
                if (inst.targets.size() != 1) throw std::runtime_error("Invalid br targets");
                out << "br label %bb" << inst.targets[0];
                break;

            case IROpcode::CondBr:
                checkOperands(inst, 1);
                if (inst.targets.size() != 2) throw std::runtime_error("Invalid condbr targets");
                out << "br ";
                printTypedValue(out, inst.operands[0]);
                out << ", label %bb" << inst.targets[0]
                    << ", label %bb" << inst.targets[1];
                break;

            case IROpcode::Ret:
                if (inst.operands.empty()) {
                    out << "ret void";
                } else {
                    checkOperands(inst, 1);
                    out << "ret ";
                    printTypedValue(out, inst.operands[0]);
                }
                break;

            case IROpcode::Unreachable:
                out << "unreachable";
                break;
        }
    }

    void IRPrinter::printTypeDef(std::ostream& out, const IRTypeDef& def) const {
        printSymbol(out, '%', def.name);
        out << " = type { ";
        for (std::size_t i = 0; i < def.fields.size(); ++i) {
            if (i != 0) out << ", ";
            printType(out, def.fields[i]);
        }
        out << " }\n";
    }

    void IRPrinter::printGlobal(std::ostream& out, const IRGlobal& global) const {
        printSymbol(out, '@', global.name);
        out << " = " << (global.is_constant ? "constant " : "global ");
        printType(out, global.type);
        out << ' ';
        if (global.initializer.kind == IRValueKind::None) {
            out << "zeroinitializer";
        } else {
            printValue(out, global.initializer);
        }
        out << '\n';
    }

    void IRPrinter::printFunction(std::ostream& out, const IRFunction& function) const {
        out << (function.is_declaration ? "declare " : "define ");
        printType(out, function.return_type);
        out << ' ';
        printSymbol(out, '@', function.name);
        out << '(';
        for (std::size_t i = 0; i < function.parameters.size(); ++i) {
            if (i != 0) out << ", ";
            if (function.is_declaration) {
                printType(out, function.parameters[i].type);
            } else {
                printTypedValue(out, function.parameters[i]);
            }
        }
        out << ')';

        if (function.is_declaration) {
            out << "\n";
            return;
        }

        out << " {\n";
        for (const IRBasicBlock& block : function.blocks) {
            // 不使用 block.name 作 LLVM 标签，保证与 targets(BlockId) 一一对应
            out << "bb" << block.id << ":\n";
            for (const IRInstruction& inst : block.instructions) {
                out << "  ";
                printInstruction(out, inst);
                out << '\n';
            }
        }
        out << "}\n";
    }

    void IRPrinter::print(std::ostream& out) const {
        // 与当前项目固定的 RV32IM/ILP32 目标保持一致
        out << "target datalayout = \"e-m:e-p:32:32-i64:64-n32-S128\"\n";
        out << "target triple = \"riscv32-unknown-unknown-elf\"\n\n";

        for (const IRTypeDef& def : module_.type_defs) {
            printTypeDef(out, def);
        }
        if (!module_.type_defs.empty()) out << '\n';

        for (const IRGlobal& global : module_.globals) {
            printGlobal(out, global);
        }
        if (!module_.globals.empty()) out << '\n';

        for (const IRFunction& function : module_.functions) {
            printFunction(out, function);
            out << '\n';
        }
    }

} // namespace rx::ir
