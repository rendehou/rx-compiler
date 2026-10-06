#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

enum class TypeKind { I32, Unit, Struct, Ref, MutRef };

struct Type {
    TypeKind kind;
    std::string name;
    std::shared_ptr<Type> inner;
};

Type scalar(TypeKind kind) { return {kind, "", nullptr}; }
Type structure(std::string name) { return {TypeKind::Struct, std::move(name), nullptr}; }
Type reference(Type inner, bool mutable_reference) {
    return {mutable_reference ? TypeKind::MutRef : TypeKind::Ref,
            "", std::make_shared<Type>(std::move(inner))};
}

bool operator==(const Type& left, const Type& right) {
    if (left.kind != right.kind) return false;
    if (left.kind == TypeKind::Struct) return left.name == right.name;
    if (left.kind == TypeKind::Ref || left.kind == TypeKind::MutRef) {
        return *left.inner == *right.inner;
    }
    return true;
}

std::string typeName(const Type& type) {
    switch (type.kind) {
        case TypeKind::I32: return "i32";
        case TypeKind::Unit: return "unit";
        case TypeKind::Struct: return type.name;
        case TypeKind::Ref: return "&" + typeName(*type.inner);
        case TypeKind::MutRef: return "&mut " + typeName(*type.inner);
    }
    return "?";
}

struct ExprInfo {
    Type type;
    bool is_place;
    bool writable;
    bool crossed_shared;
};

class SemanticError : public std::runtime_error {
public: using std::runtime_error::runtime_error;
};

ExprInfo borrow(const ExprInfo& source, bool mutable_borrow) {
    if (mutable_borrow && source.is_place && !source.writable) {
        throw SemanticError("cannot mutably borrow immutable place");
    }
    return {reference(source.type, mutable_borrow), false, false, false};
}

ExprInfo dereference(const ExprInfo& source) {
    if (source.type.kind != TypeKind::Ref && source.type.kind != TypeKind::MutRef) {
        throw SemanticError("cannot dereference non-reference");
    }
    const bool crossed_shared = source.crossed_shared || source.type.kind == TypeKind::Ref;
    const bool writable = source.type.kind == TypeKind::MutRef && !crossed_shared;
    return {*source.type.inner, true, writable, crossed_shared};
}

Type coerce(const Type& source, const Type& target) {
    if (source == target) return target;
    if (source.kind == TypeKind::MutRef && target.kind == TypeKind::Ref &&
        *source.inner == *target.inner) {
        return target;
    }
    throw SemanticError("cannot coerce " + typeName(source) + " to " + typeName(target));
}

enum class Receiver { Shared, Mutable };
struct Method { Receiver receiver; Type result; };

class MethodTable {
public:
    void add(const std::string& struct_name, const std::string& method_name,
             Receiver receiver, Type result) {
        methods_[struct_name + "::" + method_name] = Method{receiver, std::move(result)};
    }

    ExprInfo call(ExprInfo receiver_value, const std::string& method_name) const {
        while (receiver_value.type.kind == TypeKind::Ref ||
               receiver_value.type.kind == TypeKind::MutRef) {
            receiver_value = dereference(receiver_value);
        }
        if (receiver_value.type.kind != TypeKind::Struct) {
            throw SemanticError("method receiver is not a struct");
        }
        const std::string key = receiver_value.type.name + "::" + method_name;
        const auto found = methods_.find(key);
        if (found == methods_.end()) throw SemanticError("unknown method: " + key);
        if (found->second.receiver == Receiver::Mutable &&
            (!receiver_value.is_place || !receiver_value.writable)) {
            throw SemanticError("method " + method_name + " requires mutable receiver");
        }
        return {found->second.result, false, false, false};
    }

private:
    std::unordered_map<std::string, Method> methods_;
};

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
    const Type i32 = scalar(TypeKind::I32);
    const ExprInfo immutable_i32{i32, true, false, false};
    const ExprInfo mutable_i32{i32, true, true, false};

    show("&mut immutable x", [&] { return typeName(borrow(immutable_i32, true).type); });
    std::cout << "&mut mutable x: " << typeName(borrow(mutable_i32, true).type) << '\n';
    std::cout << "coerce &mut i32 to &i32: "
              << typeName(coerce(reference(i32, true), reference(i32, false))) << '\n';

    ExprInfo double_reference{reference(reference(i32, true), false), true, false, false};
    const ExprInfo after_two_dereferences = dereference(dereference(double_reference));
    std::cout << "write through &&mut i32: "
              << (after_two_dereferences.writable ? "allowed" : "denied") << '\n';

    MethodTable methods;
    methods.add("Counter", "get", Receiver::Shared, i32);
    methods.add("Counter", "increase", Receiver::Mutable, scalar(TypeKind::Unit));
    const ExprInfo mutable_counter{structure("Counter"), true, true, false};
    const ExprInfo immutable_counter{structure("Counter"), true, false, false};
    std::cout << "mutable counter.increase: "
              << typeName(methods.call(mutable_counter, "increase").type) << '\n';
    std::cout << "immutable counter.get: "
              << typeName(methods.call(immutable_counter, "get").type) << '\n';
    show("immutable counter.increase", [&] {
        return typeName(methods.call(immutable_counter, "increase").type);
    });
}
