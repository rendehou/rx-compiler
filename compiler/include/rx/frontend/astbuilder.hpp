#pragma once

#include <any>
#include <memory>

#include "ParserBaseVisitor.h"
#include "rx/ast/program.hpp"

namespace rx::frontend {

class AstBuilder : public ParserBaseVisitor {//来自默认访问节点的继承
public:
    std::shared_ptr<ast::Program>
    build(Parser::CrateContext* ctx);//实际的build函数，生成ast节点

    std::any visitFunctionDefinition(Parser::FunctionDefinitionContext* ctx) override;
    std::any visitCrate(Parser::CrateContext* ctx) override;
    
private:
    ast::Program* current_program_ = nullptr; //实际节点内部的信息
};

} // namespace rx::frontend