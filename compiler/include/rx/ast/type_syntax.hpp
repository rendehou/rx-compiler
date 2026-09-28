#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rx/ast/source_span.hpp"

namespace rx::ast {

struct TypeSyntax;
struct GenericArgumentSyntax;

// A path is shared by type paths, value paths, and constant paths. Each
// segment owns its own generic arguments, as in Vec::<i32>::new.
struct PathSegment {
    SourceSpan span{};
    std::string name;  // Also preserves self / Self when present.
    std::vector<GenericArgumentSyntax> generic_arguments;
};

struct Path {
    SourceSpan span{};
    bool absolute = false;
    std::vector<PathSegment> segments;
};

enum class ConstValueKind { Integer, Boolean, Path, Negate };

// The restricted constValue grammar, not an arbitrary run-time Expr.
struct ConstValueSyntax {
    SourceSpan span{};
    ConstValueKind kind = ConstValueKind::Integer;
    std::string integer_spelling;
    bool boolean_value = false;
    Path path;
    std::unique_ptr<ConstValueSyntax> operand;
};

enum class TypeSyntaxKind { Unit, Path, Reference, Array };

// This is what the source wrote. Resolved types/StructIds belong to semantic/.
struct TypeSyntax {
    SourceSpan span{};
    TypeSyntaxKind kind = TypeSyntaxKind::Unit;
    Path path;                                  // Path
    std::string lifetime;                       // Optional reference lifetime
    bool mutable_reference = false;             // Reference
    std::unique_ptr<TypeSyntax> referent;        // Reference
    std::unique_ptr<TypeSyntax> array_element;   // Array
    std::unique_ptr<ConstValueSyntax> length;    // Array
};

enum class GenericArgumentKind { Lifetime, Type };

// Type is meaningful only for kind == Type; lifetime is meaningful only for
// kind == Lifetime. Keep source order so invalid argument lists remain visible.
struct GenericArgumentSyntax {
    SourceSpan span{};
    GenericArgumentKind kind = GenericArgumentKind::Type;
    std::string lifetime;
    TypeSyntax type;  // Meaningful only for kind == Type.
};

}  // namespace rx::ast
