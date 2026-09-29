#include "rx/frontend/ast_builder.hpp"

#include <stdexcept>

namespace rx::frontend {

// 把ANTLR的crate根Context转成AST根节点，并按源码顺序保存顶层item。
std::unique_ptr<ast::Program>
AstBuilder::build(RxParser::CrateContext* context) {
    auto program = std::make_unique<ast::Program>();

    // crate中的item*可为空；逐个构造以保留源码顺序。
    for (auto* itemContext : context->item()) {
        program->items.push_back(buildItem(itemContext));
    }

    return program;
}

// 根据item匹配到的子规则，分派到对应的AST构造函数。
std::unique_ptr<ast::Item>
AstBuilder::buildItem(RxParser::ItemContext* context) {
    // ANTLR为每个备选分支生成访问函数；非空Context表示该分支匹配成功。
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

    // 解析成功后应命中一支；否则说明grammar与构造器不一致。
    throw std::logic_error("unknown item form");
}

// 构造use节点；导入路径树交给buildUseTree递归处理。
std::unique_ptr<ast::Item>
AstBuilder::buildUseDeclaration(RxParser::UseDeclarationContext* context) {
    auto use = std::make_unique<ast::UseItem>();
    // 只保存有结构意义的useTree；关键字和分号不需要成为AST节点。
    use->tree = buildUseTree(context->useTree());
    return use;
}

// 构造一层useTree；分组成员递归构造。
ast::UseTree
AstBuilder::buildUseTree(RxParser::UseTreeContext* context) {
    ast::UseTree tree;

    // 保存路径前缀，例如a::b::{c,d}中的a::b。
    if (auto* path = context->usePath()) {
        for (auto* segment : path->usePathSegment()) {
            tree.prefix.push_back(segment->getText());
        }
        // 路径段之间各有一个::；分隔符数量不少于段数时，另有开头的::。
        tree.leading_separator = path->PATHSEP().size() >= path->usePathSegment().size();
    } else {
        tree.leading_separator = context->PATHSEP() != nullptr;
    }

    if (context->STAR() != nullptr) {
        // a::*是通配导入，只需保存前缀和Glob形式。
        tree.form = ast::UseTreeKind::Glob;
        return tree;
    }

    if (context->LBRACE() != nullptr) {
        // a::{b,c}是分组导入；成员可继续嵌套，需递归构造。
        tree.form = ast::UseTreeKind::Group;
        for (auto* nested : context->useTree()) {
            tree.group_items.push_back(buildUseTree(nested));
        }
        return tree;
    }

    // 普通路径可带别名；as _与具体别名含义不同，分别保存。
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

// 构造函数签名和函数体；self接收者与普通参数分开保存。
std::unique_ptr<ast::FunctionItem>
AstBuilder::buildFunction(RxParser::FunctionDefinitionContext* context) {
    auto function = std::make_unique<ast::FunctionItem>();

    // 保存函数名；泛型和where子句仅在源码出现时标记为written。
    function->name = context->identifier()->getText();
    if (auto* generics = context->genericParams()) {
        function->generic_parameters = buildGenericParameters(generics);
    }
    if (auto* parameters = context->functionParameters()) {
        // selfParam只能位于参数列表开头，单独保存为receiver。
        if (auto* self = parameters->selfParam()) {
            function->receiver = buildReceiver(self);
        }
        // 逐个构造普通参数；它们与receiver分开保存。
        for (auto* parameter : parameters->functionParam()) {
            function->parameters.push_back(buildFunctionParameter(parameter));
        }
    }
    if (context->ARROW() != nullptr) {
        // 只有出现->才保存显式返回类型。
        function->return_type = buildTypeRef(context->typeRef());
    }
    if (auto* where = context->whereClause()) {
        function->where_clause = buildWhereClause(where);
    }
    // 函数体是代码块；内部语句和尾表达式由buildBlock处理。
    function->body = buildBlock(context->blockExpression());

    return function;
}

// 构造结构体名称、泛型约束、属性和字段。
std::unique_ptr<ast::StructItem>
AstBuilder::buildStructDefinition(RxParser::StructDefinitionContext* context) {
    auto structure = std::make_unique<ast::StructItem>();

    // 属性和字段按源码顺序保存；字段类型只记录语法，不在此处做语义检查。
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
        // 保存字段名和类型；类型合法性及重名检查留给语义阶段。
        result.name = field->identifier()->getText();
        result.type = buildTypeRef(field->typeRef());
        structure->fields.push_back(std::move(result));
    }

    return structure;
}

// 把derive中的trait token映射为AST枚举。
ast::DeriveAttribute
AstBuilder::buildOuterAttribute(RxParser::OuterAttributeContext* context) {
    ast::DeriveAttribute attribute;

    // 保存每个derive项；支持范围由语义规则决定。
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

// 构造常量；初值使用受限的constValue，而非普通表达式。
std::unique_ptr<ast::ConstantItem>
AstBuilder::buildConstantItem(RxParser::ConstantItemContext* context) {
    auto constant = std::make_unique<ast::ConstantItem>();

    // const必须显式声明类型；初值与运行时表达式分开表示。
    constant->name = context->identifier()->getText();
    constant->type = buildTypeRef(context->typeRef());
    constant->initializer = buildConstValue(context->constValue());

    return constant;
}

// 构造inherent impl及其关联函数、常量。
std::unique_ptr<ast::ImplItem>
AstBuilder::buildInherentImpl(RxParser::InherentImplContext* context) {
    auto impl = std::make_unique<ast::ImplItem>();

    // 保存目标类型；成员归属和冲突检查留给语义阶段。
    if (auto* generics = context->genericParams()) {
        impl->generic_parameters = buildGenericParameters(generics);
    }
    impl->target_type = buildTypeRef(context->typeRef());
    if (auto* where = context->whereClause()) {
        impl->where_clause = buildWhereClause(where);
    }
    for (auto* associated : context->associatedItem()) {
        ast::AssociatedItem item;
        // associatedItem是函数或常量二选一，由variant保留具体种类。
        if (auto* function = associated->functionDefinition()) {
            item.value = buildFunction(function);
        } else if (auto* constant = associated->constantItem()) {
            item.value = buildConstantItem(constant);
        }
        impl->items.push_back(std::move(item));
    }

    return impl;
}

// 构造生命周期泛型参数列表。
ast::GenericParameters
AstBuilder::buildGenericParameters(RxParser::GenericParamsContext* context) {
    ast::GenericParameters parameters;
    // 有Context表示源码写过泛型，即使参数列表为空也要保留此信息。
    parameters.written = true;

    // 当前grammar只允许生命周期泛型参数；每项可带边界。
    for (auto* parameter : context->lifetimeParam()) {
        ast::LifetimeParameter lifetime;
        lifetime.name = parameter->lifetime()->getText();
        if (auto* bounds = parameter->lifetimeBounds()) {
            for (auto* bound : bounds->lifetime()) {
                lifetime.bounds.push_back(bound->getText());
            }
        }
        parameters.lifetimes.push_back(std::move(lifetime));
    }

    return parameters;
}

// 构造where子句，并区分生命周期约束与类型约束。
ast::WhereClause
AstBuilder::buildWhereClause(RxParser::WhereClauseContext* context) {
    ast::WhereClause clause;
    // 保存where是否显式出现；约束是否成立由语义阶段判断。
    clause.written = true;

    for (auto* item : context->whereClauseItem()) {
        ast::WherePredicate predicate;
        // 'a:...是生命周期约束，T:...是类型约束。
        if (item->lifetime() != nullptr) {
            predicate.subject_is_lifetime = true;
            predicate.lifetime_subject = item->lifetime()->getText();
            if (auto* bounds = item->lifetimeBounds()) {
                for (auto* bound : bounds->lifetime()) {
                    predicate.lifetime_bounds.push_back(bound->getText());
                }
            }
        } else {
            predicate.type_subject = buildTypeRef(item->typeRef());
            if (auto* bounds = item->typeParamBounds()) {
                for (auto* bound : bounds->lifetime()) {
                    predicate.lifetime_bounds.push_back(bound->getText());
                }
            }
        }
        clause.predicates.push_back(std::move(predicate));
    }
    return clause;
}

// 把selfParam的引用、mut和生命周期信息映射到Receiver。
ast::Receiver
AstBuilder::buildReceiver(RxParser::SelfParamContext* context) {
    ast::Receiver receiver;

    // &表示引用接收者；mut区分可变形式和值形式。
    if (context->AMP() != nullptr) {
        receiver.kind = context->MUT() != nullptr
            ? ast::ReceiverKind::MutableReference
            : ast::ReceiverKind::SharedReference;
    } else {
        receiver.kind = context->MUT() != nullptr
            ? ast::ReceiverKind::MutableValue
            : ast::ReceiverKind::Value;
    }
    // 保存可选生命周期，供后续语义检查使用。
    if (auto* lifetime = context->lifetime()) {
        receiver.lifetime = lifetime->getText();
    }

    return receiver;
}

// 构造普通参数的名称、mut绑定标记和类型。
ast::FunctionParameter
AstBuilder::buildFunctionParameter(RxParser::FunctionParamContext* context) {
    ast::FunctionParameter parameter;

    // identifierBinding的mut修饰绑定，不等同于参数类型是可变引用。
    parameter.name = context->identifierBinding()->identifier()->getText();
    parameter.mutable_binding = context->identifierBinding()->MUT() != nullptr;
    // 递归构造冒号后的类型；类型是否匹配留给语义阶段。
    parameter.type = buildTypeRef(context->typeRef());

    return parameter;
}






// 构造代码块；当前版本暂时只支持空块。
ast::Block 
AstBuilder::buildBlock(RxParser::BlockExpressionContext* context) {
    // 普通statement和尾表达式语义不同；当前遇到任意非空内容都会报错。
    if (!context->statement().empty() ||
        context->statementExpression() != nullptr) {
        throw std::logic_error("this first example only supports an empty body");
    }

    return ast::Block{};
}

// 根据statement分支构造具体节点，并保留分号信息。
std::unique_ptr<ast::Statement>
AstBuilder::buildStatement(RxParser::StatementContext* context) {
    // 按匹配到的子Context选择AST节点；具体子树交给对应构造函数。
    if (auto* let = context->letStatement()) {
        return buildLetStatement(let);
    }

    if (auto* expr = context->expressionWithBlock()) {
        auto node = std::make_unique<ast::ExpressionStatement>();
        node->expression = buildExpressionWithBlock(expr);
        // 此分支分号可选；保留分号状态以区分语句与块尾值。
        node->has_semicolon = context->SEMI() != nullptr;
        return node;
    }

    if (auto* expr = context->statementExpression()) {
        auto node = std::make_unique<ast::ExpressionStatement>();
        node->expression = buildStatementExpression(expr);
        // 此分支要求分号，因此标记固定为true。
        node->has_semicolon = true;
        return node;
    }

    if (context->SEMI()) {
        // 单独的分号是空语句，不创建空表达式。
        return std::make_unique<ast::EmptyStatement>();
    }

    throw std::logic_error("unknown statement form");
}

}  // namespace rx::frontend
