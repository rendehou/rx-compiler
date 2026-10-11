#include "rx/frontend/astbuilder.hpp"

#include "rx/ast/item.hpp"

namespace rx::frontend {

    std::unique_ptr<ast::Program>
    AstBuilder::build(Parser::CrateContext* ctx) {
        auto program = std::make_unique<ast::Program>();

        current_program_ = program.get();

        visit(ctx);

        current_program_ = nullptr;

        return program;
    }
}