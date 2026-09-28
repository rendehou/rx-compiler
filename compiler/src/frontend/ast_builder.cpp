#include "rx/frontend/ast_builder.hpp"

#include <stdexcept>

namespace rx::frontend {

std::unique_ptr<ast::Program>
AstBuilder::build(RxParser::CrateContext* context) {
    auto program = std::make_unique<ast::Program>();

    for (auto* itemContext : context->item()) {
        program->items.push_back(buildItem(itemContext));
    }

    return program;
}

std::unique_ptr<ast::Item>
AstBuilder::buildItem(RxParser::ItemContext* context) {
    if (auto* function = context->functionDefinition()) {
        return buildFunction(function);
    }

    throw std::logic_error("this first example only supports functions");
}

std::unique_ptr<ast::FunctionItem>
AstBuilder::buildFunction(RxParser::FunctionDefinitionContext* context) {
    auto function = std::make_unique<ast::FunctionItem>();

    function->name = context->identifier()->getText();
    function->body = buildBlock(context->blockExpression());

    return function;
}

ast::Block
AstBuilder::buildBlock(RxParser::BlockExpressionContext* context) {
    if (!context->statement().empty() ||
        context->statementExpression() != nullptr) {
        throw std::logic_error("this first example only supports an empty body");
    }

    return ast::Block{};
}

}  // namespace rx::frontend