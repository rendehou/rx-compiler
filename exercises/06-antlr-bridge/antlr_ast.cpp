#include "antlr_ast.hpp"

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

template <typename Context>
Context* only(const std::vector<Context*>& contexts, const char* rule_name) {
    if (contexts.size() != 1) {
        throw std::runtime_error(
            std::string("unsupported operator in ") + rule_name +
            " in this example");
    }
    return contexts[0];
}

AstNode buildExpression(rx::RxParser::ExpressionContext* context);
AstNode buildAdditive(rx::RxParser::AdditiveExpressionContext* context);
AstNode buildMultiplicative(
    rx::RxParser::MultiplicativeExpressionContext* context);
AstNode buildCast(rx::RxParser::CastExpressionContext* context);
AstNode buildUnary(rx::RxParser::UnaryExpressionContext* context);
AstNode buildPostfix(rx::RxParser::PostfixExpressionContext* context);
AstNode buildPrimary(rx::RxParser::PrimaryExpressionContext* context);

rx::RxParser::AdditiveExpressionContext* findAdditive(
    rx::RxParser::ExpressionContext* context) {
    auto* assignment = context->assignmentExpression();
    if (assignment->assignmentOperator() != nullptr) {
        throw std::runtime_error("assignment is unsupported in this example");
    }

    auto* logical_or = assignment->logicalOrExpression();
    auto* logical_and = only(
        logical_or->logicalAndExpression(), "logicalOrExpression");
    auto* comparison = only(
        logical_and->comparisonExpression(), "logicalAndExpression");

    if (comparison->comparisonExceptLt() != nullptr ||
        comparison->closedBitOrExpression() != nullptr ||
        comparison->LT() != nullptr) {
        throw std::runtime_error("comparison is unsupported in this example");
    }

    auto* bit_or = only(comparison->bitOrExpression(), "comparisonExpression");
    auto* bit_xor = only(bit_or->bitXorExpression(), "bitOrExpression");
    auto* bit_and = only(bit_xor->bitAndExpression(), "bitXorExpression");
    auto* shift = only(bit_and->shiftExpression(), "bitAndExpression");

    if (!shift->closedAdditiveExpression().empty() ||
        !shift->SHL().empty() || !shift->shiftRight().empty()) {
        throw std::runtime_error("shift is unsupported in this example");
    }
    return only(shift->additiveExpression(), "shiftExpression");
}

AstNode buildExpression(rx::RxParser::ExpressionContext* context) {
    if (context == nullptr) {
        throw std::runtime_error("missing ExpressionContext");
    }
    return buildAdditive(findAdditive(context));
}

AstNode buildAdditive(rx::RxParser::AdditiveExpressionContext* context) {
    const auto operands = context->multiplicativeExpression();
    const auto operations = context->additiveOperator();
    if (operands.empty() || operands.size() != operations.size() + 1) {
        throw std::runtime_error("malformed AdditiveExpressionContext");
    }

    AstNode result = buildMultiplicative(operands[0]);
    for (std::size_t index = 0; index < operations.size(); ++index) {
        AstNode right = buildMultiplicative(operands[index + 1]);
        result = AstNode{
            AstKind::Binary,
            operations[index]->getText(),
            {std::move(result), std::move(right)},
        };
    }
    return result;
}

AstNode buildMultiplicative(
    rx::RxParser::MultiplicativeExpressionContext* context) {
    const auto operands = context->castExpression();
    const auto operations = context->multiplicativeOperator();
    if (operands.empty() || operands.size() != operations.size() + 1) {
        throw std::runtime_error("malformed MultiplicativeExpressionContext");
    }

    AstNode result = buildCast(operands[0]);
    for (std::size_t index = 0; index < operations.size(); ++index) {
        AstNode right = buildCast(operands[index + 1]);
        result = AstNode{
            AstKind::Binary,
            operations[index]->getText(),
            {std::move(result), std::move(right)},
        };
    }
    return result;
}

AstNode buildCast(rx::RxParser::CastExpressionContext* context) {
    if (!context->AS().empty() || !context->typeRef().empty()) {
        throw std::runtime_error("cast is unsupported in this example");
    }
    return buildUnary(context->unaryExpression());
}

AstNode buildUnary(rx::RxParser::UnaryExpressionContext* context) {
    if (context->unaryOperator() != nullptr || context->postfixExpression() == nullptr) {
        throw std::runtime_error("unary operator is unsupported in this example");
    }
    return buildPostfix(context->postfixExpression());
}

AstNode buildPostfix(rx::RxParser::PostfixExpressionContext* context) {
    if (!context->postfixSuffix().empty()) {
        throw std::runtime_error("postfix suffix is unsupported in this example");
    }
    return buildPrimary(context->primaryExpression());
}

AstNode buildPrimary(rx::RxParser::PrimaryExpressionContext* context) {
    if (context->expressionWithBlock() != nullptr ||
        context->nonBlockPrimary() == nullptr) {
        throw std::runtime_error("block expression is unsupported in this example");
    }

    auto* primary = context->nonBlockPrimary();
    if (auto* literal = primary->literalExpression()) {
        if (literal->INTEGER_LITERAL() != nullptr) {
            return AstNode{
                AstKind::Integer,
                literal->INTEGER_LITERAL()->getText(),
                {},
            };
        }
        if (literal->TRUE() != nullptr || literal->FALSE() != nullptr) {
            return AstNode{AstKind::Bool, literal->getText(), {}};
        }
    }

    if (primary->LPAREN() != nullptr && primary->expression() != nullptr &&
        primary->RPAREN() != nullptr) {
        return buildExpression(primary->expression());
    }

    throw std::runtime_error("primary expression is unsupported in this example");
}

}  // namespace

AstNode buildAst(rx::RxParser::ExpressionContext* context) {
    return buildExpression(context);
}

void printAst(const AstNode& node, int depth) {
    std::cout << std::string(static_cast<std::size_t>(depth * 2), ' ');
    switch (node.kind) {
        case AstKind::Integer:
            std::cout << "Integer";
            break;
        case AstKind::Bool:
            std::cout << "Bool";
            break;
        case AstKind::Binary:
            std::cout << "Binary";
            break;
    }
    std::cout << " \"" << node.value << "\"\n";

    for (const AstNode& child : node.children) {
        printAst(child, depth + 1);
    }
}
