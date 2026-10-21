#include "rx/semantic/type.hpp"

#include <utility>

namespace rx::semantic {

    Type::Type(TypeKind kind) : kind(kind) {}

    Type Type::makeI32() {
        return Type(TypeKind::I32);
    }

    Type Type::makeU32() {
        return Type(TypeKind::U32);
    }

    Type Type::makeISize() {
        return Type(TypeKind::ISize);
    }

    Type Type::makeUSize() {
        return Type(TypeKind::USize);
    }

    Type Type::makeBool() {
        return Type(TypeKind::Bool);
    }

    Type Type::makeUnit() {
        return Type(TypeKind::Unit);
    }

    Type Type::makeNever() {
        return Type(TypeKind::Never);
    }

    Type Type::makeError() {
        return Type(TypeKind::Error);
    }

    Type Type::makeStruct(std::string name) {
        Type type(TypeKind::Struct);
        type.struct_name = std::move(name);
        return type;
    }

    Type Type::makeReference(Type referent, bool mutable_reference) {
        Type type(TypeKind::Reference);
        type.inner = std::make_shared<Type>(std::move(referent));
        type.mutable_reference = mutable_reference;
        return type;
    }

    Type Type::makeArray(Type element, std::uint64_t length) {
        Type type(TypeKind::Array);
        type.inner = std::make_shared<Type>(std::move(element));
        type.array_length = length;
        return type;
    }

    Type Type::makeBox(Type element) {
        Type type(TypeKind::Box);
        type.inner = std::make_shared<Type>(std::move(element));
        return type;
    }

    Type Type::makeVec(Type element) {
        Type type(TypeKind::Vec);
        type.inner = std::make_shared<Type>(std::move(element));
        return type;
    }

    bool Type::isInteger() const {
        return kind == TypeKind::I32
            || kind == TypeKind::U32
            || kind == TypeKind::ISize
            || kind == TypeKind::USize;
    }

    bool Type::isSignedInteger() const {
        return kind == TypeKind::I32
            || kind == TypeKind::ISize;
    }

    bool Type::isUnsignedInteger() const {
        return kind == TypeKind::U32
            || kind == TypeKind::USize;
    }

    bool Type::isBool() const {
        return kind == TypeKind::Bool;
    }

    bool Type::isUnit() const {
        return kind == TypeKind::Unit;
    }

    bool Type::isNever() const {
        return kind == TypeKind::Never;
    }

    bool Type::isError() const {
        return kind == TypeKind::Error;
    }

    bool Type::isStruct() const {
        return kind == TypeKind::Struct;
    }

    bool Type::isReference() const {
        return kind == TypeKind::Reference;
    }

    bool Type::isArray() const {
        return kind == TypeKind::Array;
    }

    bool Type::isBox() const {
        return kind == TypeKind::Box;
    }

    bool Type::isVec() const {
        return kind == TypeKind::Vec;
    }

    bool Type::operator==(const Type& other) const {
        if (kind != other.kind) return false;

        switch (kind) {
            case TypeKind::Struct:
                return struct_name == other.struct_name;

            case TypeKind::Reference:
                if (mutable_reference != other.mutable_reference) return false;
                if (!inner || !other.inner) return inner == other.inner;
                return *inner == *other.inner;

            case TypeKind::Array:
                if (array_length != other.array_length) return false;
                if (!inner || !other.inner) return inner == other.inner;
                return *inner == *other.inner;

            case TypeKind::Box:
            case TypeKind::Vec:
                if (!inner || !other.inner) return inner == other.inner;
                return *inner == *other.inner;

            default:
                return true;
        }
    }

    bool Type::operator!=(const Type& other) const {
        return !(*this == other);
    }

} // namespace rx::semantic