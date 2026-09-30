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


ast::TypeSyntax
AstBuilder::buildTypeRef(RxParser::TypeRefContext* context) {
    if (context->LPAREN() != nullptr) {
        if (context->typeRef() != nullptr) {
            return buildTypeRef(context->typeRef());
        }
        ast::TypeSyntax unit;
        unit.kind = ast::TypeSyntaxKind::Unit;
        return unit;
    }

    if (context->typePath() != nullptr) {
        ast::TypeSyntax path;
        path.kind = ast::TypeSyntaxKind::Path;
        path.path = buildTypePath(context->typePath());
        return path;
    }

    if (context->referenceType() != nullptr) {
        return buildReferenceType(context->referenceType());
    }

    if (context->arrayType() != nullptr) {
        return buildArrayType(context->arrayType());
    }

    throw std::logic_error("unknown typeRef form");
}

ast::TypeSyntax
AstBuilder::buildReferenceType(RxParser::ReferenceTypeContext* context) {
    const bool double_reference = context->ANDAND() != nullptr;

    ast::TypeSyntax reference;
    reference.kind = ast::TypeSyntaxKind::Reference;
    reference.lifetime = context->lifetime() != nullptr
        ? context->lifetime()->getText() : "";
    reference.mutable_reference = context->MUT() != nullptr;
    reference.referent =
        std::make_unique<ast::TypeSyntax>(buildTypeRef(context->typeRef()));

    if (!double_reference) {
        return reference;
    }

    // ANDAND 构造两层引用，生命周期与 mut 属于内层。
    ast::TypeSyntax outer;
    outer.kind = ast::TypeSyntaxKind::Reference;
    outer.referent = std::make_unique<ast::TypeSyntax>(std::move(reference));
    return outer;
}

ast::TypeSyntax
AstBuilder::buildArrayType(RxParser::ArrayTypeContext* context) {
    ast::TypeSyntax array;
    array.kind = ast::TypeSyntaxKind::Array;
    array.array_element =
        std::make_unique<ast::TypeSyntax>(buildTypeRef(context->typeRef()));
    array.length =
        std::make_unique<ast::ConstValueSyntax>(buildConstValue(context->constValue()));
    return array;
}

ast::TypeSyntax
AstBuilder::buildClosedCastType(RxParser::ClosedCastTypeContext* context) {
    if (context->LPAREN() != nullptr) {
        if (context->typeRef() != nullptr) {
            return buildTypeRef(context->typeRef());
        }
        ast::TypeSyntax unit;
        unit.kind = ast::TypeSyntaxKind::Unit;
        return unit;
    }

    if (context->arrayType() != nullptr) {
        return buildArrayType(context->arrayType());
    }

    if (context->AMP() != nullptr || context->ANDAND() != nullptr) {
        const bool double_reference = context->ANDAND() != nullptr;

        ast::TypeSyntax reference;
        reference.kind = ast::TypeSyntaxKind::Reference;
        reference.lifetime = context->lifetime() != nullptr
            ? context->lifetime()->getText() : "";
        reference.mutable_reference = context->MUT() != nullptr;
        reference.referent = std::make_unique<ast::TypeSyntax>(
            buildClosedCastType(context->closedCastType()));

        if (!double_reference) {
            return reference;
        }

        ast::TypeSyntax outer;
        outer.kind = ast::TypeSyntaxKind::Reference;
        outer.referent = std::make_unique<ast::TypeSyntax>(std::move(reference));
        return outer;
    }

    // (typePathSegment PATHSEP)* pathIdentSegment PATHSEP? genericArgs
    ast::TypeSyntax path;
    path.kind = ast::TypeSyntaxKind::Path;
    for (auto* segment : context->typePathSegment()) {
        path.path.segments.push_back(buildTypePathSegment(segment));
    }
    ast::PathSegment last;
    last.name = context->pathIdentSegment()->getText();
    if (auto* generics = context->genericArgs()) {
        last.generic_arguments = buildGenericArgs(generics);
    }
    path.path.segments.push_back(std::move(last));
    return path;
}

ast::Path
AstBuilder::buildTypePath(RxParser::TypePathContext* context) {
    ast::Path path;
    for (auto* segment : context->typePathSegment()) {
        path.segments.push_back(buildTypePathSegment(segment));
    }
    return path;
}

ast::PathSegment
AstBuilder::buildTypePathSegment(RxParser::TypePathSegmentContext* context) {
    ast::PathSegment segment;
    segment.name = context->pathIdentSegment()->getText();
    if (auto* generics = context->genericArgs()) {
        segment.generic_arguments = buildGenericArgs(generics);
    }
    return segment;
}

std::vector<ast::GenericArgumentSyntax>
AstBuilder::buildGenericArgs(RxParser::GenericArgsContext* context) {
    std::vector<ast::GenericArgumentSyntax> arguments;

    for (auto* argument : context->genericArg()) {
        ast::GenericArgumentSyntax result;
        if (argument->lifetime() != nullptr) {
            result.kind = ast::GenericArgumentKind::Lifetime;
            result.lifetime = argument->lifetime()->getText();
        } else {
            result.kind = ast::GenericArgumentKind::Type;
            result.type = buildTypeRef(argument->typeRef());
        }
        arguments.push_back(std::move(result));
    }

    return arguments;
}

ast::ConstValueSyntax
AstBuilder::buildConstValue(RxParser::ConstValueContext* context) {
    if (context->LPAREN() != nullptr) {
        return buildConstValue(context->constValue());
    }

    if (context->INTEGER_LITERAL() != nullptr) {
        ast::ConstValueSyntax value;
        value.kind = ast::ConstValueKind::Integer;
        value.integer_spelling = context->INTEGER_LITERAL()->getText();
        return value;
    }

    if (context->TRUE() != nullptr || context->FALSE() != nullptr) {
        ast::ConstValueSyntax value;
        value.kind = ast::ConstValueKind::Boolean;
        value.boolean_value = context->TRUE() != nullptr;
        return value;
    }

    if (context->pathInExpression() != nullptr) {
        ast::ConstValueSyntax value;
        value.kind = ast::ConstValueKind::Path;
        value.path = buildPathInExpression(context->pathInExpression());
        return value;
    }

    if (context->MINUS() != nullptr) {
        ast::ConstValueSyntax value;
        value.kind = ast::ConstValueKind::Negate;
        value.operand = std::make_unique<ast::ConstValueSyntax>(
            buildMagnitude(context->magnitude()));
        return value;
    }

    throw std::logic_error("unknown constValue form");
}

ast::ConstValueSyntax
AstBuilder::buildMagnitude(RxParser::MagnitudeContext* context) {
    if (context->LPAREN() != nullptr) {
        return buildMagnitude(context->magnitude());
    }

    if (context->INTEGER_LITERAL() != nullptr) {
        ast::ConstValueSyntax value;
        value.kind = ast::ConstValueKind::Integer;
        value.integer_spelling = context->INTEGER_LITERAL()->getText();
        return value;
    }

    if (context->pathInExpression() != nullptr) {
        ast::ConstValueSyntax value;
        value.kind = ast::ConstValueKind::Path;
        value.path = buildPathInExpression(context->pathInExpression());
        return value;
    }

    throw std::logic_error("unknown magnitude form");
}

ast::Path
AstBuilder::buildPathInExpression(RxParser::PathInExpressionContext* context) {
    ast::Path path;
    for (auto* segment : context->pathExprSegment()) {
        path.segments.push_back(buildPathExprSegment(segment));
    }
    return path;
}

ast::PathSegment
AstBuilder::buildPathExprSegment(RxParser::PathExprSegmentContext* context) {
    ast::PathSegment segment;
    segment.name = context->pathIdentSegment()->getText();
    if (auto* generics = context->genericArgs()) {
        segment.generic_arguments = buildGenericArgs(generics);
    }
    return segment;
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

std::unique_ptr<ast::LetStatement>
AstBuilder::buildLetStatement(RxParser::LetStatementContext* context) {
    auto let = std::make_unique<ast::LetStatement>();

    let->name = context->identifierBinding()->identifier()->getText();
    let->mutable_binding = context->identifierBinding()->MUT() != nullptr;
    if (context->COLON() != nullptr) {
        let->declared_type = buildTypeRef(context->typeRef());
    }
    let->initializer = buildExpression(context->expression());

    return let;
}

std::unique_ptr<ast::Expr>
AstBuilder::buildLiteral(RxParser::LiteralExpressionContext* context) {
    if (context->INTEGER_LITERAL() != nullptr) {
        auto literal = std::make_unique<ast::IntegerExpr>();
        literal->spelling = context->INTEGER_LITERAL()->getText();
        return literal;
    }

    auto literal = std::make_unique<ast::BooleanExpr>();
    literal->value = context->TRUE() != nullptr;
    return literal;
}

std::unique_ptr<ast::Expr>
AstBuilder::buildArrayExpression(RxParser::ArrayExpressionContext* context) {
    if (auto* count = context->constValue()) {
        auto array = std::make_unique<ast::ArrayRepeatExpr>();
        array->element = buildExpression(context->expression()[0]);
        array->count = buildConstValue(count);
        return array;
    }

    auto array = std::make_unique<ast::ArrayListExpr>();
    for (auto* element : context->expression()) {
        array->elements.push_back(buildExpression(element));
    }
    return array;
}

std::unique_ptr<ast::Expr>
AstBuilder::applyPostfixSuffixes(
        std::unique_ptr<ast::Expr> base,
        const std::vector<RxParser::PostfixSuffixContext*>& suffixes) {
    for (auto* suffix : suffixes) {
        if (auto* call = suffix->callArguments()) {
            auto expression = std::make_unique<ast::CallExpr>();
            expression->callee = std::move(base);
            for (auto* argument : call->expression()) {
                expression->arguments.push_back(buildExpression(argument));
            }
            base = std::move(expression);
        } else if (suffix->LBRACKET() != nullptr) {
            auto expression = std::make_unique<ast::IndexExpr>();
            expression->base = std::move(base);
            expression->index = buildExpression(suffix->expression());
            base = std::move(expression);
        } else if (auto* dot = suffix->dotSuffix()) {
            base = applyDotSuffix(std::move(base), dot);
        } else {
            throw std::logic_error("unknown postfixSuffix form");
        }
    }

    return base;
}

std::unique_ptr<ast::Expr>
AstBuilder::applyDotSuffix(std::unique_ptr<ast::Expr> base,
                           RxParser::DotSuffixContext* suffix) {
    if (suffix->callArguments() != nullptr) {
        auto call = std::make_unique<ast::MethodCallExpr>();
        call->receiver = std::move(base);
        call->method = buildPathExprSegment(suffix->pathExprSegment());
        for (auto* argument : suffix->callArguments()->expression()) {
            call->arguments.push_back(buildExpression(argument));
        }
        return call;
    }

    auto access = std::make_unique<ast::FieldAccessExpr>();
    access->base = std::move(base);
    access->field = suffix->identifier()->getText();
    return access;
}

ast::AssignmentOperator
AstBuilder::mapAssignmentOperator(RxParser::AssignmentOperatorContext* context) const {
    if (context->equalsSign() != nullptr) return ast::AssignmentOperator::Assign;
    if (context->PLUS_ASSIGN() != nullptr) return ast::AssignmentOperator::Add;
    if (context->MINUS_ASSIGN() != nullptr) return ast::AssignmentOperator::Subtract;
    if (context->STAR_ASSIGN() != nullptr) return ast::AssignmentOperator::Multiply;
    if (context->SLASH_ASSIGN() != nullptr) return ast::AssignmentOperator::Divide;
    if (context->PERCENT_ASSIGN() != nullptr) return ast::AssignmentOperator::Remainder;
    if (context->AMP_ASSIGN() != nullptr) return ast::AssignmentOperator::BitAnd;
    if (context->PIPE_ASSIGN() != nullptr) return ast::AssignmentOperator::BitOr;
    if (context->CARET_ASSIGN() != nullptr) return ast::AssignmentOperator::BitXor;
    if (context->SHL_ASSIGN() != nullptr) return ast::AssignmentOperator::ShiftLeft;
    if (context->GT() != nullptr && context->GT_SECOND() != nullptr &&
        context->SHR_EQ() != nullptr) {
        return ast::AssignmentOperator::ShiftRight;
    }
    throw std::logic_error("unknown assignmentOperator form");
}

ast::UnaryOperator
AstBuilder::mapUnaryOperator(RxParser::UnaryOperatorContext* context) const {
    if (context->MINUS() != nullptr) return ast::UnaryOperator::Negate;
    if (context->NOT() != nullptr) return ast::UnaryOperator::Not;
    if (context->STAR() != nullptr) return ast::UnaryOperator::Dereference;
    if (context->AMP() != nullptr || context->ANDAND() != nullptr) {
        return context->MUT() != nullptr
            ? ast::UnaryOperator::BorrowMutable
            : ast::UnaryOperator::BorrowShared;
    }
    throw std::logic_error("unknown unaryOperator form");
}

std::optional<ast::BinaryOperator>
AstBuilder::mapBinaryOperator(antlr4::tree::ParseTree* child) const {
    if (auto* terminal = dynamic_cast<antlr4::tree::TerminalNode*>(child)) {
        switch (terminal->getSymbol()->getType()) {
        case RxParser::PIPE: return ast::BinaryOperator::BitOr;
        case RxParser::CARET: return ast::BinaryOperator::BitXor;
        case RxParser::AMP: return ast::BinaryOperator::BitAnd;
        case RxParser::OROR: return ast::BinaryOperator::LogicalOr;
        case RxParser::ANDAND: return ast::BinaryOperator::LogicalAnd;
        case RxParser::SHL: return ast::BinaryOperator::ShiftLeft;
        case RxParser::LT: return ast::BinaryOperator::Less;
        default: return std::nullopt;
        }
    }

    if (auto* additive = dynamic_cast<RxParser::AdditiveOperatorContext*>(child)) {
        if (additive->PLUS() != nullptr) return ast::BinaryOperator::Add;
        if (additive->MINUS() != nullptr) return ast::BinaryOperator::Subtract;
    }
    if (auto* multiplicative =
            dynamic_cast<RxParser::MultiplicativeOperatorContext*>(child)) {
        if (multiplicative->STAR() != nullptr) return ast::BinaryOperator::Multiply;
        if (multiplicative->SLASH() != nullptr) return ast::BinaryOperator::Divide;
        if (multiplicative->PERCENT() != nullptr) return ast::BinaryOperator::Remainder;
    }
    if (dynamic_cast<RxParser::ShiftRightContext*>(child) != nullptr) {
        return ast::BinaryOperator::ShiftRight;
    }
    if (auto* comparison =
            dynamic_cast<RxParser::ComparisonExceptLtContext*>(child)) {
        if (comparison->EQEQ() != nullptr) return ast::BinaryOperator::Equal;
        if (comparison->NE() != nullptr) return ast::BinaryOperator::NotEqual;
        if (comparison->LE() != nullptr) return ast::BinaryOperator::LessEqual;
        if (comparison->GT() != nullptr) {
            return comparison->GE_EQ() != nullptr
                       ? ast::BinaryOperator::GreaterEqual
                       : ast::BinaryOperator::Greater;
        }
        if (comparison->GT_SECOND() != nullptr) {
            return comparison->SHR_EQ() != nullptr
                       ? ast::BinaryOperator::GreaterEqual
                       : ast::BinaryOperator::Greater;
        }
        if (comparison->genericClose() != nullptr) {
            return ast::BinaryOperator::Greater;
        }
    }

    return std::nullopt;
}

template <typename OperandBuilder>
std::unique_ptr<ast::Expr>
AstBuilder::buildBinaryChain(antlr4::tree::ParseTree* context,
                             OperandBuilder&& operandBuilder) {
    std::unique_ptr<ast::Expr> result;
    std::optional<ast::BinaryOperator> pending;

    for (auto* child : context->children) {
        if (const auto op = mapBinaryOperator(child)) {
            if (pending) {
                throw std::logic_error("malformed binary chain: two operators in a row");
            }
            pending = *op;
            continue;
        }

        auto operand = operandBuilder(child);
        if (operand == nullptr) {
            throw std::logic_error("malformed binary chain: unrecognized child");
        }

        if (pending) {
            if (result == nullptr) {
                throw std::logic_error("malformed binary chain: leading operator");
            }
            auto binary = std::make_unique<ast::BinaryExpr>();
            binary->op = *pending;
            binary->left = std::move(result);
            binary->right = std::move(operand);
            result = std::move(binary);
            pending.reset();
        } else if (result == nullptr) {
            result = std::move(operand);
        } else {
            throw std::logic_error("malformed binary chain: two operands in a row");
        }
    }

    if (pending || result == nullptr) {
        throw std::logic_error("malformed binary chain: missing operand");
    }

    return result;
}

// 普通expression从assignmentExpression开始；其余运算层级由子函数递归构造。
std::unique_ptr<ast::Expr>
AstBuilder::buildExpression(RxParser::ExpressionContext* context) {
    return buildAssignmentExpression(context->assignmentExpression());
}

// 赋值右侧递归解析，因此a=b=c构造成a=(b=c)。
std::unique_ptr<ast::Expr>
AstBuilder::buildAssignmentExpression(RxParser::AssignmentExpressionContext* context) {
    auto place = buildLogicalOrExpression(context->logicalOrExpression());
    if (auto* operation = context->assignmentOperator()) {
        auto assignment = std::make_unique<ast::AssignmentExpr>();
        assignment->op = mapAssignmentOperator(operation);
        assignment->place = std::move(place);
        assignment->value = buildExpression(context->expression());
        return assignment;
    }
    return place;
}

// 同优先级的||按源码顺序左折叠成BinaryExpr。
std::unique_ptr<ast::Expr>
AstBuilder::buildLogicalOrExpression(RxParser::LogicalOrExpressionContext* context) {
    return buildBinaryChain(context, [this](antlr4::tree::ParseTree* child)
        -> std::unique_ptr<ast::Expr> {
        if (auto* operand = dynamic_cast<RxParser::LogicalAndExpressionContext*>(child)) {
            return buildLogicalAndExpression(operand);
        }
        return nullptr;
    });
}

// 同优先级的&&按源码顺序左折叠；这里是逻辑与，不是前缀双重借用。
std::unique_ptr<ast::Expr>
AstBuilder::buildLogicalAndExpression(RxParser::LogicalAndExpressionContext* context) {
    return buildBinaryChain(context, [this](antlr4::tree::ParseTree* child)
        -> std::unique_ptr<ast::Expr> {
        if (auto* operand = dynamic_cast<RxParser::ComparisonExpressionContext*>(child)) {
            return buildComparisonExpression(operand);
        }
        return nullptr;
    });
}

// 比较表达式最多有一个比较符；closedBitOr分支专门处理与泛型尖括号相邻的<。
std::unique_ptr<ast::Expr>
AstBuilder::buildComparisonExpression(RxParser::ComparisonExpressionContext* context) {
    return buildBinaryChain(context, [this](antlr4::tree::ParseTree* child)
        -> std::unique_ptr<ast::Expr> {
        if (auto* operand = dynamic_cast<RxParser::BitOrExpressionContext*>(child)) {
            return buildBitOrExpression(operand);
        }
        if (auto* operand = dynamic_cast<RxParser::ClosedBitOrExpressionContext*>(child)) {
            return buildClosedBitOrExpression(operand);
        }
        return nullptr;
    });
}

// 位或层把每个bitXor操作数按源码顺序组合。
std::unique_ptr<ast::Expr>
AstBuilder::buildBitOrExpression(RxParser::BitOrExpressionContext* context) {
    return buildBinaryChain(context, [this](antlr4::tree::ParseTree* child)
        -> std::unique_ptr<ast::Expr> {
        if (auto* operand = dynamic_cast<RxParser::BitXorExpressionContext*>(child)) {
            return buildBitXorExpression(operand);
        }
        return nullptr;
    });
}

// closed变体与普通位或产生相同AST，只限制最后一个操作数的语法形态。
std::unique_ptr<ast::Expr>
AstBuilder::buildClosedBitOrExpression(RxParser::ClosedBitOrExpressionContext* context) {
    return buildBinaryChain(context, [this](antlr4::tree::ParseTree* child)
        -> std::unique_ptr<ast::Expr> {
        if (auto* operand = dynamic_cast<RxParser::BitXorExpressionContext*>(child)) {
            return buildBitXorExpression(operand);
        }
        if (auto* operand = dynamic_cast<RxParser::ClosedBitXorExpressionContext*>(child)) {
            return buildClosedBitXorExpression(operand);
        }
        return nullptr;
    });
}

// 位异或层按源码顺序左折叠。
std::unique_ptr<ast::Expr>
AstBuilder::buildBitXorExpression(RxParser::BitXorExpressionContext* context) {
    return buildBinaryChain(context, [this](antlr4::tree::ParseTree* child)
        -> std::unique_ptr<ast::Expr> {
        if (auto* operand = dynamic_cast<RxParser::BitAndExpressionContext*>(child)) {
            return buildBitAndExpression(operand);
        }
        return nullptr;
    });
}

// closed异或只改变语法边界，不改变AST中的运算优先级。
std::unique_ptr<ast::Expr>
AstBuilder::buildClosedBitXorExpression(RxParser::ClosedBitXorExpressionContext* context) {
    return buildBinaryChain(context, [this](antlr4::tree::ParseTree* child)
        -> std::unique_ptr<ast::Expr> {
        if (auto* operand = dynamic_cast<RxParser::BitAndExpressionContext*>(child)) {
            return buildBitAndExpression(operand);
        }
        if (auto* operand = dynamic_cast<RxParser::ClosedBitAndExpressionContext*>(child)) {
            return buildClosedBitAndExpression(operand);
        }
        return nullptr;
    });
}

// 位与层的操作数来自shiftExpression。
std::unique_ptr<ast::Expr>
AstBuilder::buildBitAndExpression(RxParser::BitAndExpressionContext* context) {
    return buildBinaryChain(context, [this](antlr4::tree::ParseTree* child)
        -> std::unique_ptr<ast::Expr> {
        if (auto* operand = dynamic_cast<RxParser::ShiftExpressionContext*>(child)) {
            return buildShiftExpression(operand);
        }
        return nullptr;
    });
}

// closed位与的末尾使用closedShift，折叠出的AST仍是普通BinaryExpr。
std::unique_ptr<ast::Expr>
AstBuilder::buildClosedBitAndExpression(RxParser::ClosedBitAndExpressionContext* context) {
    return buildBinaryChain(context, [this](antlr4::tree::ParseTree* child)
        -> std::unique_ptr<ast::Expr> {
        if (auto* operand = dynamic_cast<RxParser::ShiftExpressionContext*>(child)) {
            return buildShiftExpression(operand);
        }
        if (auto* operand = dynamic_cast<RxParser::ClosedShiftExpressionContext*>(child)) {
            return buildClosedShiftExpression(operand);
        }
        return nullptr;
    });
}

// 左移和右移共享优先级；closedAdditive只用于处理尖括号/移位符的文法歧义。
std::unique_ptr<ast::Expr>
AstBuilder::buildShiftExpression(RxParser::ShiftExpressionContext* context) {
    return buildBinaryChain(context, [this](antlr4::tree::ParseTree* child)
        -> std::unique_ptr<ast::Expr> {
        if (auto* operand = dynamic_cast<RxParser::AdditiveExpressionContext*>(child)) {
            return buildAdditiveExpression(operand);
        }
        if (auto* operand = dynamic_cast<RxParser::ClosedAdditiveExpressionContext*>(child)) {
            return buildClosedAdditiveExpression(operand);
        }
        return nullptr;
    });
}

// closed移位表达式要求最后一个加法操作数也采用closed形式。
std::unique_ptr<ast::Expr>
AstBuilder::buildClosedShiftExpression(RxParser::ClosedShiftExpressionContext* context) {
    return buildBinaryChain(context, [this](antlr4::tree::ParseTree* child)
        -> std::unique_ptr<ast::Expr> {
        if (auto* operand = dynamic_cast<RxParser::AdditiveExpressionContext*>(child)) {
            return buildAdditiveExpression(operand);
        }
        if (auto* operand = dynamic_cast<RxParser::ClosedAdditiveExpressionContext*>(child)) {
            return buildClosedAdditiveExpression(operand);
        }
        return nullptr;
    });
}

// 加减运算同优先级并左结合。
std::unique_ptr<ast::Expr>
AstBuilder::buildAdditiveExpression(RxParser::AdditiveExpressionContext* context) {
    return buildBinaryChain(context, [this](antlr4::tree::ParseTree* child)
        -> std::unique_ptr<ast::Expr> {
        if (auto* operand = dynamic_cast<RxParser::MultiplicativeExpressionContext*>(child)) {
            return buildMultiplicativeExpression(operand);
        }
        return nullptr;
    });
}

// closed加减表达式以closedMultiplicative作为最后一个操作数。
std::unique_ptr<ast::Expr>
AstBuilder::buildClosedAdditiveExpression(RxParser::ClosedAdditiveExpressionContext* context) {
    return buildBinaryChain(context, [this](antlr4::tree::ParseTree* child)
        -> std::unique_ptr<ast::Expr> {
        if (auto* operand = dynamic_cast<RxParser::MultiplicativeExpressionContext*>(child)) {
            return buildMultiplicativeExpression(operand);
        }
        if (auto* operand = dynamic_cast<RxParser::ClosedMultiplicativeExpressionContext*>(child)) {
            return buildClosedMultiplicativeExpression(operand);
        }
        return nullptr;
    });
}

// 乘除和取余同优先级并左结合。
std::unique_ptr<ast::Expr>
AstBuilder::buildMultiplicativeExpression(RxParser::MultiplicativeExpressionContext* context) {
    return buildBinaryChain(context, [this](antlr4::tree::ParseTree* child)
        -> std::unique_ptr<ast::Expr> {
        if (auto* operand = dynamic_cast<RxParser::CastExpressionContext*>(child)) {
            return buildCastExpression(operand);
        }
        return nullptr;
    });
}

// closed乘法表达式以closedCast作为最后一个操作数。
std::unique_ptr<ast::Expr>
AstBuilder::buildClosedMultiplicativeExpression(RxParser::ClosedMultiplicativeExpressionContext* context) {
    return buildBinaryChain(context, [this](antlr4::tree::ParseTree* child)
        -> std::unique_ptr<ast::Expr> {
        if (auto* operand = dynamic_cast<RxParser::CastExpressionContext*>(child)) {
            return buildCastExpression(operand);
        }
        if (auto* operand = dynamic_cast<RxParser::ClosedCastExpressionContext*>(child)) {
            return buildClosedCastExpression(operand);
        }
        return nullptr;
    });
}

// as转换可以连续出现；逐个包裹形成左结合的CastExpr链。
std::unique_ptr<ast::Expr>
AstBuilder::buildCastExpression(RxParser::CastExpressionContext* context) {
    auto result = buildUnaryExpression(context->unaryExpression());
    for (auto* target : context->typeRef()) {
        auto cast = std::make_unique<ast::CastExpr>();
        cast->value = std::move(result);
        cast->target_type = buildTypeRef(target);
        result = std::move(cast);
    }
    return result;
}

// closedCast仅限制最后一个as类型；最终AST仍是普通类型转换节点。
std::unique_ptr<ast::Expr>
AstBuilder::buildClosedCastExpression(RxParser::ClosedCastExpressionContext* context) {
    if (context->unaryExpression() != nullptr) {
        return buildUnaryExpression(context->unaryExpression());
    }

    auto cast = std::make_unique<ast::CastExpr>();
    cast->value = buildCastExpression(context->castExpression());
    cast->target_type = buildClosedCastType(context->closedCastType());
    return cast;
}

// 前缀运算递归构造；词法token&&代表两层借用，mut属于靠近操作数的内层。
std::unique_ptr<ast::Expr>
AstBuilder::buildUnaryExpression(RxParser::UnaryExpressionContext* context) {
    if (auto* operation = context->unaryOperator()) {
        auto inner = std::make_unique<ast::UnaryExpr>();
        inner->op = mapUnaryOperator(operation);
        inner->operand = buildUnaryExpression(context->unaryExpression());
        if (operation->ANDAND() == nullptr) {
            return inner;
        }

        auto outer = std::make_unique<ast::UnaryExpr>();
        outer->op = ast::UnaryOperator::BorrowShared;
        outer->operand = std::move(inner);
        return outer;
    }
    return buildPostfixExpression(context->postfixExpression());
}

// 后缀链从主表达式开始，按源码顺序依次叠加调用、下标、字段或方法访问。
std::unique_ptr<ast::Expr>
AstBuilder::buildPostfixExpression(RxParser::PostfixExpressionContext* context) {
    auto base = buildPrimaryExpression(context->primaryExpression());
    return applyPostfixSuffixes(std::move(base), context->postfixSuffix());
}

// 主表达式在普通原子表达式与块表达式之间二选一。
std::unique_ptr<ast::Expr>
AstBuilder::buildPrimaryExpression(RxParser::PrimaryExpressionContext* context) {
    if (auto* primary = context->nonBlockPrimary()) {
        return buildNonBlockPrimary(primary);
    }
    if (auto* expression = context->expressionWithBlock()) {
        return buildExpressionWithBlock(expression);
    }
    throw std::logic_error("unknown primaryExpression form");
}

// 构造字面量、路径/结构体初始化、括号、数组和控制转移等非块主表达式。
std::unique_ptr<ast::Expr>
AstBuilder::buildNonBlockPrimary(RxParser::NonBlockPrimaryContext* context) {
    if (auto* literal = context->literalExpression()) {
        return buildLiteral(literal);
    }

    if (auto* path = context->pathInExpression()) {
        if (context->LBRACE() == nullptr) {
            auto expression = std::make_unique<ast::PathExpr>();
            expression->path = buildPathInExpression(path);
            return expression;
        }

        auto expression = std::make_unique<ast::StructInitExpr>();
        expression->path = buildPathInExpression(path);
        if (auto* fields = context->structExprFields()) {
            for (auto* field : fields->structExprField()) {
                ast::StructInitializerField initializer;
                initializer.name = field->identifier()->getText();
                initializer.value = buildExpression(field->expression());
                expression->fields.push_back(std::move(initializer));
            }
        }
        return expression;
    }

    if (context->LPAREN() != nullptr) {
        if (auto* inner = context->expression()) {
            return buildExpression(inner);
        }
        return std::make_unique<ast::UnitExpr>();
    }

    if (auto* array = context->arrayExpression()) {
        return buildArrayExpression(array);
    }

    if (context->BREAK() != nullptr) {
        auto expression = std::make_unique<ast::BreakExpr>();
        if (auto* value = context->expression()) {
            expression->value = buildExpression(value);
        }
        return expression;
    }

    if (context->RETURN() != nullptr) {
        auto expression = std::make_unique<ast::ReturnExpr>();
        if (auto* value = context->expression()) {
            expression->value = buildExpression(value);
        }
        return expression;
    }

    if (context->CONTINUE() != nullptr) {
        return std::make_unique<ast::ContinueExpr>();
    }

    throw std::logic_error("unknown nonBlockPrimary form");
}

// 普通expressionWithBlock分发到块、if、loop或while节点。
std::unique_ptr<ast::Expr>
AstBuilder::buildExpressionWithBlock(RxParser::ExpressionWithBlockContext* context) {
    if (auto* conditional = context->ifExpression()) {
        return buildIfExpression(conditional);
    }
    if (context->LOOP() != nullptr) {
        auto expression = std::make_unique<ast::LoopExpr>();
        expression->body = buildBlock(context->blockExpression());
        return expression;
    }
    if (context->WHILE() != nullptr) {
        auto expression = std::make_unique<ast::WhileExpr>();
        expression->condition = buildConditionExpression(context->conditionExpression());
        expression->body = buildBlock(context->blockExpression());
        return expression;
    }
    if (auto* block = context->blockExpression()) {
        auto expression = std::make_unique<ast::BlockExpr>();
        expression->block = buildBlock(block);
        return expression;
    }
    throw std::logic_error("unknown expressionWithBlock form");
}

// if的条件交给condition构造器；then与可选else块分别保存在AST中。
std::unique_ptr<ast::Expr>
AstBuilder::buildIfExpression(RxParser::IfExpressionContext* context) {
    const auto blocks = context->blockExpression();
    if (blocks.empty()) {
        throw std::logic_error("ifExpression is missing its then block");
    }

    auto expression = std::make_unique<ast::IfExpr>();
    expression->condition = buildConditionExpression(context->conditionExpression());
    expression->then_block = buildBlock(blocks.front());

    if (auto* else_if = context->ifExpression()) {
        expression->else_branch = buildIfExpression(else_if);
    } else if (blocks.size() > 1) {
        auto else_block = std::make_unique<ast::BlockExpr>();
        else_block->block = buildBlock(blocks[1]);
        expression->else_branch = std::move(else_block);
    }

    return expression;
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
