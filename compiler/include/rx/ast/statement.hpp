#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rx/ast/node.hpp"
#include "rx/ast/type_syntax.hpp"

namespace rx::ast {

struct Expr;

// 语句类别的抽象多态基类，继承 AstNode；对应 Parser.g4 的 statement 规则。
// EmptyStatement、LetStatement、ExpressionStatement 是它的具体子类。
struct Statement : AstNode {
    virtual ~Statement() = default;
    [[nodiscard]] AstChildren children() const override = 0;
protected:
    explicit Statement(NodeKind kind) : AstNode(kind) {}
};

// 空语句节点，继承 Statement；对应单独一个分号的语句形式。
// 语法没有携带其他值，因此只有继承来的类别和源码位置。
struct EmptyStatement final : Statement {
    EmptyStatement();
    [[nodiscard]] AstChildren children() const override;
};

// let 绑定语句节点，继承 Statement；对应 letStatement 规则。
// 保存绑定名、mut 标记、可选显式类型和等号右侧的初始化表达式。
struct LetStatement final : Statement {
    LetStatement();
    ~LetStatement() override;
    [[nodiscard]] AstChildren children() const override;
    std::string name;                              // 被声明的变量名。
    bool mutable_binding = false;                 // 绑定是否写了 mut。
    std::optional<TypeSyntax> declared_type;       // 可选冒号类型；未显式标注时为空。
    std::unique_ptr<Expr> initializer;             // 等号后的初始化表达式，拥有该子树。
};

// 表达式语句节点，继承 Statement；对应带分号的表达式或可选分号的块表达式语句。
// expression 保存表达式子树，has_semicolon 保留源码是否有分号这一语法区别。
struct ExpressionStatement final : Statement {
    ExpressionStatement();
    ~ExpressionStatement() override;
    [[nodiscard]] AstChildren children() const override;
    std::unique_ptr<Expr> expression;              // 语句中的表达式子树。
    bool has_semicolon = false;                    // 源码中是否实际写了分号。
};

// 代码块节点，直接继承 AstNode；对应 blockExpression 规则。
// statements 保存普通语句，tail 保存可选的末尾无分号表达式，二者角色不同。
struct Block final : AstNode {
    Block();
    ~Block() override;
    Block(Block&&) noexcept;
    Block& operator=(Block&&) noexcept;
    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;
    [[nodiscard]] AstChildren children() const override;

    std::vector<std::unique_ptr<Statement>> statements; // 花括号内按顺序出现的普通语句。
    std::unique_ptr<Expr> tail;                         // 可选末尾值表达式；不存在时为空。
};

}  // namespace rx::ast
