#include "rx/frontend/astbuilder.hpp"

#include "rx/ast/item.hpp"

namespace rx::frontend {

    std::shared_ptr<ast::Program>
    AstBuilder::build(Parser::CrateContext* ctx) {//这是建立整个ast的函数，从根节点对应的context开始
        auto program = std::make_shared<ast::Program>();

        current_program_ = program.get();

        visit(ctx);

        current_program_ = nullptr;

        return program;
    }


    //下面开始是具体的
    std::any AstBuilder::visitCrate(Parser::CrateContext* ctx) {
        auto crate = 
    }

    std::any AstBuilder::visitFunctionDefinition(Parser::FunctionDefinitionContext* ctx) {
        auto function = std::make_shared<ast::FunctionItem>();//建立一个新节点

        function->name = ctx->identifier()->getText();//在context tree中读取相应信息

        current_program_->items.push_back(//
            std::move(function)
        );

        return {};
    }
}