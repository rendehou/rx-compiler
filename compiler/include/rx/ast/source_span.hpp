#pragma once

#include <cstddef>

namespace rx::ast {

// 源码中的单个位置；不是多态 AST 节点，而是供每个 AstNode 保存的坐标值。
// line 从 1 开始、column 从 0 开始，与 ANTLR 坐标一致。
struct SourcePosition {
    std::size_t line = 0;   // 源码行号；0 表示未知。
    std::size_t column = 0; // 行内列号，从 0 开始。

    [[nodiscard]] constexpr bool valid() const noexcept { return line != 0; }
};

// 源码范围值类型；不是多态 AST 节点。AstNode 的 span 字段用它记录该节点对应的文本区间。
// begin/end 是左闭右开范围，因此不依赖 ANTLR 对象，解析树销毁后仍有效。
struct SourceSpan {
    SourcePosition begin{}; // 节点源码范围的起始位置。
    SourcePosition end{};   // 节点源码范围的结束位置（不包含该位置）。

    [[nodiscard]] constexpr bool valid() const noexcept { return begin.valid(); }

    [[nodiscard]] static constexpr SourceSpan point(const std::size_t line,
                                                    const std::size_t column) noexcept {
        const SourcePosition position{line, column};
        return SourceSpan{position, position};
    }
};

}  // namespace rx::ast
