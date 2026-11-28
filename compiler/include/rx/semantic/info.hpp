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

        std::string target;

        // 最终解析到的函数 AST 节点
        NodeId function_node = ast::InvalidNodeId;

        // 自动解引用、自动借用等调整
        std::vector<AdjustmentKind> receiver_adjustments;
    };

    struct FunctionInfo {

        // 函数返回类型
        Type return_type;

        // 函数所属结构体，普通函数为空
        std::string owner_type;

        // 参数对应的 BindingId，如果有 self，self为第一个
        std::vector<BindingId> parameter_bindings;
    };

    struct StructFieldInfo {
        std::string name;
        Type type;
        std::size_t index = 0;
    };

    struct StructInfo {
        std::string name;
        std::vector<StructFieldInfo> fields;
    };

    // 传递给ir的结果
    struct SemanticInfo {

        // 每个变量绑定对应一个不同的编号
        std::unordered_map<NodeId, BindingId> bindings;

        // 对每个ast表达式给出类型
        std::unordered_map<NodeId, Type> expr_types;

        // 为函数调用类型直接给出结果
        std::unordered_map<NodeId, CallResolution> calls;
        
        // 对变量绑定直接给出类型
        std::unordered_map<BindingId, Type> binding_types;

        // 函数调用表
        std::unordered_map<NodeId, CallResolution> calls;

        // 函数信息表
        std::unordered_map<NodeId, FunctionInfo> functions;

        // 结构体信息表
        std::unordered_map<NodeId, StructInfo> structs;
    };

}