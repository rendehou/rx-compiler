#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rx/ast/source_span.hpp"
#include "rx/ast/type_syntax.hpp"

namespace rx::ast {

struct Expr;

struct Statement {
    SourceSpan span{};
    virtual ~Statement() = default;
};

struct EmptyStatement final : Statement {};

struct LetStatement final : Statement {
    std::string name;
    bool mutable_binding = false;
    std::optional<TypeSyntax> declared_type;
    std::unique_ptr<Expr> initializer;

    ~LetStatement() override;
};

struct ExpressionStatement final : Statement {
    std::unique_ptr<Expr> expression;
    bool has_semicolon = false;

    ~ExpressionStatement() override;
};

// Ordinary statements and the optional final value are deliberately separate.
struct Block {
    SourceSpan span{};
    std::vector<std::unique_ptr<Statement>> statements;
    std::unique_ptr<Expr> tail;

    Block() = default;
    ~Block();
    Block(Block&&) noexcept;
    Block& operator=(Block&&) noexcept;
    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;
};

}  // namespace rx::ast
