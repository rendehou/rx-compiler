#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

enum class TypeKind { I32, USize, Unit, Struct, Array };

struct Type {
    TypeKind kind;
    std::string name;
    std::shared_ptr<Type> element;
    std::size_t length = 0;
};

Type scalar(TypeKind kind) { return Type{kind, "", nullptr, 0}; }
Type structure(std::string name) { return Type{TypeKind::Struct, std::move(name), nullptr, 0}; }
Type array(Type element, std::size_t length) {
    return Type{TypeKind::Array, "", std::make_shared<Type>(std::move(element)), length};
}

bool operator==(const Type& left, const Type& right) {
    if (left.kind != right.kind) return false;
    if (left.kind == TypeKind::Struct) return left.name == right.name;
    if (left.kind == TypeKind::Array) {
        return left.length == right.length && *left.element == *right.element;
    }
    return true;
}

std::string typeName(const Type& type) {
    switch (type.kind) {
        case TypeKind::I32: return "i32";
        case TypeKind::USize: return "usize";
        case TypeKind::Unit: return "unit";
        case TypeKind::Struct: return type.name;
        case TypeKind::Array:
            return "[" + typeName(*type.element) + "; " + std::to_string(type.length) + "]";
    }
    return "?";
}

enum class Category { Value, Place };
struct ExprInfo { Type type; Category category; bool mutable_place; };
struct StructDef { std::unordered_map<std::string, Type> fields; };

class SemanticError : public std::runtime_error {
public: using std::runtime_error::runtime_error;
};

class Checker {
public:
    void addStruct(std::string name, StructDef definition) {
        if (!structs_.emplace(std::move(name), std::move(definition)).second) {
            throw SemanticError("duplicate struct");
        }
    }

    ExprInfo structConstruct(
        const std::string& name,
        const std::vector<std::pair<std::string, ExprInfo>>& initializers) const {
        const StructDef& definition = findStruct(name);
        std::unordered_set<std::string> seen;
        for (const auto& [field_name, value] : initializers) {
            const auto field = definition.fields.find(field_name);
            if (field == definition.fields.end()) {
                throw SemanticError("unknown field " + field_name + " in " + name);
            }
            if (!seen.insert(field_name).second) {
                throw SemanticError("duplicate field " + field_name + " in " + name);
            }
            if (!(field->second == value.type)) {
                throw SemanticError("wrong type for field " + field_name);
            }
        }
        for (const auto& [field_name, type] : definition.fields) {
            (void)type;
            if (seen.find(field_name) == seen.end()) {
                throw SemanticError("missing field " + field_name + " in " + name);
            }
        }
        return {structure(name), Category::Value, false};
    }

    ExprInfo field(const ExprInfo& base, const std::string& field_name) const {
        if (base.type.kind != TypeKind::Struct) throw SemanticError("field base is not struct");
        const StructDef& definition = findStruct(base.type.name);
        const auto field = definition.fields.find(field_name);
        if (field == definition.fields.end()) throw SemanticError("unknown field: " + field_name);
        return {field->second, base.category,
                base.category == Category::Place && base.mutable_place};
    }

    ExprInfo arrayLiteral(const std::vector<ExprInfo>& elements) const {
        if (elements.empty()) throw SemanticError("cannot infer empty array element type");
        for (const ExprInfo& element : elements) {
            if (!(element.type == elements[0].type)) throw SemanticError("array element mismatch");
        }
        return {array(elements[0].type, elements.size()), Category::Value, false};
    }

    ExprInfo index(const ExprInfo& base, const ExprInfo& index_value) const {
        if (base.type.kind != TypeKind::Array) throw SemanticError("index base is not array");
        if (index_value.type.kind != TypeKind::USize) throw SemanticError("array index must be usize");
        return {*base.type.element, base.category,
                base.category == Category::Place && base.mutable_place};
    }

    Type assign(const ExprInfo& destination, const ExprInfo& value) const {
        if (destination.category != Category::Place) {
            throw SemanticError("assignment destination is a value, not a place");
        }
        if (!destination.mutable_place) {
            throw SemanticError("assignment destination is immutable");
        }
        if (!(destination.type == value.type)) throw SemanticError("assignment type mismatch");
        return scalar(TypeKind::Unit);
    }

private:
    const StructDef& findStruct(const std::string& name) const {
        const auto found = structs_.find(name);
        if (found == structs_.end()) throw SemanticError("unknown struct: " + name);
        return found->second;
    }
    std::unordered_map<std::string, StructDef> structs_;
};

ExprInfo value(Type type) { return {std::move(type), Category::Value, false}; }
ExprInfo place(Type type, bool mutable_place) {
    return {std::move(type), Category::Place, mutable_place};
}

template <typename Action>
void show(std::string_view label, Action action) {
    try {
        const std::string result = action();
        std::cout << label << ": " << result << '\n';
    }
    catch (const SemanticError& error) {
        std::cout << label << ": error: " << error.what() << '\n';
    }
}

int main() {
    Checker checker;
    const Type i32 = scalar(TypeKind::I32);
    const Type usize = scalar(TypeKind::USize);
    checker.addStruct("Point", {{{"x", i32}, {"y", i32}}});
    checker.addStruct("Pair", {{{"x", i32}, {"y", i32}}});

    std::cout << "Point equals Pair: "
              << (structure("Point") == structure("Pair") ? "yes" : "no") << '\n';
    const ExprInfo point_x = checker.field(place(structure("Point"), true), "x");
    std::cout << "p.x: " << typeName(point_x.type) << ", place, mutable\n";
    show("assign p.x", [&] { return typeName(checker.assign(point_x, value(i32))); });
    show("assign arithmetic result", [&] {
        return typeName(checker.assign(value(i32), value(i32)));
    });

    const ExprInfo array_value = checker.arrayLiteral({value(i32), value(i32)});
    const ExprInfo immutable_element = checker.index(
        place(array_value.type, false), value(usize));
    show("assign immutable array element", [&] {
        return typeName(checker.assign(immutable_element, value(i32)));
    });
    show("construct missing field", [&] {
        return typeName(checker.structConstruct("Point", {{"x", value(i32)}}).type);
    });
}
