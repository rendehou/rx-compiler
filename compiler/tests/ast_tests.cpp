#include "rx/ast/expression.hpp"
#include "rx/ast/item.hpp"
#include "rx/ast/program.hpp"
#include "rx/ast/statement.hpp"
#include "rx/ast/type_syntax.hpp"
#include "rx/frontend/ast_builder.hpp"
#include "rx/frontend/parser.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <type_traits>

namespace {

int failures = 0;

void check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

template <class Node>
void checkCategory(const rx::ast::NodeKind expected, const char* message) {
    const Node node{};
    check(node.category == expected, message);
}

void testConcreteNodeCategories() {
    using namespace rx::ast;

#define CHECK_CATEGORY(Type, Kind) checkCategory<Type>(NodeKind::Kind, #Type " category")
    CHECK_CATEGORY(Program, Program);
    CHECK_CATEGORY(UseItem, UseItem);
    CHECK_CATEGORY(FunctionItem, FunctionItem);
    CHECK_CATEGORY(StructItem, StructItem);
    CHECK_CATEGORY(ConstantItem, ConstantItem);
    CHECK_CATEGORY(ImplItem, ImplItem);
    CHECK_CATEGORY(UseTree, UseTree);
    CHECK_CATEGORY(LifetimeParameter, LifetimeParameter);
    CHECK_CATEGORY(GenericParameters, GenericParameters);
    CHECK_CATEGORY(WherePredicate, WherePredicate);
    CHECK_CATEGORY(WhereClause, WhereClause);
    CHECK_CATEGORY(FunctionParameter, FunctionParameter);
    CHECK_CATEGORY(Receiver, Receiver);
    CHECK_CATEGORY(StructField, StructField);
    CHECK_CATEGORY(DeriveAttribute, DeriveAttribute);
    CHECK_CATEGORY(AssociatedItem, AssociatedItem);
    CHECK_CATEGORY(Block, Block);
    CHECK_CATEGORY(EmptyStatement, EmptyStatement);
    CHECK_CATEGORY(LetStatement, LetStatement);
    CHECK_CATEGORY(ExpressionStatement, ExpressionStatement);
    CHECK_CATEGORY(IntegerExpr, IntegerExpr);
    CHECK_CATEGORY(BooleanExpr, BooleanExpr);
    CHECK_CATEGORY(UnitExpr, UnitExpr);
    CHECK_CATEGORY(PathExpr, PathExpr);
    CHECK_CATEGORY(BlockExpr, BlockExpr);
    CHECK_CATEGORY(IfExpr, IfExpr);
    CHECK_CATEGORY(WhileExpr, WhileExpr);
    CHECK_CATEGORY(LoopExpr, LoopExpr);
    CHECK_CATEGORY(BreakExpr, BreakExpr);
    CHECK_CATEGORY(ContinueExpr, ContinueExpr);
    CHECK_CATEGORY(ReturnExpr, ReturnExpr);
    CHECK_CATEGORY(UnaryExpr, UnaryExpr);
    CHECK_CATEGORY(BinaryExpr, BinaryExpr);
    CHECK_CATEGORY(AssignmentExpr, AssignmentExpr);
    CHECK_CATEGORY(CastExpr, CastExpr);
    CHECK_CATEGORY(CallExpr, CallExpr);
    CHECK_CATEGORY(MethodCallExpr, MethodCallExpr);
    CHECK_CATEGORY(FieldAccessExpr, FieldAccessExpr);
    CHECK_CATEGORY(IndexExpr, IndexExpr);
    CHECK_CATEGORY(ArrayListExpr, ArrayListExpr);
    CHECK_CATEGORY(ArrayRepeatExpr, ArrayRepeatExpr);
    CHECK_CATEGORY(StructInitExpr, StructInitExpr);
    CHECK_CATEGORY(StructInitializerField, StructInitializerField);
    CHECK_CATEGORY(Path, Path);
    CHECK_CATEGORY(PathSegment, PathSegment);
    CHECK_CATEGORY(ConstValueSyntax, ConstValueSyntax);
    CHECK_CATEGORY(TypeSyntax, TypeSyntax);
    CHECK_CATEGORY(GenericArgumentSyntax, GenericArgumentSyntax);
#undef CHECK_CATEGORY

    static_assert(std::is_base_of_v<AstNode, Item>);
    static_assert(std::is_base_of_v<Item, FunctionItem>);
    static_assert(std::is_base_of_v<Item, StructItem>);
    static_assert(std::is_base_of_v<Item, ConstantItem>);
    static_assert(std::is_base_of_v<Item, ImplItem>);
    static_assert(std::is_base_of_v<Item, UseItem>);
    static_assert(std::is_base_of_v<Statement, LetStatement>);
    static_assert(std::is_base_of_v<Statement, ExpressionStatement>);
    static_assert(std::is_base_of_v<Expr, BinaryExpr>);
    static_assert(std::is_base_of_v<Expr, IfExpr>);
}

void testOwnedChildrenAndExpressionShape() {
    using namespace rx::ast;

    Program program;
    auto function = std::make_unique<FunctionItem>();
    function->name = "main";
    function->body.statements.push_back(std::make_unique<EmptyStatement>());

    auto sum = std::make_unique<BinaryExpr>();
    sum->op = BinaryOperator::Add;
    auto left = std::make_unique<IntegerExpr>();
    left->spelling = "1";
    auto right = std::make_unique<IntegerExpr>();
    right->spelling = "2";
    const auto* left_pointer = left.get();
    const auto* right_pointer = right.get();
    sum->left = std::move(left);
    sum->right = std::move(right);
    const auto* sum_pointer = sum.get();
    function->body.tail = std::move(sum);

    auto* function_pointer = function.get();
    program.items.push_back(std::move(function));

    const auto program_children = program.children();
    check(program_children.size() == 1, "Program exposes its one top-level item");
    check(program_children.front() == function_pointer,
          "Program child points to the owned FunctionItem");

    const auto function_children = function_pointer->children();
    check(function_children.size() == 3,
          "FunctionItem exposes generic parameters, where clause, and body when optionals are absent");
    check(function_children[2] == &function_pointer->body,
          "FunctionItem exposes its Block as a direct child");

    const auto block_children = function_pointer->body.children();
    check(block_children.size() == 2, "Block exposes statement(s) followed by its tail value");
    check(block_children[0] == function_pointer->body.statements.front().get(),
          "Block exposes its owned statement");
    check(block_children[1] == sum_pointer, "Block exposes its owned tail expression");

    const auto sum_children = sum_pointer->children();
    check(sum_children.size() == 2, "BinaryExpr exposes exactly its two operands");
    check(sum_children[0] == left_pointer && sum_children[1] == right_pointer,
          "BinaryExpr children preserve left-to-right operand order");
    check(dynamic_cast<const Item*>(program_children.front()) != nullptr,
          "top-level item is accessible polymorphically through Item");
    check(dynamic_cast<const Expr*>(sum_pointer) != nullptr,
          "binary expression is accessible polymorphically through Expr");
}

void testTypeAndRecursiveUseChildren() {
    using namespace rx::ast;

    TypeSyntax array_type;
    array_type.kind = TypeSyntaxKind::Array;
    array_type.array_element = std::make_unique<TypeSyntax>();
    array_type.array_element->kind = TypeSyntaxKind::Unit;
    array_type.length = std::make_unique<ConstValueSyntax>();
    array_type.length->kind = ConstValueKind::Integer;
    array_type.length->integer_spelling = "4";

    const auto type_children = array_type.children();
    check(type_children.size() == 2, "array TypeSyntax exposes element type and length");
    check(type_children[0] == array_type.array_element.get(),
          "array type exposes its element type first");
    check(type_children[1] == array_type.length.get(),
          "array type exposes its length second");

    UseItem use_item;
    use_item.tree.form = UseTreeKind::Group;
    use_item.tree.group_items.emplace_back();
    use_item.tree.group_items.back().form = UseTreeKind::Path;
    use_item.tree.group_items.back().prefix.push_back("io");
    const auto use_children = use_item.children();
    check(use_children.size() == 1 && use_children.front() == &use_item.tree,
          "UseItem exposes its recursive UseTree");
    const auto group_children = use_item.tree.children();
    check(group_children.size() == 1 &&
              group_children.front() == &use_item.tree.group_items.front(),
          "group UseTree exposes its nested import tree");
}

void testParserToAstSmokeCase() {
    using namespace rx;

    auto parsed = frontend::ParsedSource::parse("fn main() {}\n");
    check(parsed->ok(), "ANTLR accepts the empty-main smoke input");
    if (!parsed->ok()) return;

    frontend::AstBuilder builder;
    auto program = builder.build(parsed->tree());
    check(program->category == ast::NodeKind::Program,
          "AstBuilder returns a Program root");
    check(program->items.size() == 1,
          "AstBuilder retains the source function as a top-level item");
    if (program->items.size() != 1) return;

    const auto* function = dynamic_cast<const ast::FunctionItem*>(program->items[0].get());
    check(function != nullptr, "AstBuilder creates a FunctionItem for fn");
    if (function == nullptr) return;
    check(function->name == "main", "AstBuilder retains the function name");
    check(function->body.statements.empty() && function->body.tail == nullptr,
          "AstBuilder represents the empty function body as an empty Block");
}

void testOrdinaryExpressionPrecedenceAndBlockContents() {
    using namespace rx;

    auto parsed = frontend::ParsedSource::parse(
        "fn main() { let x = 1 + 2 * 3; x }\n");
    check(parsed->ok(), "ANTLR accepts ordinary expressions in a nonempty block");
    if (!parsed->ok()) return;

    frontend::AstBuilder builder;
    auto program = builder.build(parsed->tree());
    auto* function = dynamic_cast<ast::FunctionItem*>(program->items.front().get());
    check(function != nullptr, "ordinary-expression program has a function root");
    if (function == nullptr) return;

    check(function->body.statements.size() == 1,
          "block keeps its let statement in source order");
    check(function->body.tail != nullptr,
          "block stores the final no-semicolon expression as its tail");
    if (function->body.statements.size() != 1 || function->body.tail == nullptr) return;

    auto* let = dynamic_cast<ast::LetStatement*>(function->body.statements.front().get());
    check(let != nullptr, "let syntax becomes a LetStatement");
    if (let == nullptr) return;

    auto* add = dynamic_cast<ast::BinaryExpr*>(let->initializer.get());
    check(add != nullptr && add->op == ast::BinaryOperator::Add,
          "addition is the root of 1 + 2 * 3");
    if (add == nullptr) return;
    auto* multiply = dynamic_cast<ast::BinaryExpr*>(add->right.get());
    check(multiply != nullptr && multiply->op == ast::BinaryOperator::Multiply,
          "multiplication remains nested on the right by precedence");
    check(dynamic_cast<ast::PathExpr*>(function->body.tail.get()) != nullptr,
          "the block tail x becomes a PathExpr");
}

void testConditionBreakAndStatementExpressions() {
    using namespace rx;

    auto parsed = frontend::ParsedSource::parse(
        "fn main() { if break 1 + 2 << 3 >> 4 { true } else { false }; "
        "while x << 1 >> 2 < 4 { x += 1; } x << 1 >> 2; }\n");
    check(parsed->ok(), "ANTLR accepts condition, break, and statement expression forms");
    if (!parsed->ok()) return;

    frontend::AstBuilder builder;
    auto program = builder.build(parsed->tree());
    auto* function = dynamic_cast<ast::FunctionItem*>(program->items.front().get());
    check(function != nullptr, "condition program has a function root");
    if (function == nullptr) return;

    check(function->body.statements.size() == 3,
          "if, while, and a statement expression are retained in the block");
    if (function->body.statements.size() != 3) return;

    auto* if_statement =
        dynamic_cast<ast::ExpressionStatement*>(function->body.statements[0].get());
    auto* if_expression = if_statement == nullptr
        ? nullptr : dynamic_cast<ast::IfExpr*>(if_statement->expression.get());
    check(if_expression != nullptr, "if statement contains an IfExpr");
    if (if_expression != nullptr) {
        auto* shift_right =
            dynamic_cast<ast::BinaryExpr*>(if_expression->condition.get());
        check(shift_right != nullptr &&
                  shift_right->op == ast::BinaryOperator::ShiftRight,
              "if condition preserves its final right shift");
        if (shift_right != nullptr) {
            auto* shift_left = dynamic_cast<ast::BinaryExpr*>(shift_right->left.get());
            check(shift_left != nullptr &&
                      shift_left->op == ast::BinaryOperator::ShiftLeft,
                  "if condition shift chain folds from left to right");
            if (shift_left != nullptr) {
                auto* add = dynamic_cast<ast::BinaryExpr*>(shift_left->left.get());
                check(add != nullptr && add->op == ast::BinaryOperator::Add,
                      "if condition preserves additive precedence below shifts");
                if (add != nullptr) {
                    auto* break_expression =
                        dynamic_cast<ast::BreakExpr*>(add->left.get());
                    check(break_expression != nullptr,
                          "condition-leading break becomes a BreakExpr operand");
                    if (break_expression != nullptr) {
                        auto* value = dynamic_cast<ast::IntegerExpr*>(
                            break_expression->value.get());
                        check(value != nullptr && value->spelling == "1",
                              "break operand is built by conditionBreak rules");
                    }
                }
            }
        }
        check(if_expression->then_block.tail != nullptr,
              "if then-block retains its tail expression");
        check(if_expression->else_branch != nullptr,
              "if else-block becomes an AST branch");
    }

    auto* while_statement =
        dynamic_cast<ast::ExpressionStatement*>(function->body.statements[1].get());
    auto* while_expression = while_statement == nullptr
        ? nullptr : dynamic_cast<ast::WhileExpr*>(while_statement->expression.get());
    check(while_expression != nullptr, "while statement contains a WhileExpr");
    if (while_expression != nullptr) {
        auto* comparison = dynamic_cast<ast::BinaryExpr*>(while_expression->condition.get());
        check(comparison != nullptr && comparison->op == ast::BinaryOperator::Less,
              "while condition becomes a comparison BinaryExpr");
        if (comparison != nullptr) {
            auto* shift_right = dynamic_cast<ast::BinaryExpr*>(comparison->left.get());
            check(shift_right != nullptr &&
                      shift_right->op == ast::BinaryOperator::ShiftRight,
                  "condition shift rule preserves the final right shift");
            if (shift_right != nullptr) {
                auto* shift_left = dynamic_cast<ast::BinaryExpr*>(shift_right->left.get());
                check(shift_left != nullptr &&
                          shift_left->op == ast::BinaryOperator::ShiftLeft,
                      "condition shift chain folds from left to right");
            }
        }
        check(while_expression->body.statements.size() == 1,
              "while body retains its assignment statement");
        if (!while_expression->body.statements.empty()) {
            auto* assignment_statement = dynamic_cast<ast::ExpressionStatement*>(
                while_expression->body.statements.front().get());
            auto* assignment = assignment_statement == nullptr
                ? nullptr : dynamic_cast<ast::AssignmentExpr*>(
                    assignment_statement->expression.get());
            check(assignment != nullptr &&
                      assignment->op == ast::AssignmentOperator::Add,
                  "statementExpression builds a compound assignment");
        }
    }

    auto* shift_statement =
        dynamic_cast<ast::ExpressionStatement*>(function->body.statements[2].get());
    auto* shift = shift_statement == nullptr ? nullptr
        : dynamic_cast<ast::BinaryExpr*>(shift_statement->expression.get());
    check(shift != nullptr && shift->op == ast::BinaryOperator::ShiftRight,
          "statementExpression builds a chained shift expression");
}

}  // namespace

int main() {
    testConcreteNodeCategories();
    testOwnedChildrenAndExpressionShape();
    testTypeAndRecursiveUseChildren();
    testParserToAstSmokeCase();
    testOrdinaryExpressionPrecedenceAndBlockContents();
    testConditionBreakAndStatementExpressions();

    if (failures != 0) {
        std::cerr << failures << " AST test(s) failed\n";
        return 1;
    }
    std::cout << "AST tests passed\n";
    return 0;
}
