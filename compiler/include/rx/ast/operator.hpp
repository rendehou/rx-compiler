#pragma once

namespace rx::ast {

// 以下枚举是表达式节点中“运算符字段”的取值，不是 AST 节点类；类型合法性由语义检查决定。
enum class UnaryOperator {
    Negate,          // 数值取负：-x。
    Not,             // 逻辑非：!x。
    Dereference,     // 解引用：*x。
    BorrowShared,    // 不可变借用：&x。
    BorrowMutable,   // 可变借用：&mut x。
};

// BinaryExpr::op 的取值：记录二元表达式中左右操作数之间的运算。
enum class BinaryOperator {
    Add, Subtract, Multiply, Divide, Remainder,
    ShiftLeft, ShiftRight,
    BitAnd, BitOr, BitXor,
    LogicalAnd, LogicalOr,
    Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual,
};

// AssignmentExpr::op 的取值：记录赋值表达式执行普通赋值还是复合赋值。
enum class AssignmentOperator {
    Assign,
    Add, Subtract, Multiply, Divide, Remainder,
    BitAnd, BitOr, BitXor, ShiftLeft, ShiftRight,
};

}  // namespace rx::ast
