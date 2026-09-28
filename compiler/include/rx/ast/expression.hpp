#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rx/ast/operator.hpp"
#include "rx/ast/statement.hpp"
#include "rx/ast/type_syntax.hpp"

namespace rx::ast {

// 表达式类别的抽象多态基类，继承 AstNode；多种具体表达式都由 Expr 派生。
// Parser.g4 中的优先级包装规则最终折叠为这些具体子类，而不是一层规则一个类。
struct Expr : AstNode {
    virtual ~Expr() = default;
    [[nodiscard]] AstChildren children() const override = 0;
protected:
    explicit Expr(NodeKind kind) : AstNode(kind) {}
};

// 整数表达式节点，继承 Expr；对应 literalExpression 中的 INTEGER_LITERAL。
// spelling 保留源码写法，避免在 AST 阶段丢失进制、下划线等信息。
struct IntegerExpr final : Expr {
    IntegerExpr();
    [[nodiscard]] AstChildren children() const override;
    std::string spelling;                          // 整数字面量原文，例如 "0xff" 或 "1_000"。
};

// 布尔表达式节点，继承 Expr；对应 literalExpression 中的 true/false。
// value 为 true 或 false，不再保存关键字文本。
struct BooleanExpr final : Expr {
    BooleanExpr();
    [[nodiscard]] AstChildren children() const override;
    bool value = false;                            // 布尔字面量的实际值。
};

// 单元值表达式节点，继承 Expr；对应空括号 ()（或空括号表达式）。
// 它没有额外字段，因为语法结构本身不携带其他值。
struct UnitExpr final : Expr {
    UnitExpr();
    [[nodiscard]] AstChildren children() const override;
};

// 路径表达式节点，继承 Expr；对应 pathInExpression，例如 x 或 module::value。
// path 保存分段名称及每段的泛型实参。
struct PathExpr final : Expr {
    PathExpr();
    [[nodiscard]] AstChildren children() const override;
    Path path;                                     // 被读取/引用的路径语法子节点。
};

// 块表达式节点，继承 Expr；对应 expressionWithBlock 中直接出现的 blockExpression。
// block 保存花括号里的语句和可选尾表达式。
struct BlockExpr final : Expr {
    BlockExpr();
    [[nodiscard]] AstChildren children() const override;
    Block block;                                   // 该表达式的代码块子节点。
};

// if 表达式节点，继承 Expr；对应 ifExpression。
// 条件、then 块和可选 else 分支分别保存为子节点。
struct IfExpr final : Expr {
    IfExpr();
    [[nodiscard]] AstChildren children() const override;
    std::unique_ptr<Expr> condition;               // if 后的条件表达式子树。
    Block then_block;                              // 条件成立时执行的代码块。
    std::unique_ptr<Expr> else_branch;              // 可选 else 块或 else if 子树；没有 else 时为空。
};

// while 循环表达式节点，继承 Expr；对应 WHILE conditionExpression blockExpression。
// condition 是循环条件，body 是每轮执行的代码块。
struct WhileExpr final : Expr {
    WhileExpr();
    [[nodiscard]] AstChildren children() const override;
    std::unique_ptr<Expr> condition;               // while 条件表达式子树。
    Block body;                                    // 循环体代码块。
};

// 无条件 loop 循环节点，继承 Expr；对应 LOOP blockExpression。
// body 保存循环体；退出条件由其中的 break 等表达式体现。
struct LoopExpr final : Expr {
    LoopExpr();
    [[nodiscard]] AstChildren children() const override;
    Block body;                                    // loop 循环体代码块。
};

// break 表达式节点，继承 Expr；对应 BREAK expression?。
// value 保存可选的 break 返回值，例如 break 7；单独 break 时为空。
struct BreakExpr final : Expr {
    BreakExpr();
    [[nodiscard]] AstChildren children() const override;
    std::unique_ptr<Expr> value;                   // 可选的退出值表达式子树。
};

// continue 表达式节点，继承 Expr；对应 CONTINUE，没有额外语法值。
struct ContinueExpr final : Expr {
    ContinueExpr();
    [[nodiscard]] AstChildren children() const override;
};

// return 表达式节点，继承 Expr；对应 RETURN expression?。
// value 保存可选返回值；裸 return 时为空。
struct ReturnExpr final : Expr {
    ReturnExpr();
    [[nodiscard]] AstChildren children() const override;
    std::unique_ptr<Expr> value;                   // 可选的返回值表达式子树。
};

// 一元运算表达式节点，继承 Expr；对应 unaryOperator unaryExpression。
// op 是运算符种类，operand 是唯一的操作数子树。
struct UnaryExpr final : Expr {
    UnaryExpr();
    [[nodiscard]] AstChildren children() const override;
    UnaryOperator op = UnaryOperator::Not;         // 负号、非、解引用或借用等一元运算符。
    std::unique_ptr<Expr> operand;                  // 运算符作用的表达式子树。
};

// 二元运算表达式节点，继承 Expr；由各级二元运算语法规则折叠得到。
// op 表示运算符，left/right 是运算符左右两侧子树。
struct BinaryExpr final : Expr {
    BinaryExpr();
    [[nodiscard]] AstChildren children() const override;
    BinaryOperator op = BinaryOperator::Add;       // 加减、比较、逻辑、位运算等二元运算符。
    std::unique_ptr<Expr> left;                     // 左操作数子树。
    std::unique_ptr<Expr> right;                    // 右操作数子树。
};

// 赋值表达式节点，继承 Expr；对应 assignmentExpression 及其赋值运算符。
// place 是被赋值一侧，value 是右侧表达式；op 保留普通或复合赋值种类。
struct AssignmentExpr final : Expr {
    AssignmentExpr();
    [[nodiscard]] AstChildren children() const override;
    AssignmentOperator op = AssignmentOperator::Assign; // =、+=、<<= 等赋值运算符。
    std::unique_ptr<Expr> place;                         // 左侧目标表达式子树。
    std::unique_ptr<Expr> value;                         // 右侧值表达式子树。
};

// 类型转换表达式节点，继承 Expr；对应 expression AS typeRef。
// value 是待转换表达式，target_type 是 as 后的目标类型语法。
struct CastExpr final : Expr {
    CastExpr();
    [[nodiscard]] AstChildren children() const override;
    std::unique_ptr<Expr> value;                   // as 左侧表达式子树。
    TypeSyntax target_type;                        // as 右侧目标类型。
};

// 普通函数调用节点，继承 Expr；对应 postfixExpression 的 callArguments 后缀。
// callee 是被调用表达式，arguments 保存实参，顺序与源码一致。
struct CallExpr final : Expr {
    CallExpr();
    [[nodiscard]] AstChildren children() const override;
    std::unique_ptr<Expr> callee;                   // 函数名/可调用表达式子树。
    std::vector<std::unique_ptr<Expr>> arguments;   // 圆括号内的实参子树列表。
};

// 方法调用节点，继承 Expr；对应 dotSuffix 中“接收者.方法名(实参)”形式。
// receiver 是点号左边的对象，method 保存方法路径段，arguments 保存实参。
struct MethodCallExpr final : Expr {
    MethodCallExpr();
    [[nodiscard]] AstChildren children() const override;
    std::unique_ptr<Expr> receiver;                 // 点号左侧的接收者表达式子树。
    PathSegment method;                             // 点号右侧的方法名及可选泛型实参。
    std::vector<std::unique_ptr<Expr>> arguments;   // 方法调用的实参列表。
};

// 字段访问节点，继承 Expr；对应 dotSuffix 中“表达式.字段名”形式。
// base 是点号左侧表达式，field 是点号右侧字段名。
struct FieldAccessExpr final : Expr {
    FieldAccessExpr();
    [[nodiscard]] AstChildren children() const override;
    std::unique_ptr<Expr> base;                     // 被访问字段所属的对象表达式子树。
    std::string field;                              // 点号后的字段名。
};

// 下标访问节点，继承 Expr；对应 postfixSuffix 中“表达式[下标]”形式。
// base 是被索引对象，index 是方括号中的下标表达式。
struct IndexExpr final : Expr {
    IndexExpr();
    [[nodiscard]] AstChildren children() const override;
    std::unique_ptr<Expr> base;                     // 数组或其他可下标对象的表达式子树。
    std::unique_ptr<Expr> index;                    // 方括号中的下标表达式子树。
};

// 数组列表节点，继承 Expr；对应 arrayExpression 中逗号分隔的元素形式。
// elements 保存每个元素的表达式子树，按源码顺序排列。
struct ArrayListExpr final : Expr {
    ArrayListExpr();
    [[nodiscard]] AstChildren children() const override;
    std::vector<std::unique_ptr<Expr>> elements;    // 方括号中的数组元素列表。
};

// 重复元素数组节点，继承 Expr；对应 arrayExpression 的 [expression; constValue] 形式。
// element 是重复的元素表达式，count 是分号后的常量长度语法。
struct ArrayRepeatExpr final : Expr {
    ArrayRepeatExpr();
    [[nodiscard]] AstChildren children() const override;
    std::unique_ptr<Expr> element;                  // 要重复放入数组的元素表达式。
    ConstValueSyntax count;                         // 数组长度常量语法。
};

// 结构体初始化器中的单个字段节点，直接继承 AstNode；对应 structExprField。
// name 是字段名，value 是冒号后的字段值表达式。
struct StructInitializerField final : AstNode {
    StructInitializerField();
    ~StructInitializerField() override;
    [[nodiscard]] AstChildren children() const override;
    std::string name;                               // 初始化的字段名。
    std::unique_ptr<Expr> value;                    // 冒号后的字段值表达式子树。
};

// 结构体初始化表达式节点，继承 Expr；对应 pathInExpression 后跟 { structExprFields }。
// path 标识要构造的结构体，fields 保存初始化字段及其源码顺序。
struct StructInitExpr final : Expr {
    StructInitExpr();
    [[nodiscard]] AstChildren children() const override;
    Path path;                                     // 被构造结构体的路径。
    std::vector<StructInitializerField> fields;    // 显式给出的字段初始化列表。
};

// Parentheses and ANTLR's precedence/condition/closed grammar wrappers do not
// create AST classes: AstBuilder folds them into the expression classes above.

}  // namespace rx::ast
