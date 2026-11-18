#pragma once

#include <unordered_map>
#include <vector>

#include "rx/ir/builder.hpp"
#include "rx/semantic/info.hpp"

namespace rx::ir {//翻译过程的中间信息

    // 一个表达式所代表的存储位置
    struct IRPlace {
        IRValue address;
        IRType type; 
    };

    // 表达式生成结果
    struct IRResult {
        bool falls_through = true; // 执行后表达式能否继续进行，跳转，返回等不行
        bool has_value = false;//是否产生ssa值
        IRValue value;// 产生的值
    };

    // 取得地址的结果
    struct PlaceResult {
        bool falls_through = true;// 执行后表达式能否继续进行，跳转，返回等不行
        bool has_place = false;// 
        IRPlace place;
    };

    // 当前进入的循环
    struct LoopFrame {
        BlockId break_target = 0;//执行break后的基本块
        BlockId continue_target = 0;//执行continue后的基本块

        bool has_result_slot = false;//循环是否要保存结果
        IRPlace result_slot;//保存结果的地址
    };

    // 生成一个函数时需要持续保存的状态
    struct FunctionContext {
        IRBuilder builder;

        // 局部变量存储位置表
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