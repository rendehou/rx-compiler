#pragma once

namespace rx::ast {

// Syntax only. SemanticChecker decides which operand/result types each op allows.
enum class UnaryOperator {
    Negate,          // -x
    Not,             // !x
    Dereference,     // *x
    BorrowShared,    // &x
    BorrowMutable,   // &mut x
};

enum class BinaryOperator {
    Add, Subtract, Multiply, Divide, Remainder,
    ShiftLeft, ShiftRight,
    BitAnd, BitOr, BitXor,
    LogicalAnd, LogicalOr,
    Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual,
};

enum class AssignmentOperator {
    Assign,
    Add, Subtract, Multiply, Divide, Remainder,
    BitAnd, BitOr, BitXor, ShiftLeft, ShiftRight,
};

}  // namespace rx::ast
