#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "rx/ast/source_span.hpp"
#include "rx/ast/statement.hpp"
#include "rx/ast/type_syntax.hpp"

namespace rx::ast {

struct LifetimeParameter {
    SourceSpan span{};
    std::string name;
    std::vector<std::string> bounds;
};

struct GenericParameters {
    SourceSpan span{};
    bool written = false;
    std::vector<LifetimeParameter> lifetimes;
};

struct WherePredicate {
    SourceSpan span{};
    bool subject_is_lifetime = false;
    std::string lifetime_subject;
    std::optional<TypeSyntax> type_subject;
    std::vector<std::string> lifetime_bounds;
};

struct WhereClause {
    SourceSpan span{};
    bool written = false;
    std::vector<WherePredicate> predicates;
};

struct FunctionParameter {
    SourceSpan span{};
    std::string name;
    bool mutable_binding = false;
    TypeSyntax type;
};

enum class ReceiverKind { Value, MutableValue, SharedReference, MutableReference };

struct ReceiverSyntax {
    SourceSpan span{};
    ReceiverKind kind = ReceiverKind::Value;
    std::string lifetime;
};

struct Item {
    SourceSpan span{};
    virtual ~Item() = default;
};

struct FunctionItem final : Item {
    std::string name;
    GenericParameters generic_parameters;
    std::optional<ReceiverSyntax> receiver;  // self forms used in impl methods.
    std::vector<FunctionParameter> parameters;
    std::optional<TypeSyntax> return_type;   // Empty means no explicit arrow (unit).
    WhereClause where_clause;
    Block body;
};

struct StructField {
    SourceSpan span{};
    std::string name;
    TypeSyntax type;
};

enum class DeriveTrait { Copy, Clone, PartialEq, Eq };

struct StructItem final : Item {
    std::string name;
    GenericParameters generic_parameters;
    WhereClause where_clause;
    std::vector<DeriveTrait> derives;  // Keep order and duplicates for checking.
    std::vector<StructField> fields;
};

struct ConstantItem final : Item {
    std::string name;
    TypeSyntax type;
    ConstValueSyntax initializer;
};

struct AssociatedItem {
    SourceSpan span{};
    std::variant<std::unique_ptr<FunctionItem>, std::unique_ptr<ConstantItem>> value;
};

struct ImplItem final : Item {
    GenericParameters generic_parameters;
    TypeSyntax target_type;
    WhereClause where_clause;
    std::vector<AssociatedItem> items;
};

// Top-level use declarations are recognized by ANTLR but may be dropped by
// AstBuilder because this assignment installs builtins independently of use.

}  // namespace rx::ast
