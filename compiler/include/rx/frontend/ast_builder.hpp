#pragma once
#include <memory>
#include <rx/ast/expression.hpp>
#include <rx/ast/item.hpp>
#include <rx/ast/program.hpp>
#include <rx/ast/source_span.hpp>
#include <rx/ast/statement.hpp>
#include <rx/ast/type_syntax.hpp>
#include "rx/frontend/parser.hpp"
// Responsibility:
// - Declare conversion from real RxParser Context objects to ast::Program.
// - Provide private helpers for Item, TypeSyntax, Statement, Block, and Expr.
// - Perform structural validation and operator mapping only.
// Never perform name lookup, type checking, or retain Context pointers in AST.

namespace rx::frontend {
    class AstBuilder {
        public:
            std::unique_ptr<ast::Program> build(RxParser::CrateContext* context);
        private:
            std::unique_ptr<ast::Item> buildItem(RxParser::ItemContext* context);
            std::unique_ptr<ast::FunctionItem> buildFunction(RxParser::FunctionDefinitionContext* context);
            ast::Block buildBlock(RxParser::BlockExpressionContext* context);
    };
}
