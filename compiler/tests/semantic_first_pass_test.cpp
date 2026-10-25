#include <initializer_list>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "rx/ast/item.hpp"
#include "rx/ast/program.hpp"
#include "rx/ast/type_syntax.hpp"
#include "rx/semantic/collector.hpp"
#include "rx/semantic/constevaluator.hpp"
#include "rx/semantic/declaration.hpp"
#include "rx/semantic/scope.hpp"
#include "rx/semantic/type.hpp"

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::shared_ptr<rx::ast::Path> makePath(const std::string& name) {
    auto segment = std::make_shared<rx::ast::PathSegment>();
    segment->name = name;

    auto path = std::make_shared<rx::ast::Path>();
    path->segments.push_back(segment);
    return path;
}

std::shared_ptr<rx::ast::TypeSyntax> makePathType(const std::string& name) {
    auto type = std::make_shared<rx::ast::TypeSyntax>();
    type->kind = rx::ast::TypeSyntaxKind::Path;
    type->path = makePath(name);
    return type;
}

std::shared_ptr<rx::ast::TypeSyntax> makeGenericType(
    const std::string& name,
    const std::shared_ptr<rx::ast::TypeSyntax>& inner
) {
    auto argument = std::make_shared<rx::ast::GenericArgumentSyntax>();
    argument->kind = rx::ast::GenericArgumentKind::Type;
    argument->type = inner;

    auto segment = std::make_shared<rx::ast::PathSegment>();
    segment->name = name;
    segment->generic_arguments.push_back(argument);

    auto path = std::make_shared<rx::ast::Path>();
    path->segments.push_back(segment);

    auto type = std::make_shared<rx::ast::TypeSyntax>();
    type->kind = rx::ast::TypeSyntaxKind::Path;
    type->path = path;
    return type;
}

std::shared_ptr<rx::ast::TypeSyntax> makeReferenceType(
    const std::shared_ptr<rx::ast::TypeSyntax>& referent,
    bool is_mutable
) {
    auto type = std::make_shared<rx::ast::TypeSyntax>();
    type->kind = rx::ast::TypeSyntaxKind::Reference;
    type->referent = referent;
    type->mutable_reference = is_mutable;
    return type;
}

std::shared_ptr<rx::ast::ConstValueSyntax> makeIntegerValue(
    const std::string& spelling
) {
    auto value = std::make_shared<rx::ast::ConstValueSyntax>();
    value->kind = rx::ast::ConstValueKind::Integer;
    value->integer_spelling = spelling;
    return value;
}

std::shared_ptr<rx::ast::ConstValueSyntax> makeBooleanValue(bool boolean_value) {
    auto value = std::make_shared<rx::ast::ConstValueSyntax>();
    value->kind = rx::ast::ConstValueKind::Boolean;
    value->boolean_value = boolean_value;
    return value;
}

std::shared_ptr<rx::ast::ConstValueSyntax> makePathValue(const std::string& name) {
    auto value = std::make_shared<rx::ast::ConstValueSyntax>();
    value->kind = rx::ast::ConstValueKind::Path;
    value->path = makePath(name);
    return value;
}

std::shared_ptr<rx::ast::ConstValueSyntax> makeNegatedValue(
    const std::shared_ptr<rx::ast::ConstValueSyntax>& operand
) {
    auto value = std::make_shared<rx::ast::ConstValueSyntax>();
    value->kind = rx::ast::ConstValueKind::Negate;
    value->operand = operand;
    return value;
}

std::shared_ptr<rx::ast::TypeSyntax> makeArrayType(
    const std::shared_ptr<rx::ast::TypeSyntax>& element,
    const std::string& length
) {
    auto type = std::make_shared<rx::ast::TypeSyntax>();
    type->kind = rx::ast::TypeSyntaxKind::Array;
    type->array_element = element;
    type->length = makeIntegerValue(length);
    return type;
}

std::shared_ptr<rx::ast::FunctionItem> makeFunction(const std::string& name) {
    auto function = std::make_shared<rx::ast::FunctionItem>();
    function->name = name;
    return function;
}

std::shared_ptr<rx::ast::StructItem> makeStruct(const std::string& name) {
    auto structure = std::make_shared<rx::ast::StructItem>();
    structure->name = name;
    return structure;
}

std::shared_ptr<rx::ast::FunctionParameter> makeParameter(
    const std::string& name,
    const std::shared_ptr<rx::ast::TypeSyntax>& type
) {
    auto parameter = std::make_shared<rx::ast::FunctionParameter>();
    parameter->name = name;
    parameter->type = type;
    return parameter;
}

std::shared_ptr<rx::ast::StructField> makeField(
    const std::string& name,
    const std::shared_ptr<rx::ast::TypeSyntax>& type
) {
    auto field = std::make_shared<rx::ast::StructField>();
    field->name = name;
    field->type = type;
    return field;
}

std::shared_ptr<rx::ast::ConstantItem> makeConstant(
    const std::string& name,
    const std::shared_ptr<rx::ast::TypeSyntax>& type,
    const std::shared_ptr<rx::ast::ConstValueSyntax>& initializer
) {
    auto constant = std::make_shared<rx::ast::ConstantItem>();
    constant->name = name;
    constant->type = type;
    constant->initializer = initializer;
    return constant;
}

std::shared_ptr<rx::ast::Program> makeProgram(
    std::initializer_list<std::shared_ptr<rx::ast::Item>> items
) {
    auto program = std::make_shared<rx::ast::Program>();
    for (const auto& item : items) {
        program->items.push_back(item);
    }
    return program;
}

void testCollector() {
    auto function = makeFunction("foo");
    auto structure = makeStruct("Point");
    auto constant = makeConstant("N", makePathType("i32"), makeIntegerValue("7"));

    auto program = makeProgram({function, structure, constant});
    expect(rx::semantic::collectItems(program), "collector rejected valid top-level items");

    expect(rx::semantic::value_map.size() == 2, "collector value_map size is wrong");
    expect(rx::semantic::struct_map.size() == 1, "collector struct_map size is wrong");
    expect(
        rx::semantic::value_map.at("foo").kind == rx::semantic::ValueSymbolKind::Function,
        "function was not collected as a function symbol"
    );
    expect(
        rx::semantic::value_map.at("N").kind == rx::semantic::ValueSymbolKind::Constant,
        "constant was not collected as a constant symbol"
    );
    expect(rx::semantic::struct_map.at("Point") == structure, "struct symbol points to wrong AST node");

    auto duplicate_value = makeProgram({
        makeFunction("same"),
        makeConstant("same", makePathType("i32"), makeIntegerValue("1"))
    });
    expect(!rx::semantic::collectItems(duplicate_value), "function/constant duplicate was not rejected");

    auto duplicate_struct = makeProgram({makeStruct("S"), makeStruct("S")});
    expect(!rx::semantic::collectItems(duplicate_struct), "duplicate struct was not rejected");

    auto separate_namespaces = makeProgram({makeStruct("same"), makeFunction("same")});
    expect(
        rx::semantic::collectItems(separate_namespaces),
        "struct and value namespaces should be independent in the current design"
    );
    expect(rx::semantic::struct_map.count("same") == 1, "same-named struct was not collected");
    expect(rx::semantic::value_map.count("same") == 1, "same-named value was not collected");

    auto first = makeProgram({makeFunction("old_name")});
    expect(rx::semantic::collectItems(first), "collector rejected first reset test program");
    expect(rx::semantic::value_map.count("old_name") == 1, "old_name was not collected");

    auto second = makeProgram({makeFunction("new_name")});
    expect(rx::semantic::collectItems(second), "collector rejected second reset test program");
    expect(rx::semantic::value_map.count("old_name") == 0, "collector did not clear stale symbols");
    expect(rx::semantic::value_map.count("new_name") == 1, "new_name was not collected after reset");
}

void testTypeResolution() {
    auto point = makeStruct("Point");
    auto program = makeProgram({point});
    expect(rx::semantic::collectItems(program), "collector failed before type-resolution test");

    expect(rx::semantic::resolveType(makePathType("i32")) == rx::semantic::Type::makeI32(), "i32 resolution failed");
    expect(rx::semantic::resolveType(makePathType("bool")) == rx::semantic::Type::makeBool(), "bool resolution failed");
    expect(rx::semantic::resolveType(makePathType("Point")) == rx::semantic::Type::makeStruct("Point"), "struct type resolution failed");
    expect(rx::semantic::resolveType(makePathType("Missing")).isError(), "unknown type should resolve to Error");

    auto reference = rx::semantic::resolveType(makeReferenceType(makePathType("i32"), true));
    expect(reference.isReference(), "reference type was not recognized");
    expect(reference.mutable_reference, "mutable reference flag was lost");
    expect(reference.inner && *reference.inner == rx::semantic::Type::makeI32(), "reference inner type is wrong");

    auto array = rx::semantic::resolveType(makeArrayType(makePathType("bool"), "4"));
    expect(array.isArray(), "array type was not recognized");
    expect(array.array_length == 4, "array length is wrong");
    expect(array.inner && *array.inner == rx::semantic::Type::makeBool(), "array element type is wrong");

    auto box = rx::semantic::resolveType(makeGenericType("Box", makePathType("i32")));
    expect(box.isBox(), "Box<T> type was not recognized");
    expect(box.inner && *box.inner == rx::semantic::Type::makeI32(), "Box inner type is wrong");

    auto vec = rx::semantic::resolveType(makeGenericType("Vec", makePathType("Point")));
    expect(vec.isVec(), "Vec<T> type was not recognized");
    expect(vec.inner && *vec.inner == rx::semantic::Type::makeStruct("Point"), "Vec inner type is wrong");
}

void testDeclarations() {
    auto f = makeFunction("f");
    auto g = makeFunction("g");
    f->parameters.push_back(makeParameter("x", makePathType("i32")));
    g->parameters.push_back(makeParameter("x", makePathType("bool")));

    auto separate_functions = makeProgram({f, g});
    expect(rx::semantic::collectItems(separate_functions), "collector failed for separate functions");
    expect(
        rx::semantic::checkDeclarations(separate_functions),
        "same parameter name in different functions should be legal"
    );

    auto duplicate_parameter = makeFunction("duplicate_parameter");
    duplicate_parameter->parameters.push_back(makeParameter("x", makePathType("i32")));
    duplicate_parameter->parameters.push_back(makeParameter("x", makePathType("bool")));
    auto duplicate_parameter_program = makeProgram({duplicate_parameter});
    expect(rx::semantic::collectItems(duplicate_parameter_program), "collector failed for duplicate-parameter program");
    expect(
        !rx::semantic::checkDeclarations(duplicate_parameter_program),
        "duplicate parameter in one function was not rejected"
    );

    auto a = makeStruct("A");
    auto b = makeStruct("B");
    a->fields.push_back(makeField("x", makePathType("i32")));
    b->fields.push_back(makeField("x", makePathType("bool")));
    auto separate_structs = makeProgram({a, b});
    expect(rx::semantic::collectItems(separate_structs), "collector failed for separate structs");
    expect(
        rx::semantic::checkDeclarations(separate_structs),
        "same field name in different structs should be legal"
    );

    auto duplicate_field = makeStruct("DuplicateField");
    duplicate_field->fields.push_back(makeField("x", makePathType("i32")));
    duplicate_field->fields.push_back(makeField("x", makePathType("bool")));
    auto duplicate_field_program = makeProgram({duplicate_field});
    expect(rx::semantic::collectItems(duplicate_field_program), "collector failed for duplicate-field program");
    expect(
        !rx::semantic::checkDeclarations(duplicate_field_program),
        "duplicate field in one struct was not rejected"
    );

    auto unknown_parameter = makeFunction("unknown_parameter");
    unknown_parameter->parameters.push_back(makeParameter("x", makePathType("UnknownType")));
    auto unknown_parameter_program = makeProgram({unknown_parameter});
    expect(rx::semantic::collectItems(unknown_parameter_program), "collector failed for unknown-type program");
    expect(
        !rx::semantic::checkDeclarations(unknown_parameter_program),
        "unknown parameter type was not rejected"
    );

    auto point = makeStruct("Point");
    auto use_point = makeFunction("use_point");
    use_point->parameters.push_back(makeParameter("p", makePathType("Point")));
    use_point->return_type = makePathType("Point");
    auto user_type_program = makeProgram({point, use_point});
    expect(rx::semantic::collectItems(user_type_program), "collector failed for user-defined type program");
    expect(
        rx::semantic::checkDeclarations(user_type_program),
        "valid user-defined struct type was rejected in declaration checking"
    );
}

void testConstEvaluator() {
    auto a = makeConstant("A", makePathType("i32"), makeIntegerValue("7"));
    auto b = makeConstant("B", makePathType("i32"), makePathValue("A"));
    auto c = makeConstant("C", makePathType("i32"), makePathValue("B"));
    auto truth = makeConstant("TRUTH", makePathType("bool"), makeBooleanValue(true));
    auto negative = makeConstant("NEG", makePathType("i32"), makeNegatedValue(makeIntegerValue("12")));

    auto program = makeProgram({a, b, c, truth, negative});
    expect(rx::semantic::collectItems(program), "collector failed before constant evaluation");
    expect(rx::semantic::checkDeclarations(program), "declaration check failed before constant evaluation");

    rx::semantic::ConstEvaluator evaluator;
    rx::semantic::ConstValue result;

    expect(evaluator.evaluateConstant(c, result), "constant reference chain failed");
    expect(result.type == rx::semantic::Type::makeI32(), "constant reference chain has wrong type");
    expect(result.signed_value == 7, "constant reference chain has wrong value");
    expect(evaluator.find("A") != nullptr, "referenced constant A was not cached");
    expect(evaluator.find("B") != nullptr, "referenced constant B was not cached");

    expect(evaluator.evaluateConstant(truth, result), "boolean constant evaluation failed");
    expect(result.type == rx::semantic::Type::makeBool(), "boolean constant has wrong type");
    expect(result.bool_value, "boolean constant has wrong value");

    expect(evaluator.evaluateConstant(negative, result), "negated signed constant evaluation failed");
    expect(result.signed_value == -12, "negated signed constant has wrong value");

    auto mismatch_source = makeConstant("SOURCE", makePathType("i32"), makeIntegerValue("1"));
    auto mismatch = makeConstant("MISMATCH", makePathType("bool"), makePathValue("SOURCE"));
    auto mismatch_program = makeProgram({mismatch_source, mismatch});
    expect(rx::semantic::collectItems(mismatch_program), "collector failed for constant mismatch program");

    rx::semantic::ConstEvaluator mismatch_evaluator;
    expect(
        !mismatch_evaluator.evaluateConstant(mismatch, result),
        "constant reference with mismatched type was not rejected"
    );

    auto cycle_a = makeConstant("CYCLE_A", makePathType("i32"), makePathValue("CYCLE_B"));
    auto cycle_b = makeConstant("CYCLE_B", makePathType("i32"), makePathValue("CYCLE_A"));
    auto cycle_program = makeProgram({cycle_a, cycle_b});
    expect(rx::semantic::collectItems(cycle_program), "collector failed for constant cycle program");

    rx::semantic::ConstEvaluator cycle_evaluator;
    expect(
        !cycle_evaluator.evaluateConstant(cycle_a, result),
        "cyclic constant dependency was not rejected"
    );
}

void testScope() {
    rx::semantic::Scope outer;

    rx::semantic::LocalSymbol outer_x;
    outer_x.type = rx::semantic::Type::makeI32();
    outer_x.mutable_binding = false;
    outer.define("x", outer_x);

    rx::semantic::Scope inner(&outer);
    expect(inner.findCurrent("x") == nullptr, "parent variable appeared in current scope");
    expect(inner.find("x") != nullptr, "parent variable was not found through scope chain");
    expect(inner.find("x")->type == rx::semantic::Type::makeI32(), "parent variable has wrong type");

    rx::semantic::LocalSymbol inner_x;
    inner_x.type = rx::semantic::Type::makeBool();
    inner_x.mutable_binding = true;
    inner.define("x", inner_x);

    expect(inner.findCurrent("x") != nullptr, "shadowing variable was not defined in inner scope");
    expect(inner.find("x")->type == rx::semantic::Type::makeBool(), "inner variable did not shadow parent");
    expect(inner.find("x")->mutable_binding, "inner mutable flag was lost");
    expect(outer.find("x")->type == rx::semantic::Type::makeI32(), "shadowing modified parent scope");
    expect(inner.find("missing") == nullptr, "missing variable unexpectedly resolved");
}

}  // namespace

int main() {
    try {
        testCollector();
        testTypeResolution();
        testDeclarations();
        testConstEvaluator();
        testScope();
    } catch (const std::exception& error) {
        std::cerr << "semantic_first_pass_test failed: " << error.what() << '\n';
        return 1;
    }

    std::cout << "semantic_first_pass_test passed\n";
    return 0;
}
