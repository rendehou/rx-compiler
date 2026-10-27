#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include "rx/ast/type_syntax.hpp"

namespace rx::semantic {

    
    enum class TypeKind {
        //具体类型
        I32,
        U32,
        ISize,
        USize,
        Bool,
        Unit,
        Never,
        Struct,
        Reference,
        //组合类型，可以在它们的里面嵌套
        Array,
        Box,
        Vec,
        Error//各种不匹配时
    };

    struct Type {
        TypeKind kind = TypeKind::Error;

        //Struct名称
        std::string struct_name;

        // Reference / Array / Box / Vec的指针
        std::shared_ptr<Type> inner;

        // Reference使用
        bool mutable_reference = false;

        // Array使用
        std::uint64_t array_length = 0;

        Type() = default;
        explicit Type(TypeKind kind);
        //构造函数
        static Type makeI32();
        static Type makeU32();
        static Type makeISize();
        static Type makeUSize();
        static Type makeBool();
        static Type makeUnit();
        static Type makeNever();//用来标注某些进程，某段代码的结束
        static Type makeError();//抛出错误
        static Type makeStruct(std::string name);
        static Type makeReference(Type referent, bool mutable_reference);
        static Type makeArray(Type element, std::uint64_t length);
        static Type makeBox(Type element);
        static Type makeVec(Type element);

        //判断函数
        bool isInteger() const;
        bool isSignedInteger() const;
        bool isUnsignedInteger() const;
        bool isBool() const;
        bool isUnit() const;
        bool isNever() const;
        bool isError() const;
        bool isStruct() const;
        bool isReference() const;
        bool isArray() const;
        bool isBox() const;
        bool isVec() const;

        bool operator==(const Type& other) const;
        bool operator!=(const Type& other) const;

    };
    Type resolveType(//类型转换，把ast的类型转换成自然类型，其中这个self_type用来标记，如果转换过程中出现self，它来自哪里
        const std::shared_ptr<rx::ast::TypeSyntax>& syntax,
        const std::string& self_type = ""
    );
} // namespace rx::semantic