#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

enum class Type { I32, Bool, Unit, Never };
enum class Kind { Integer, Bool, Unit, Block, If, Return, Loop, While, Break, Continue };

struct Expr {
    Kind kind;
    std::vector<Expr> children;
    bool has_tail = false; // 只对 Block 有意义
};

class SemanticError : public std::runtime_error {
public: using std::runtime_error::runtime_error;
};

std::string_view typeName(Type type) {
    switch (type) {
        case Type::I32: return "i32";
        case Type::Bool: return "bool";
        case Type::Unit: return "unit";
        case Type::Never: return "never";
    }
    throw SemanticError("unknown type");
}

struct LoopContext {
    bool allow_value;
    std::optional<Type> break_type;
};

class Checker {
public:
    explicit Checker(Type function_result) : function_result_(function_result) {}

    Type check(const Expr& expression, std::optional<Type> expected = std::nullopt) {
        const Type actual = checkCore(expression);
        if (expected && actual != Type::Never && actual != *expected) {
            throw SemanticError("expected " + std::string(typeName(*expected)) +
                                ", got " + std::string(typeName(actual)));
        }
        return actual;
    }

private:
    static Type commonType(Type left, Type right) {
        if (left == Type::Never) return right;
        if (right == Type::Never) return left;
        if (left == right) return left;
        throw SemanticError("incompatible branch types: " +
                            std::string(typeName(left)) + " and " +
                            std::string(typeName(right)));
    }

    Type checkCore(const Expr& expression) {
        switch (expression.kind) {
            case Kind::Integer: return Type::I32;
            case Kind::Bool: return Type::Bool;
            case Kind::Unit: return Type::Unit;

            case Kind::Block: {
                Type last = Type::Unit;
                for (const Expr& child : expression.children) {
                    last = check(child); // Never 后面的节点仍继续检查
                }
                return expression.has_tail && !expression.children.empty()
                    ? last : Type::Unit;
            }

            case Kind::If: {
                if (expression.children.size() < 2 || expression.children.size() > 3) {
                    throw SemanticError("malformed if expression");
                }
                check(expression.children[0], Type::Bool);
                const Type then_type = check(expression.children[1]);
                if (expression.children.size() == 2) {
                    if (then_type != Type::Unit && then_type != Type::Never) {
                        throw SemanticError("if without else must produce unit");
                    }
                    return Type::Unit;
                }
                return commonType(then_type, check(expression.children[2]));
            }

            case Kind::Return: {
                const Type value = expression.children.empty()
                    ? Type::Unit : check(expression.children[0], function_result_);
                (void)value;
                if (expression.children.empty() && function_result_ != Type::Unit) {
                    throw SemanticError("return without value in non-unit function");
                }
                return Type::Never;
            }

            case Kind::While: {
                if (expression.children.size() != 2) throw SemanticError("malformed while");
                check(expression.children[0], Type::Bool); // 压栈前检查 condition
                loops_.push_back(LoopContext{false, std::nullopt});
                check(expression.children[1], Type::Unit);
                loops_.pop_back();
                return Type::Unit;
            }

            case Kind::Loop: {
                if (expression.children.size() != 1) throw SemanticError("malformed loop");
                loops_.push_back(LoopContext{true, std::nullopt});
                check(expression.children[0], Type::Unit);
                const std::optional<Type> result = loops_.back().break_type;
                loops_.pop_back();
                return result.value_or(Type::Never);
            }

            case Kind::Break: {
                if (loops_.empty()) throw SemanticError("break outside loop");
                LoopContext& loop = loops_.back();
                const Type value = expression.children.empty()
                    ? Type::Unit : check(expression.children[0]);
                if (!loop.allow_value && value != Type::Unit) {
                    throw SemanticError("while does not allow break value");
                }
                loop.break_type = loop.break_type
                    ? commonType(*loop.break_type, value) : value;
                return Type::Never;
            }

            case Kind::Continue:
                if (loops_.empty()) throw SemanticError("continue outside loop");
                return Type::Never;
        }
        throw SemanticError("unknown expression");
    }

    Type function_result_;
    std::vector<LoopContext> loops_;
};

Expr leaf(Kind kind) { return Expr{kind, {}, false}; }
Expr block(std::vector<Expr> children, bool has_tail) {
    return Expr{Kind::Block, std::move(children), has_tail};
}
Expr node(Kind kind, std::vector<Expr> children) {
    return Expr{kind, std::move(children), false};
}

template <typename Action>
void show(std::string_view label, Action action) {
    try {
        const Type result = action();
        std::cout << label << ": " << typeName(result) << '\n';
    } catch (const SemanticError& error) {
        std::cout << label << ": error: " << error.what() << '\n';
    }
}

int main() {
    const Expr if_with_return = node(Kind::If, {
        leaf(Kind::Bool),
        block({node(Kind::Return, {leaf(Kind::Integer)})}, true),
        block({leaf(Kind::Integer)}, true),
    });
    const Expr loop_value = node(Kind::Loop, {
        block({node(Kind::Break, {leaf(Kind::Integer)})}, false),
    });
    const Expr outside_break = node(Kind::Break, {});
    const Expr bad_while = node(Kind::While, {
        leaf(Kind::Bool),
        block({node(Kind::Break, {leaf(Kind::Integer)})}, false),
    });
    const Expr bad_if = node(Kind::If, {
        leaf(Kind::Bool), block({leaf(Kind::Integer)}, true),
    });

    show("if with return", [&] { return Checker(Type::I32).check(if_with_return, Type::I32); });
    show("loop with value", [&] { return Checker(Type::Unit).check(loop_value); });
    show("break outside", [&] { return Checker(Type::Unit).check(outside_break); });
    show("while break value", [&] { return Checker(Type::Unit).check(bad_while); });
    show("if without else", [&] { return Checker(Type::Unit).check(bad_if); });
}
