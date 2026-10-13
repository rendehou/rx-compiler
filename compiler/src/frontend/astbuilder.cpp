#include "rx/frontend/astbuilder.hpp"

#include "rx/ast/item.hpp"
#include "rx/ast/expression.hpp"
#include "rx/ast/statement.hpp"
#include "rx/ast/operator.hpp"
#include "rx/ast/node.hpp"
#include "rx/ast/program.hpp"
#include "rx/ast/type_syntax.hpp"
#include "rx/ast/source_span.hpp"

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

        // impl的目标类型,在impl中定义语句的类型
        impl->target_type = asNode<ast::TypeSyntax>(
            visit(ctx->typeRef())
        );

        // where语句，是用于约束泛型的
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

        return std::static_pointer_cast<ast::AstNode>(use);
    }

    //第二部分
    std::any AstBuilder::visitGenericParams(
        Parser::GenericParamsContext* ctx
    ) {

        auto parameters = std::make_shared<ast::GenericParameters>();

        parameters->written = true;//存在这个泛型参数
        for (
            auto* lifetimeCtx : ctx->lifetimeParam()
        ) {
            parameters->lifetimes.push_back(
                asNode<ast::LifetimeParameter>(
                    visit(lifetimeCtx)
                )
            );
        }


        return std::static_pointer_cast<ast::AstNode>(parameters);
    }

    std::any AstBuilder::visitLifetimeParam(
        Parser::LifetimeParamContext* ctx
    ) {
        auto lifetime = std::make_shared<ast::LifetimeParameter>();

        lifetime->name = ctx->lifetime()->getText();

        if (auto* bounds = ctx->lifetimeBounds()) {
            for (auto* boundCtx :bounds->lifetime()) {
                lifetime->bounds.push_back(
                    boundCtx->getText()
                );
            }
        }

        return std::static_pointer_cast<ast::AstNode>(lifetime);
    }

    std::any AstBuilder::visitSelfParam(//代表自身的参数
        Parser::SelfParamContext* ctx
    ) {

        auto receiver = std::make_shared<ast::Receiver>();
        if (ctx->AMP()) {
            if (ctx->MUT()) {//参数类别
                receiver->kind = ast::ReceiverKind::MutableReference;
            } else {
                receiver->kind = ast::ReceiverKind:: SharedReference;
            }
        } else {

            if (ctx->MUT()) {
                receiver->kind = ast::ReceiverKind:: MutableValue;
            } else {
                receiver->kind =ast::ReceiverKind:: Value;
            }
        }

        if (ctx->lifetime()) {
            receiver->lifetime = ctx->lifetime()->getText();
        }
        return std::static_pointer_cast<ast::AstNode>(receiver);
    }


    std::any AstBuilder::visitFunctionParam(
        Parser::FunctionParamContext* ctx
    ) {

        auto parameter =std::make_shared<ast::FunctionParameter>();

        parameter->name = ctx->identifierBinding()->identifier()->getText();

        parameter->mutable_binding = ctx->identifierBinding()->MUT()!= nullptr;//判断是否有mut

        parameter->type = asNode<ast::TypeSyntax>(
             visit(ctx->typeRef())
        );

        return std::static_pointer_cast<ast::AstNode>(parameter);
    }
    std::any AstBuilder::visitWhereClause(
        Parser::WhereClauseContext* ctx
    ) {

        auto where = std::make_shared<ast::WhereClause>();

        where->written = true;

        for (auto* itemCtx : ctx->whereClauseItem()) {
            where->predicates.push_back(
                asNode<ast::WherePredicate>(//对应 whereClauseItem 规则的两种形式。保存主体及其生命周期边界。
                    visit(itemCtx)
                )
            );
        }
        return std::static_pointer_cast<ast::AstNode>(where);
    }

    std::any AstBuilder::visitWhereClauseItem(
        Parser::WhereClauseItemContext* ctx
    ) {
        auto predicate = std::make_shared<ast::WherePredicate>();

        if (ctx->lifetime()) {//lifetime bound
            predicate->subject_is_lifetime = true;
            predicate->lifetime_subject = ctx->lifetime()->getText();

            if (auto* bounds = ctx->lifetimeBounds()) {
                for (auto* boundCtx : bounds->lifetime()) {
                    predicate->lifetime_bounds.push_back(
                        boundCtx->getText()
                    );
                }
            }
        } else {//都在是typeparam
            predicate->subject_is_lifetime = false;

            predicate->type_subject = asNode<ast::TypeSyntax>(
                visit(ctx->typeRef())
            );

            if (auto* bounds = ctx->typeParamBounds()) {
                for (auto* boundCtx : bounds->lifetime()) {
                    predicate->lifetime_bounds.push_back(
                        boundCtx->getText()
                    );
                }
            }
        }

        return std::static_pointer_cast<ast::AstNode>(
            predicate
        );
    }


    std::any AstBuilder::visitStructField(
        Parser::StructFieldContext* ctx
    ) {
        auto field = std::make_shared<ast::StructField>();

        field->name = ctx->identifier()->getText();

        field->type = asNode<ast::TypeSyntax>(
            visit(ctx->typeRef())
        );

        return std::static_pointer_cast<ast::AstNode>(
            field
        );
    }

    std::any AstBuilder::visitOuterAttribute(
        Parser::OuterAttributeContext* ctx
    ) {
        auto attribute = std::make_shared<ast::DeriveAttribute>();

        for (auto* deriveCtx : ctx->deriveName()) {
            std::string name = deriveCtx->getText();

            if (name == "Copy") {
                attribute->traits.push_back(ast::DeriveTrait::Copy);
            } else if (name == "Clone") {
                attribute->traits.push_back(ast::DeriveTrait::Clone);
            } else if (name == "PartialEq") {
                attribute->traits.push_back(ast::DeriveTrait::PartialEq);
            } else if (name == "Eq") {
                attribute->traits.push_back(ast::DeriveTrait::Eq);
            }
        }

        return std::static_pointer_cast<ast::AstNode>(
            attribute
        );
    }

    std::any AstBuilder::visitAssociatedItem(
        Parser::AssociatedItemContext* ctx
    ) {
        auto item = std::make_shared<ast::AssociatedItem>();

        if (ctx->functionDefinition()) {
            item->value = asNode<ast::FunctionItem>(
                visit(ctx->functionDefinition())
            );
        } else {
            item->value = asNode<ast::ConstantItem>(
                visit(ctx->constantItem())
            );
        }

        return std::static_pointer_cast<ast::AstNode>(
            item
        );
    }

    //下面的是第三部分，type和path
    std::any AstBuilder::visitTypeRef(
        Parser::TypeRefContext* ctx
    ) {
        if (ctx->LPAREN()) {//判断左括号分类

            //(type)形
            if (ctx->typeRef()) {
                return visit(ctx->typeRef());
            }

            //空括号
            auto type = std::make_shared<ast::TypeSyntax>();
            type->kind = ast::TypeSyntaxKind::Unit;

            return std::static_pointer_cast<ast::AstNode>(
                type
            );
        }

        if (ctx->typePath()) {//没有括号会很三类，讨论一下
            auto type = std::make_shared<ast::TypeSyntax>();
            type->kind = ast::TypeSyntaxKind::Path;
            type->path = asNode<ast::Path>(
                visit(ctx->typePath())
            );

            return std::static_pointer_cast<ast::AstNode>(
                type
            );
        }

        if (ctx->referenceType()) {
            return visit(ctx->referenceType());
        }

        return visit(ctx->arrayType());
    }


    std::any AstBuilder::visitReferenceType(//引用类型
        Parser::ReferenceTypeContext* ctx
    ) {
        auto type = std::make_shared<ast::TypeSyntax>();

        type->kind = ast::TypeSyntaxKind::Reference;
        type->mutable_reference = ctx->MUT() != nullptr;

        if (ctx->lifetime()) {
            type->lifetime = ctx->lifetime()->getText();
        }

        type->referent = asNode<ast::TypeSyntax>(
            visit(ctx->typeRef())
        );

        // &&T在语法上表示两层引用
        if (ctx->ANDAND()) {
            auto outer = std::make_shared<ast::TypeSyntax>();

            outer->kind = ast::TypeSyntaxKind::Reference;
            outer->referent = type;

            return std::static_pointer_cast<ast::AstNode>(
                outer
            );
        }

        return std::static_pointer_cast<ast::AstNode>(
            type
        );
    }


    std::any AstBuilder::visitArrayType(
        Parser::ArrayTypeContext* ctx
    ) {
        auto type = std::make_shared<ast::TypeSyntax>();

        type->kind = ast::TypeSyntaxKind::Array;

        type->array_element = asNode<ast::TypeSyntax>(
            visit(ctx->typeRef())
        );

        type->length = asNode<ast::ConstValueSyntax>(
            visit(ctx->constValue())
        );

        return std::static_pointer_cast<ast::AstNode>(type);
    }


    std::any AstBuilder::visitTypePath(
        Parser::TypePathContext* ctx
    ) {
        auto path = std::make_shared<ast::Path>();

        for (auto* segmentCtx : ctx->typePathSegment()) {
            path->segments.push_back(
                asNode<ast::PathSegment>(
                    visit(segmentCtx)
                )
            );
        }

        return std::static_pointer_cast<ast::AstNode>(
            path
        );
    }


    std::any AstBuilder::visitTypePathSegment(//路径的一段
        Parser::TypePathSegmentContext* ctx
    ) {
        auto segment = std::make_shared<ast::PathSegment>();

        segment->name = ctx->pathIdentSegment()->getText();

        if (auto* arguments = ctx->genericArgs()) {//如果后面有泛型参数
            for (auto* argumentCtx : arguments->genericArg()) {
                segment->generic_arguments.push_back(
                    asNode<ast::GenericArgumentSyntax>(
                        visit(argumentCtx)
                    )
                );
            }
        }

        return std::static_pointer_cast<ast::AstNode>(
            segment
        );
    }


    std::any AstBuilder::visitPathInExpression(//一整条路径
        Parser::PathInExpressionContext* ctx
    ) {
        auto path = std::make_shared<ast::Path>();

        for (auto* segmentCtx : ctx->pathExprSegment()) {
            path->segments.push_back(
                asNode<ast::PathSegment>(
                    visit(segmentCtx)
                )
            );
        }

        return std::static_pointer_cast<ast::AstNode>(
            path
        );
    }


    std::any AstBuilder::visitPathExprSegment(//表达式版本的
        Parser::PathExprSegmentContext* ctx
    ) {
        auto segment = std::make_shared<ast::PathSegment>();

        segment->name = ctx->pathIdentSegment()->getText();

        if (auto* arguments = ctx->genericArgs()) {
            for (auto* argumentCtx : arguments->genericArg()) {
                segment->generic_arguments.push_back(
                    asNode<ast::GenericArgumentSyntax>(
                        visit(argumentCtx)
                    )
                );
            }
        }

        return std::static_pointer_cast<ast::AstNode>(
            segment
        );
    }


    std::any AstBuilder::visitGenericArg(
        Parser::GenericArgContext* ctx
    ) {
        auto argument = std::make_shared<ast::GenericArgumentSyntax>();

        if (ctx->lifetime()) {
            argument->kind = ast::GenericArgumentKind::Lifetime;
            argument->lifetime = ctx->lifetime()->getText();
        } else {
            argument->kind = ast::GenericArgumentKind::Type;

            argument->type = asNode<ast::TypeSyntax>(
                visit(ctx->typeRef())
            );
        }

        return std::static_pointer_cast<ast::AstNode>(
            argument
        );
    }


    std::any AstBuilder::visitClosedCastType(//用来处理歧义
        Parser::ClosedCastTypeContext* ctx
    ) {

        //()或者(type)
        if (ctx->LPAREN()) {
            if (ctx->typeRef()) {
                return visit(ctx->typeRef());
            }

            auto type = std::make_shared<ast::TypeSyntax>();
            type->kind = ast::TypeSyntaxKind::Unit;

            return std::static_pointer_cast<ast::AstNode>(
                type
            );
        }

        if (ctx->arrayType()) {
            return visit(ctx->arrayType());
        }

        //引用类型
        if (ctx->closedCastType()) {
            auto type = std::make_shared<ast::TypeSyntax>();

            type->kind = ast::TypeSyntaxKind::Reference;
            type->mutable_reference = ctx->MUT() != nullptr;

            if (ctx->lifetime()) {
                type->lifetime = ctx->lifetime()->getText();
            }

            type->referent = asNode<ast::TypeSyntax>(
                visit(ctx->closedCastType())
            );

            if (ctx->ANDAND()) {
                auto outer = std::make_shared<ast::TypeSyntax>();

                outer->kind = ast::TypeSyntaxKind::Reference;
                outer->referent = type;

                return std::static_pointer_cast<ast::AstNode>(
                    outer
                );
            }

            return std::static_pointer_cast<ast::AstNode>(
                type
            );
        }

        // 最后一种情况是带显式泛型参数的path
        auto path = std::make_shared<ast::Path>();

        for (auto* segmentCtx : ctx->typePathSegment()) {
            path->segments.push_back(
                asNode<ast::PathSegment>(
                    visit(segmentCtx)
                )
            );
        }

        auto lastSegment = std::make_shared<ast::PathSegment>();
        lastSegment->name = ctx->pathIdentSegment()->getText();

        for (auto* argumentCtx : ctx->genericArgs()->genericArg()) {
            lastSegment->generic_arguments.push_back(
                asNode<ast::GenericArgumentSyntax>(
                    visit(argumentCtx)
                )
            );
        }

        path->segments.push_back(lastSegment);

        auto type = std::make_shared<ast::TypeSyntax>();
        type->kind = ast::TypeSyntaxKind::Path;
        type->path = path;

        return std::static_pointer_cast<ast::AstNode>(
            type
        );
    }


    std::any AstBuilder::visitConstValue(
        Parser::ConstValueContext* ctx
    ) {

        if (ctx->INTEGER_LITERAL()) {
            auto value = std::make_shared<ast::ConstValueSyntax>();

            value->kind = ast::ConstValueKind::Integer;
            value->integer_spelling = ctx->INTEGER_LITERAL()->getText();

            return std::static_pointer_cast<ast::AstNode>(
                value
            );
        }

        if (ctx->TRUE() || ctx->FALSE()) {
            auto value = std::make_shared<ast::ConstValueSyntax>();

            value->kind = ast::ConstValueKind::Boolean;
            value->boolean_value = ctx->TRUE() != nullptr;

            return std::static_pointer_cast<ast::AstNode>(
                value
            );
        }

        if (ctx->pathInExpression()) {
            auto value = std::make_shared<ast::ConstValueSyntax>();

            value->kind = ast::ConstValueKind::Path;
            value->path = asNode<ast::Path>(
                visit(ctx->pathInExpression())
            );

            return std::static_pointer_cast<ast::AstNode>(
                value
            );
        }

        if (ctx->MINUS()) {
            auto value = std::make_shared<ast::ConstValueSyntax>();
            value->kind = ast::ConstValueKind::Negate;

            auto* magnitude = ctx->magnitude();

            while (magnitude->magnitude()) {
                magnitude = magnitude->magnitude();
            }

            auto operand = std::make_shared<ast::ConstValueSyntax>();

            if (magnitude->INTEGER_LITERAL()) {
                operand->kind = ast::ConstValueKind::Integer;
                operand->integer_spelling = magnitude->INTEGER_LITERAL()->getText();
            } else {
                operand->kind = ast::ConstValueKind::Path;
                operand->path = asNode<ast::Path>(
                    visit(magnitude->pathInExpression())
                );
            }

            value->operand = operand;

            return std::static_pointer_cast<ast::AstNode>(
                value
            );
        }

        //括号不会出现在AST中
        return visit(ctx->constValue());
    }

    //blockand statement
    std::any AstBuilder::visitBlockExpression(
        Parser::BlockExpressionContext* ctx
    ) {
        auto block = std::make_shared<ast::Block>();

        for (auto* statementCtx : ctx->statement()) {
            block->statements.push_back(
                asNode<ast::Statement>(
                    visit(statementCtx)
                )
            );
        }

        if (ctx->statementExpression()) {//块尾指令
            block->tail = asNode<ast::Expr>(
                visit(ctx->statementExpression())
            );
        }

        return std::static_pointer_cast<ast::AstNode>(
            block
        );
    }

    std::any AstBuilder::visitStatement(
        Parser::StatementContext* ctx
    ) {

        // 空语句
        if (!ctx->letStatement()
            && !ctx->expressionWithBlock()
            && !ctx->statementExpression()) {

            auto statement = std::make_shared<ast::EmptyStatement>();

            return std::static_pointer_cast<ast::AstNode>(
                statement
            );
        }

        if (ctx->letStatement()) {
            return visit(ctx->letStatement());
        }

        auto statement = std::make_shared<ast::ExpressionStatement>();

        if (ctx->expressionWithBlock()) {
            statement->expression = asNode<ast::Expr>(
                visit(ctx->expressionWithBlock())
            );

            statement->has_semicolon = ctx->SEMI() != nullptr;
        } else {
            statement->expression = asNode<ast::Expr>(
                visit(ctx->statementExpression())
            );

            statement->has_semicolon = true;
        }

        return std::static_pointer_cast<ast::AstNode>(
            statement
        );
    }

    std::any AstBuilder::visitLetStatement(
        Parser::LetStatementContext* ctx
    ) {
        auto statement = std::make_shared<ast::LetStatement>();

        statement->name = ctx->identifierBinding()->identifier()->getText();
        statement->mutable_binding = ctx->identifierBinding()->MUT() != nullptr;

        if (ctx->typeRef()) {
            statement->declared_type = asNode<ast::TypeSyntax>(
                visit(ctx->typeRef())
            );
        }

        statement->initializer = asNode<ast::Expr>(
            visit(ctx->expression())
        );

        return std::static_pointer_cast<ast::AstNode>(
            statement
        );
    }

     std::any AstBuilder::visitExpressionWithBlock(
        Parser::ExpressionWithBlockContext* ctx
    ) {

        if (ctx->blockExpression() && !ctx->LOOP() && !ctx->WHILE()) {
            auto expression = std::make_shared<ast::BlockExpr>();

            expression->block = asNode<ast::Block>(
                visit(ctx->blockExpression())
            );

            return std::static_pointer_cast<ast::AstNode>(
                expression
            );
        }

        if (ctx->ifExpression()) {
            return visit(ctx->ifExpression());
        }

        if (ctx->LOOP()) {
            auto expression = std::make_shared<ast::LoopExpr>();

            expression->body = asNode<ast::Block>(
                visit(ctx->blockExpression())
            );

            return std::static_pointer_cast<ast::AstNode>(
                expression
            );
        }

        auto expression = std::make_shared<ast::WhileExpr>();

        expression->condition = asNode<ast::Expr>(
            visit(ctx->conditionExpression())
        );

        expression->body = asNode<ast::Block>(
            visit(ctx->blockExpression())
        );

        return std::static_pointer_cast<ast::AstNode>(
            expression
        );
    }
}