#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

enum class Type { I32, Bool, Unit };
enum class ExprKind { Integer, Bool, Name, Call };

struct Expr {
    ExprKind kind;
    std::string text;
    std::vector<Expr> children;
};

struct Parameter {
    std::string name;
    Type type;
};

struct Body {
    std::vector<Expr> statements;
    std::optional<Expr> tail;
};

struct Function {
    std::string name;
    std::vector<Parameter> parameters;
    Type result;
    Body body;
};

struct Program { std::vector<Function> functions; };
struct Signature { std::vector<Type> parameters; Type result; };

class SemanticError : public std::runtime_error {
public: using std::runtime_error::runtime_error;
};

std::string_view typeName(Type type) {
    switch (type) {
        case Type::I32: return "i32";
        case Type::Bool: return "bool";
        case Type::Unit: return "unit";
    }
    throw SemanticError("unknown type");
}

class Analyzer {
public:
    explicit Analyzer(std::ostream& output) : output_(output) {}

    void analyze(const Program& program) {
        functions_.clear();
        collectFunctions(program);
        for (const Function& function : program.functions) checkFunction(function);
        checkEntryPoint();
    }

private:
    void collectFunctions(const Program& program) {
        for (const Function& function : program.functions) {
            Signature signature{{}, function.result};
            for (const Parameter& parameter : function.parameters) {
                signature.parameters.push_back(parameter.type);
            }
            if (!functions_.emplace(function.name, signature).second) {
                throw SemanticError("duplicate function: " + function.name);
            }
            output_ << "collect " << function.name << "(";
            for (std::size_t i = 0; i < signature.parameters.size(); ++i) {
                if (i != 0) output_ << ", ";
                output_ << typeName(signature.parameters[i]);
            }
            output_ << ") -> " << typeName(signature.result) << '\n';
        }
    }

    void checkFunction(const Function& function) {
        std::unordered_map<std::string, Type> locals;
        for (const Parameter& parameter : function.parameters) {
            if (!locals.emplace(parameter.name, parameter.type).second) {
                throw SemanticError("duplicate parameter: " + parameter.name);
            }
        }
        for (const Expr& statement : function.body.statements) {
            (void)checkExpr(statement, locals);
        }
        const Type body_type = function.body.tail
            ? checkExpr(*function.body.tail, locals) : Type::Unit;
        if (body_type != function.result) {
            throw SemanticError("body of " + function.name + " has type " +
                                std::string(typeName(body_type)) + ", expected " +
                                std::string(typeName(function.result)));
        }
        output_ << "check " << function.name << ": ok\n";
    }

    Type checkExpr(const Expr& expression,
                   const std::unordered_map<std::string, Type>& locals) {
        if (expression.kind == ExprKind::Integer) return Type::I32;
        if (expression.kind == ExprKind::Bool) return Type::Bool;
        if (expression.kind == ExprKind::Name) {
            const auto found = locals.find(expression.text);
            if (found == locals.end()) {
                throw SemanticError("unknown local name: " + expression.text);
            }
            return found->second;
        }

        if (locals.find(expression.text) != locals.end()) {
            throw SemanticError("local value is not callable: " + expression.text);
        }
        const auto found = functions_.find(expression.text);
        if (found == functions_.end()) {
            throw SemanticError("unknown function: " + expression.text);
        }
        const Signature& signature = found->second;
        if (expression.children.size() != signature.parameters.size()) {
            throw SemanticError("function " + expression.text + " expects " +
                std::to_string(signature.parameters.size()) + " arguments, got " +
                std::to_string(expression.children.size()));
        }
        for (std::size_t i = 0; i < expression.children.size(); ++i) {
            const Type actual = checkExpr(expression.children[i], locals);
            if (actual != signature.parameters[i]) {
                throw SemanticError("wrong argument type in call to " + expression.text);
            }
        }
        return signature.result;
    }

    void checkEntryPoint() {
        const auto found = functions_.find("main");
        if (found == functions_.end()) throw SemanticError("missing main function");
        if (!found->second.parameters.empty() || found->second.result != Type::Unit) {
            throw SemanticError("main must have signature fn main()");
        }
        output_ << "entry main: ok\n";
    }

    std::ostream& output_;
    std::unordered_map<std::string, Signature> functions_;
};

Expr integer(std::string text) { return {ExprKind::Integer, std::move(text), {}}; }
Expr name(std::string text) { return {ExprKind::Name, std::move(text), {}}; }
Expr call(std::string name_text, std::vector<Expr> arguments) {
    return {ExprKind::Call, std::move(name_text), std::move(arguments)};
}

void run(std::string_view title, const Program& program) {
    std::cout << title << '\n';
    try {
        Analyzer(std::cout).analyze(program);
        std::cout << "result: ok\n";
    } catch (const SemanticError& error) {
        std::cout << "result: error: " << error.what() << '\n';
    }
}

int main() {
    const Function add{"add", {{"a", Type::I32}, {"b", Type::I32}},
                       Type::I32, {{}, name("a")}};
    const Function twice{"twice", {{"x", Type::I32}}, Type::I32,
                         {{}, call("add", {name("x"), name("x")})}};
    const Function main_ok{"main", {}, Type::Unit,
                           {{call("twice", {integer("21")})}, std::nullopt}};
    const Function main_bad{"main", {}, Type::Unit,
                            {{call("add", {integer("1")})}, std::nullopt}};

    run("VALID PROGRAM", Program{{twice, add, main_ok}});
    run("WRONG CALL PROGRAM", Program{{add, main_bad}});
}
