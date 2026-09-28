#pragma once

#include <memory>
#include <vector>

#include "rx/ast/item.hpp"
#include "rx/ast/node.hpp"

namespace rx::ast {

// 程序 AST 的根节点，直接继承 AstNode；对应 Parser.g4 的 crate 规则。
// items 按源文件顺序拥有每个顶层 item（use、fn、struct、const 或 impl）。
struct Program final : AstNode {
    Program() : AstNode(NodeKind::Program) {}
    [[nodiscard]] AstChildren children() const override;

    std::vector<std::unique_ptr<Item>> items;  // crate 中的顶层条目，顺序与源码一致。
};

}  // namespace rx::ast
