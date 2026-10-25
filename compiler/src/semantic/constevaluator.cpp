#include "rx/semantic/constevaluator.hpp"
#include "rx/semantic/collector.hpp"

#include <string>

namespace rx::semantic {

   bool ConstEvaluator::evaluateConstant(//处理整个表达式，并用evaluate value来计算值
        const std::shared_ptr<ast::ConstantItem>& constant,
        ConstValue& result
    ) {
        if (!constant) {
            return false;
        }

        Type type = resolveType(constant->type);

        if (type.isError()) {
            return false;
        }
        if (!evaluateValue(constant->initializer, type, result)) {
            return false;
        }
        if (result.type != type) {
            return false;
        }
        return true;
    }

    bool ConstEvaluator::evaluateValue(//evaluator具体的值，把ast节点赋值，string变int或者加负号，可递归
        const std::shared_ptr<ast::ConstValueSyntax>& value,
        const Type& type,
        ConstValue& result
    ) {
        if (!value) {
            return false;
        }

        switch (value->kind) {

            case ast::ConstValueKind::Integer: {
                if (!type.isInteger()) {
                    return false;
                }

                try {
                    if (type.isSignedInteger()) {
                        result.signed_value = std::stoll(value->integer_spelling);
                    }
                    else {
                        result.unsigned_value = std::stoull(value->integer_spelling);
                    }
                }
                catch (...) {
                    return false;
                }

                result.type = type;
                return true;
            }

            case ast::ConstValueKind::Boolean: {
                if (!type.isBool()) {
                    return false;
                }
                result.type = type;
                result.bool_value = value->boolean_value;
                return true;
            }

            case ast::ConstValueKind::Path: {
                if (!value->path || value->path->segments.size() != 1) {
                    return false;
                }

                const std::string& name = value->path->segments[0]->name;

                ConstValue referenced;

                if (!evaluateByName(name, referenced)) {
                    return false;
                }
                if (referenced.type != type) {
                    return false;
                }

                result = referenced;

                return true;
            }
            case ast::ConstValueKind::Negate: {
                if (!type.isSignedInteger()) {
                    return false;
                }

                ConstValue operand;
                if (!evaluateValue(value->operand, type, operand)) {
                    return false;
                }

                result = operand;
                result.signed_value = -operand.signed_value;

                return true;
            }

        }
        return false;
    }

    bool ConstEvaluator::evaluateByName(//按名字查找，不存在就计算
        const std::string& name,
        ConstValue& result
    ) {
        auto state_it = states_.find(name);

        if (state_it != states_.end()) {

            if (state_it->second == ConstState::Evaluated) {
                result = values_.at(name);
                return true;
            }

            if (state_it->second == ConstState::Evaluating) {//在计算的过程中再次遇到有占用的
                return false;
            }
        }

        auto symbol_it = value_map.find(name);
        if (symbol_it == value_map.end()) {
            return false;
        }

        if (symbol_it->second.kind != ValueSymbolKind::Constant) {
            return false;
        }
        auto constant = symbol_it->second.constant;
        if (!constant) {
            return false;
        }

        // 在递归计算前标记正在计算
        states_[name] = ConstState::Evaluating;

        ConstValue value;

        if (!evaluateConstant(constant, value)) {
            states_[name] = ConstState::Unvisited;
            return false;
        }

        states_[name] = ConstState::Evaluated;
        values_[name] = value;

        result = value;

        return true;
    }

    const ConstValue* ConstEvaluator::find(//查找名称对应的值
        const std::string& name
    ) const {

        auto it = values_.find(name);
        if (it == values_.end()) {
            return nullptr;
        }
        return &it->second;
    }
}