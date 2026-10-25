#pragma once

#include "rx/ast/item.hpp"
#include "rx/ast/type_syntax.hpp"
#include "rx/semantic/type.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace rx::semantic {

    enum class ConstState {
        Unvisited,
        Evaluated
    };

    struct ConstValue {//可能是signed，unsigned和bool
        Type type;

        std::int64_t signed_value = 0;
        std::uint64_t unsigned_value = 0;
        bool bool_value = false;
    };

    class ConstEvaluator {
    public:
        ConstEvaluator() = default;

        bool evaluateConstant(//处理一整个赋值表达
            const std::shared_ptr<ast::ConstantItem>& constant,
            ConstValue& result
        );

        bool evaluateValue(//int bool negate path
            const std::shared_ptr<ast::ConstValueSyntax>& value,
            const Type& expected_type,
            ConstValue& result
        );

        const ConstValue* find(//查询已经有的值，在values里面找
            const std::string& name
        ) const;

    private:
        std::unordered_map<std::string, ConstState> states_;//变量常量计算状态
        std::unordered_map<std::string, ConstValue> values_;//变量，常量的值

        bool evaluateByName(//根据名称查询是否已经被计算
            const std::string& name,
            ConstValue& result
        );
    };
    

}