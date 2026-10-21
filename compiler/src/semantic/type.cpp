#include "rx/semantic/type.hpp"
#include "rx/semantic/collector.hpp"
#include "rx/ast/type_syntax.hpp"
#include <utility>

namespace rx::semantic {//type负责声明用到的所有类型。

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

    Type resolveType(//递归进行类型转换
        const std::shared_ptr<rx::ast::TypeSyntax>& syntax,
        const std::string& self_type
    ) {

        if (!syntax) return Type::makeError();

        if (syntax->kind == rx::ast::TypeSyntaxKind::Unit) {//空括号
            return Type::makeUnit();
        }

        if (syntax->kind == rx::ast::TypeSyntaxKind::Reference) {//引用
            Type inner = resolveType(syntax->referent, self_type);//递归处理引用内层

            if (inner.isError()) return Type::makeError();
            return Type::makeReference(inner, syntax->mutable_reference);
        }

        if (syntax->kind == rx::ast::TypeSyntaxKind::Array) {
            Type inner = resolveType(syntax->array_element, self_type);//数组存的变量类型,递归处理内层
            if (inner.isError()) return Type::makeError();

            if (!syntax->length ||
                syntax->length->kind != rx::ast::ConstValueKind::Integer) {
                return Type::makeError();
            }//已经是整数

            std::uint64_t length = std::stoull(syntax->length->integer_spelling);//将数组原有的长度从字符串转换为ll

            return Type::makeArray(inner, length);
        }

        if (syntax->kind == rx::ast::TypeSyntaxKind::Path) {
            if (!syntax->path || syntax->path->segments.empty()) {
                return Type::makeError();
            }

            auto segment = syntax->path->segments.back();
            const std::string& name = segment->name;

            //简单类型
            if (name == "i32") return Type::makeI32();
            if (name == "u32") return Type::makeU32();
            if (name == "isize") return Type::makeISize();
            if (name == "usize") return Type::makeUSize();
            if (name == "bool") return Type::makeBool();

            if (name == "Self") {//自身
                if (self_type.empty()) return Type::makeError();
                return Type::makeStruct(self_type);
            }

            if (name == "Box") {
                if (segment->generic_arguments.size() != 1) {
                    return Type::makeError();
                }

                auto argument = segment->generic_arguments[0];//是否只有一个参数

                if (!argument ||
                    argument->kind != rx::ast::GenericArgumentKind::Type) {
                    return Type::makeError();
                }//检验是不是类型

                Type inner = resolveType(argument->type, self_type);//同样的逐层递归
                if (inner.isError()) return Type::makeError();

                return Type::makeBox(inner);
            }

            if (name == "Vec") {
                if (segment->generic_arguments.size() != 1) {
                    return Type::makeError();
                }

                auto argument = segment->generic_arguments[0];
                if (!argument ||
                    argument->kind != rx::ast::GenericArgumentKind::Type) {
                    return Type::makeError();
                }//是否是类型

                Type inner = resolveType(argument->type, self_type);
                if (inner.isError()) return Type::makeError();

                return Type::makeVec(inner);
            }

            if (struct_map.find(name) != struct_map.end()) {//用户定义的struct
                return Type::makeStruct(name);
            }

            return Type::makeError();
        }

        return Type::makeError();
    }

} // namespace rx::semantic