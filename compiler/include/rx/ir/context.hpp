#pragma once

#include <unordered_map>
#include <vector>

#include "rx/ir/builder.hpp"
#include "rx/semantic/info.hpp"

namespace rx::ir {

    // 一个表达式所代表的存储位置
    struct IRPlace {
        IRValue address;
        IRType type;           // 这个位置存储的数据类型
    };

    // 表达式生成结果
    struct IRResult {
        bool falls_through = true; // 当前路径能否继续执行
        bool has_value = false;    // 是否产生实际 LLVM 值
        IRValue value;
    };

    // 取得地址的结果
    struct PlaceResult {
        bool falls_through = true;
        bool has_place = false;
        IRPlace place;
    };

    // 当前进入的循环
    struct LoopFrame {
        BlockId break_target = 0;
        BlockId continue_target = 0;

        // 带值的 loop 可以通过临时存储位置合并 break 结果
        bool has_result_slot = false;
        IRPlace result_slot;
    };

    // 生成一个函数时需要持续保存的状态
    struct FunctionContext {
        IRBuilder builder;

        // Binding ID -> 局部变量存储位置
        std::unordered_map<semantic::BindingId, IRPlace> locals;

        // 当前嵌套循环
        std::vector<LoopFrame> loops;

        // 当前函数的返回类型
        IRType return_type;

        FunctionContext(
            IRModule& module,
            FunctionId function,
            const IRType& type
        ) : builder(module, function), return_type(type) {}
    };

}