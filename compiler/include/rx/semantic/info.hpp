#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "rx/ast/node.hpp"
#include "rx/ast/expression.hpp"
#include "rx/semantic/type.hpp"

namespace rx::semantic {

    using BindingId = std::size_t;

    // AST 节点编号
    using NodeId = ast::NodeId;
    
    // 方法调用时已经进行的类型转换，包括函数，struct的隐式类型转换，自动借用，自动解引用等
    enum class AdjustmentKind {
        Deref,
        BorrowShared,
        BorrowMutable
    };

    struct CallResolution {
        std::string target; // 解析后的函数或方法标识
        std::vector<AdjustmentKind> receiver_adjustments;
    };

    // 传递给ir的结果
    struct SemanticInfo {

        // 每个变量绑定对应一个不同的编号
        std::unordered_map<NodeId, BindingId> bindings;

        // 对每个ast表达式给出类型
        std::unordered_map<NodeId, Type> expr_types;

        // 为函数调用类型直接给出结果
        std::unordered_map<NodeId, CallResolution> calls;
        
    };

}