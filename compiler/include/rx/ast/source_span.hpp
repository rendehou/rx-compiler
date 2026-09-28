#pragma once

#include <cstddef>

namespace rx::ast {

// A position in the source file. Lines are one-based; columns are zero-based,
// matching ANTLR's coordinates. Convert columns only when displaying them.
struct SourcePosition {
    std::size_t line = 0;   // Zero means that the location is unknown.
    std::size_t column = 0;

    [[nodiscard]] constexpr bool valid() const noexcept { return line != 0; }
};

// A half-open source range [begin, end). It contains plain numbers only, so
// AST nodes remain valid after ANTLR's tokens and parse tree are destroyed.
// A point diagnostic has begin == end.
struct SourceSpan {
    SourcePosition begin{};
    SourcePosition end{};

    [[nodiscard]] constexpr bool valid() const noexcept { return begin.valid(); }

    [[nodiscard]] static constexpr SourceSpan point(const std::size_t line,
                                                    const std::size_t column) noexcept {
        const SourcePosition position{line, column};
        return SourceSpan{position, position};
    }
};

}  // namespace rx::ast
