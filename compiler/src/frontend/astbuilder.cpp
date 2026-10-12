#include "rx/frontend/astbuilder.hpp"

#include "rx/ast/item.hpp"

namespace rx::frontend {

    template <typename T>
    std::shared_ptr<T> asNode(std::any value) {//辅助函数，方便转换any到实际的类
        return std::dynamic_pointer_cast<T>(
            std::any_cast<std::shared_ptr<ast::AstNode>>(value)
        );
    }

    std::shared_ptr<ast::Program> AstBuilder::build(
        Parser::CrateContext* ctx
    ) {
        
        auto node = std::any_cast<std::shared_ptr<ast::AstNode>>(
            visit(ctx)
        );

        return std::dynamic_pointer_cast<ast::Program>(
            node
        );
    }


    //下面开始是具体的
    std::any AstBuilder::visitCrate(Parser::CrateContext* ctx) {

        auto program = std::make_shared<ast::Program>();//创建节点
        for (auto* itemCtx : ctx->item()) {
            /*auto node = std::any_cast<std::shared_ptr<ast::AstNode>>(visit(itemCtx));
            auto item = std::dynamic_pointer_cast<ast::Item>(node);
            program->items.push_back(item);*/
            program->items.push_back(asNode<ast::Item>(
                visit(itemCtx)
            ));//使用辅助函数类型转换
        }

        return std::static_pointer_cast<ast::AstNode>(program);
    }

    std::any AstBuilder::visitFunctionDefinition(
        Parser::FunctionDefinitionContext* ctx
    ) {
        auto function = std::make_shared<ast::FunctionItem>();

        // 函数名
        function->name = ctx->identifier()->getText();

        // 泛型参数
        if (auto* generics = ctx->genericParams()) {
            function->generic_parameters = asNode<ast::GenericParameters>(
                visit(generics)
            );
        }

        // 函数参数
        if (auto* parameters = ctx->functionParameters()) {

            // self 参数
            if (auto* self = parameters->selfParam()) {
                function->receiver = asNode<ast::Receiver>(
                    visit(self)
                );
            }

            // 普通参数
            for (auto* paramCtx : parameters->functionParam()) {
                function->parameters.push_back(
                    asNode<ast::FunctionParameter>(
                        visit(paramCtx)
                    )
                );
            }
        }

        // 返回类型
        if (auto* returnType = ctx->typeRef()) {
            function->return_type = asNode<ast::TypeSyntax>(
                visit(returnType)
            );
        }

        // where
        if (auto* where = ctx->whereClause()) {
            function->where_clause = asNode<ast::WhereClause>(
                visit(where)
            );
        }

        // 函数体
        function->body = asNode<ast::Block>(
            visit(ctx->blockExpression())
        );

        return std::static_pointer_cast<ast::AstNode>(
            function
        );
    }


    std::any AstBuilder::visitStructDefinition(
        Parser::StructDefinitionContext* ctx
    ) {
        auto structure =  std::make_shared<ast::StructItem>();

        // 结构体名
        structure->name = ctx->identifier()->getText();

        // 泛型
        if (auto* generics = ctx->genericParams()) {
            structure->generic_parameters = asNode<ast::GenericParameters>(
                visit(generics)
            );
        }

        // where
        if (auto* where = ctx->whereClause()) {
            structure->where_clause = asNode<ast::WhereClause>(
                visit(where)
            );
        }

        // #[derive(...)] 等属性,用于自动完成类，具体有复制，判断等号等
        for (auto* attributeCtx : ctx->outerAttribute()) {
            structure->attributes.push_back(
                asNode<ast::DeriveAttribute>(
                    visit(attributeCtx)
                )
            );
        }

        // struct里的字段
        for (auto* fieldCtx : ctx->structField()) {
            structure->fields.push_back(
                asNode<ast::StructField>(
                    visit(fieldCtx)
                )
            );
        }

        return std::static_pointer_cast<ast::AstNode>(
            structure
        );
    }


    std::any AstBuilder::visitConstantItem(
        Parser::ConstantItemContext* ctx
    ) {
        auto constant = std::make_shared<ast::ConstantItem>();

        // 常量名
        constant->name = ctx->identifier()->getText();

        // 类型
        constant->type = asNode<ast::TypeSyntax>(
            visit(ctx->typeRef())
        );

        // 初始值
        constant->initializer = asNode<ast::ConstValueSyntax>(
            visit(ctx->constValue())
        );

        return std::static_pointer_cast<ast::AstNode>(
            constant
        );
    }


    std::any AstBuilder::visitInherentImpl(
        Parser::InherentImplContext* ctx
    ) {
        auto impl = std::make_shared<ast::ImplItem>();

        // 泛型，也就是待定具体使用类型，等使用的时候实例化
        if (auto* generics = ctx->genericParams()) {
            impl->generic_parameters = asNode<ast::GenericParameters>(
                visit(generics)
            );
        }

        // impl 的目标类型
        impl->target_type = asNode<ast::TypeSyntax>(
            visit(ctx->typeRef())
        );

        // where
        if (auto* where = ctx->whereClause()) {
            impl->where_clause = asNode<ast::WhereClause>(
                visit(where)
            );
        }

        // impl内部成员
        for (auto* itemCtx : ctx->associatedItem()) {
            impl->items.push_back(
                asNode<ast::AssociatedItem>(
                    visit(itemCtx)
                ));
        }

        return std::static_pointer_cast<ast::AstNode>(impl);
    }


    std::any AstBuilder::visitUseDeclaration(
        Parser::UseDeclarationContext* ctx
    ) {
        auto use = std::make_shared<ast::UseItem>();

        use->tree = asNode<ast::UseTree>(
                visit(ctx->useTree())
         );

        return std::static_pointer_cast<ast::AstNode>(
            use
        );
    }
}