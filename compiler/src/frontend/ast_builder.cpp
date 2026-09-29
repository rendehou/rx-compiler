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

//Item及其子类
std::unique_ptr<ast::Item>
AstBuilder::buildItem(RxParser::ItemContext* context) {
    if (auto* use = context->useDeclaration()) {
        return buildUseDeclaration(use);
    }

    if (auto* function = context->functionDefinition()) {
        return buildFunction(function);
    }

    if (auto* structure = context->structDefinition()) {
        return buildStructDefinition(structure);
    }

    if (auto* constant = context->constantItem()) {
        return buildConstantItem(constant);
    }

    if (auto* impl = context->inherentImpl()) {
        return buildInherentImpl(impl);
    }

    throw std::logic_error("unknown item form");
}

std::unique_ptr<ast::Item>
AstBuilder::buildUseDeclaration(RxParser::UseDeclarationContext* context) {
    auto use = std::make_unique<ast::UseItem>();
    use->tree = buildUseTree(context->useTree());
    return use;
}

ast::UseTree
AstBuilder::buildUseTree(RxParser::UseTreeContext* context) {
    ast::UseTree tree;

    // 收集路径前缀段，并判断路径是否以 :: 开头。
    if (auto* path = context->usePath()) {
        for (auto* segment : path->usePathSegment()) {
            tree.prefix.push_back(segment->getText());
        }
        // usePath 自带前导 :: 时，PATHSEP 的数量不少于段数。
        tree.leading_separator = path->PATHSEP().size() >= path->usePathSegment().size();
    } else {
        tree.leading_separator = context->PATHSEP() != nullptr;
    }

    if (context->STAR() != nullptr) {
        tree.form = ast::UseTreeKind::Glob;
        return tree;
    }

    if (context->LBRACE() != nullptr) {
        tree.form = ast::UseTreeKind::Group;
        for (auto* nested : context->useTree()) {
            tree.group_items.push_back(buildUseTree(nested));
        }
        return tree;
    }

    // usePath (AS (identifier | UNDERSCORE))?
    tree.form = ast::UseTreeKind::Path;
    if (context->AS() != nullptr) {
        if (context->identifier() != nullptr) {
            tree.alias = context->identifier()->getText();
        } else if (context->UNDERSCORE() != nullptr) {
            tree.alias_is_underscore = true;
        }
    }
    return tree;
}

std::unique_ptr<ast::FunctionItem>
AstBuilder::buildFunction(RxParser::FunctionDefinitionContext* context) {
    auto function = std::make_unique<ast::FunctionItem>();

    function->name = context->identifier()->getText();
    if (auto* generics = context->genericParams()) {
        function->generic_parameters = buildGenericParameters(generics);
    }
    if (auto* parameters = context->functionParameters()) {
        if (auto* self = parameters->selfParam()) {
            function->receiver = buildReceiver(self);
        }
        for (auto* parameter : parameters->functionParam()) {
            function->parameters.push_back(buildFunctionParameter(parameter));
        }
    }
    if (context->ARROW() != nullptr) {
        function->return_type = buildTypeRef(context->typeRef());
    }
    if (auto* where = context->whereClause()) {
        function->where_clause = buildWhereClause(where);
    }
    function->body = buildBlock(context->blockExpression());

    return function;
}

std::unique_ptr<ast::StructItem>
AstBuilder::buildStructDefinition(RxParser::StructDefinitionContext* context) {
    auto structure = std::make_unique<ast::StructItem>();

    structure->name = context->identifier()->getText();
    if (auto* generics = context->genericParams()) {
        structure->generic_parameters = buildGenericParameters(generics);
    }
    if (auto* where = context->whereClause()) {
        structure->where_clause = buildWhereClause(where);
    }
    for (auto* attribute : context->outerAttribute()) {
        structure->attributes.push_back(buildOuterAttribute(attribute));
    }
    for (auto* field : context->structField()) {
        ast::StructField result;
        result.name = field->identifier()->getText();
        result.type = buildTypeRef(field->typeRef());
        structure->fields.push_back(std::move(result));
    }

    return structure;
}

ast::DeriveAttribute
AstBuilder::buildOuterAttribute(RxParser::OuterAttributeContext* context) {
    ast::DeriveAttribute attribute;

    for (auto* name : context->deriveName()) {
        if (name->COPY() != nullptr) {
            attribute.traits.push_back(ast::DeriveTrait::Copy);
        } else if (name->CLONE() != nullptr) {
            attribute.traits.push_back(ast::DeriveTrait::Clone);
        } else if (name->PARTIAL_EQ() != nullptr) {
            attribute.traits.push_back(ast::DeriveTrait::PartialEq);
        } else if (name->EQ() != nullptr) {
            attribute.traits.push_back(ast::DeriveTrait::Eq);
        }
    }

    return attribute;
}

std::unique_ptr<ast::ConstantItem>
AstBuilder::buildConstantItem(RxParser::ConstantItemContext* context) {
    auto constant = std::make_unique<ast::ConstantItem>();

    constant->name = context->identifier()->getText();
    constant->type = buildTypeRef(context->typeRef());
    constant->initializer = buildConstValue(context->constValue());

    return constant;
}

std::unique_ptr<ast::ImplItem>
AstBuilder::buildInherentImpl(RxParser::InherentImplContext* context) {
    auto impl = std::make_unique<ast::ImplItem>();

    if (auto* generics = context->genericParams()) {
        impl->generic_parameters = buildGenericParameters(generics);
    }
    impl->target_type = buildTypeRef(context->typeRef());
    if (auto* where = context->whereClause()) {
        impl->where_clause = buildWhereClause(where);
    }
    for (auto* associated : context->associatedItem()) {
        ast::AssociatedItem item;
        if (auto* function = associated->functionDefinition()) {
            item.value = buildFunction(function);
        } else if (auto* constant = associated->constantItem()) {
            item.value = buildConstantItem(constant);
        }
        impl->items.push_back(std::move(item));
    }

    return impl;
}







ast::Block 
AstBuilder::buildBlock(RxParser::BlockExpressionContext* context) {
    if (!context->statement().empty() ||
        context->statementExpression() != nullptr) {
        throw std::logic_error("this first example only supports an empty body");
    }

    return ast::Block{};
}
std::unique_ptr<ast::Statement>
AstBuilder::buildStatement(RxParser::StatementContext* ctx) {
    if (auto* let = ctx->letStatement()) {
        return buildLetStatement(let);
    }

    if (auto* expr = ctx->expressionWithBlock()) {
        auto node = std::make_unique<ast::ExpressionStatement>();
        node->expression = buildExpressionWithBlock(expr);
        node->has_semicolon = ctx->SEMI() != nullptr;
        return node;
    }

    if (auto* expr = ctx->statementExpression()) {
        auto node = std::make_unique<ast::ExpressionStatement>();
        node->expression = buildStatementExpression(expr);
        node->has_semicolon = true;
        return node;
    }

    if (ctx->SEMI()) {
        return std::make_unique<ast::EmptyStatement>();
    }

    throw std::logic_error("unknown statement form");
}

}  // namespace rx::frontend