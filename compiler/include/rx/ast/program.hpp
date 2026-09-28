#pragma once

#include "rx/ast/source_span.hpp"
#include "rx/ast/type_syntax.hpp"
// Responsibility:
// - Define ast::Program, the root containing all retained top-level items.
// - Program owns every AST node after AstBuilder finishes.
// - use declarations/lifetime-only metadata may be intentionally discarded.

namespace rx::ast {
    struct Program {
        SourceSpan span{};
        virtual ~Program();
    };
}

