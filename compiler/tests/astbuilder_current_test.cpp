#include <cassert>
#include <iostream>
#include <memory>
#include <string>

#include "antlr4-runtime.h"
#include "RxLexer.h"
#include "Parser.h"

#include "rx/ast/expression.hpp"
#include "rx/ast/item.hpp"
#include "rx/ast/operator.hpp"
#include "rx/ast/program.hpp"
#include "rx/ast/statement.hpp"
#include "rx/frontend/astbuilder.hpp"

namespace {

template <typename T, typename U>
std::shared_ptr<T> requireNode(const std::shared_ptr<U>& node) {
    auto result = std::dynamic_pointer_cast<T>(node);
    assert(result != nullptr);
    return result;
}

std::shared_ptr<rx::ast::Program> buildProgram(const std::string& source) {
    antlr4::ANTLRInputStream input(source);
    RxLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    Parser parser(&tokens);

    auto* crate = parser.crate();
    assert(parser.getNumberOfSyntaxErrors() == 0);

    rx::frontend::AstBuilder builder;
    auto program = builder.build(crate);
    assert(program != nullptr);
    return program;
}

void testCurrentAstBuilder() {
    const std::string source = R"(
use a::{b, c};

struct Point {
    x: i32,
    y: i32,
}

const N: i32 = 3;

fn main() {
    let x = 1 + 2 * 3;
    let y = foo([1, 2][0]).bar()[1];
    let z = Point { x: 1, y: x + 2 };
}
)";

    auto program = buildProgram(source);
    assert(program->items.size() == 4);

    // 1. use a::{b, c};
    auto useItem = requireNode<rx::ast::UseItem>(program->items[0]);
    assert(useItem->tree != nullptr);
    assert(useItem->tree->form == rx::ast::UseTreeKind::Group);
    assert(useItem->tree->prefix.size() == 1);
    assert(useItem->tree->prefix[0] == "a");
    assert(useItem->tree->group_items.size() == 2);
    assert(useItem->tree->group_items[0]->prefix[0] == "b");
    assert(useItem->tree->group_items[1]->prefix[0] == "c");

    // 2. struct Point { x: i32, y: i32 }
    auto structure = requireNode<rx::ast::StructItem>(program->items[1]);
    assert(structure->name == "Point");
    assert(structure->fields.size() == 2);
    assert(structure->fields[0]->name == "x");
    assert(structure->fields[1]->name == "y");

    // 3. const N: i32 = 3;
    auto constant = requireNode<rx::ast::ConstantItem>(program->items[2]);
    assert(constant->name == "N");
    assert(constant->initializer != nullptr);
    assert(constant->initializer->kind == rx::ast::ConstValueKind::Integer);
    assert(constant->initializer->integer_spelling == "3");

    // 4. function body and ordinary expression precedence.
    auto function = requireNode<rx::ast::FunctionItem>(program->items[3]);
    assert(function->name == "main");
    assert(function->body != nullptr);
    assert(function->body->statements.size() == 3);
    assert(function->body->tail == nullptr);

    // let x = 1 + 2 * 3;
    auto letX = requireNode<rx::ast::LetStatement>(function->body->statements[0]);
    auto add = requireNode<rx::ast::BinaryExpr>(letX->initializer);
    assert(add->op == rx::ast::BinaryOperator::Add);

    auto one = requireNode<rx::ast::IntegerExpr>(add->left);
    assert(one->spelling == "1");

    auto multiply = requireNode<rx::ast::BinaryExpr>(add->right);
    assert(multiply->op == rx::ast::BinaryOperator::Multiply);
    assert(requireNode<rx::ast::IntegerExpr>(multiply->left)->spelling == "2");
    assert(requireNode<rx::ast::IntegerExpr>(multiply->right)->spelling == "3");

    // let y = foo([1, 2][0]).bar()[1];
    // Expected nesting: Index(MethodCall(Call(foo, Index(ArrayList, 0)), bar), 1)
    auto letY = requireNode<rx::ast::LetStatement>(function->body->statements[1]);
    auto outerIndex = requireNode<rx::ast::IndexExpr>(letY->initializer);
    assert(requireNode<rx::ast::IntegerExpr>(outerIndex->index)->spelling == "1");

    auto method = requireNode<rx::ast::MethodCallExpr>(outerIndex->base);
    assert(method->method != nullptr);
    assert(method->method->name == "bar");
    assert(method->arguments.empty());

    auto call = requireNode<rx::ast::CallExpr>(method->receiver);
    assert(call->arguments.size() == 1);
    auto callee = requireNode<rx::ast::PathExpr>(call->callee);
    assert(callee->path != nullptr);
    assert(callee->path->segments.size() == 1);
    assert(callee->path->segments[0]->name == "foo");

    auto innerIndex = requireNode<rx::ast::IndexExpr>(call->arguments[0]);
    assert(requireNode<rx::ast::IntegerExpr>(innerIndex->index)->spelling == "0");
    auto array = requireNode<rx::ast::ArrayListExpr>(innerIndex->base);
    assert(array->elements.size() == 2);
    assert(requireNode<rx::ast::IntegerExpr>(array->elements[0])->spelling == "1");
    assert(requireNode<rx::ast::IntegerExpr>(array->elements[1])->spelling == "2");

    // let z = Point { x: 1, y: x + 2 };
    auto letZ = requireNode<rx::ast::LetStatement>(function->body->statements[2]);
    auto init = requireNode<rx::ast::StructInitExpr>(letZ->initializer);
    assert(init->path != nullptr);
    assert(init->path->segments.size() == 1);
    assert(init->path->segments[0]->name == "Point");
    assert(init->fields.size() == 2);
    assert(init->fields[0]->name == "x");
    assert(init->fields[1]->name == "y");

    auto fieldYAdd = requireNode<rx::ast::BinaryExpr>(init->fields[1]->value);
    assert(fieldYAdd->op == rx::ast::BinaryOperator::Add);

    std::cout << "astbuilder_current_test passed\n";
}

}  // namespace

int main() {
    testCurrentAstBuilder();
    return 0;
}
