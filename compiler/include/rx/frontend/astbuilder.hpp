#pragma once

#include <any>
#include <memory>

#include "ParserBaseVisitor.h"
#include "rx/ast/program.hpp"

namespace rx::frontend {

class AstBuilder : public ParserBaseVisitor {//来自默认访问节点的继承
public:
    std::unique_ptr<ast::Program>
    build(Parser::CrateContext* ctx);//实际的build函数，生成ast节点

    std::any visitFunctionDefinition(Parser::FunctionDefinitionContext* ctx) override;

private:
    ast::Program* current_program_ = nullptr;
};

} // namespace rx::frontend