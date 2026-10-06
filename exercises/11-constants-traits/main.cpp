#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class SemanticError : public std::runtime_error {
public: using std::runtime_error::runtime_error;
};

struct Signature { std::string text; };

std::unordered_map<std::string, Signature> installBuiltins() {
    return {
        {"get_i32", {"() -> i32"}},
        {"print_i32", {"(i32) -> unit"}},
        {"println_i32", {"(i32) -> unit"}},
        {"Box::new", {"(T) -> Box<T>"}},
        {"Vec::push", {"(&mut Vec<T>, T) -> unit"}},
    };
}

enum class Visit { Unvisited, Visiting, Done };
struct ConstExpr { bool is_literal; std::int32_t value; std::string reference; };
struct Constant { ConstExpr expression; Visit state = Visit::Unvisited; std::int32_t value = 0; };

class ConstantEvaluator {
public:
    void addLiteral(std::string name, std::int32_t value) {
        constants_.emplace(std::move(name), Constant{{true, value, ""}});
    }
    void addReference(std::string name, std::string target) {
        constants_.emplace(std::move(name), Constant{{false, 0, std::move(target)}});
    }

    std::int32_t evaluate(const std::string& name) {
        const auto found = constants_.find(name);
        if (found == constants_.end()) throw SemanticError("unknown constant: " + name);
        Constant& constant = found->second;
        if (constant.state == Visit::Done) return constant.value;
        if (constant.state == Visit::Visiting) {
            throw SemanticError("constant cycle at " + name);
        }
        constant.state = Visit::Visiting;
        constant.value = constant.expression.is_literal
            ? constant.expression.value : evaluate(constant.expression.reference);
        constant.state = Visit::Done;
        return constant.value;
    }

private:
    std::unordered_map<std::string, Constant> constants_;
};

enum class Storage { I32, InlineStruct, Box, Vec, Reference };
struct Field { Storage storage; std::string target; };
struct StructDef {
    std::vector<Field> fields;
    std::unordered_set<std::string> derives;
};

class TypeRules {
public:
    void add(std::string name, StructDef definition) {
        structs_.emplace(std::move(name), std::move(definition));
    }

    bool supportsCopy(const std::string& name) {
        std::unordered_set<std::string> visiting;
        return supportsCopyStruct(name, visiting);
    }

    void checkLayout(const std::string& name) {
        std::unordered_map<std::string, Visit> states;
        checkLayoutDfs(name, states);
    }

private:
    bool supportsCopyStruct(const std::string& name,
                            std::unordered_set<std::string>& visiting) {
        const StructDef& definition = find(name);
        if (definition.derives.find("Copy") == definition.derives.end()) return false;
        if (definition.derives.find("Clone") == definition.derives.end()) {
            throw SemanticError(name + " derives Copy without Clone");
        }
        if (!visiting.insert(name).second) return true; // 防止能力查询无限递归
        for (const Field& field : definition.fields) {
            bool field_copy = false;
            switch (field.storage) {
                case Storage::I32:
                case Storage::Reference:
                    field_copy = true;
                    break;
                case Storage::InlineStruct:
                    field_copy = supportsCopyStruct(field.target, visiting);
                    break;
                case Storage::Box:
                case Storage::Vec:
                    field_copy = false;
                    break;
            }
            if (!field_copy) {
                visiting.erase(name);
                return false;
            }
        }
        visiting.erase(name);
        return true;
    }

    void checkLayoutDfs(const std::string& name,
                        std::unordered_map<std::string, Visit>& states) {
        Visit& state = states[name];
        if (state == Visit::Done) return;
        if (state == Visit::Visiting) {
            throw SemanticError("inline layout cycle at " + name);
        }
        state = Visit::Visiting;
        for (const Field& field : find(name).fields) {
            if (field.storage == Storage::InlineStruct) {
                checkLayoutDfs(field.target, states);
            }
            // Box、Vec、Reference 间接保存，不沿这些边计算 inline layout。
        }
        state = Visit::Done;
    }

    const StructDef& find(const std::string& name) const {
        const auto found = structs_.find(name);
        if (found == structs_.end()) throw SemanticError("unknown struct: " + name);
        return found->second;
    }

    std::unordered_map<std::string, StructDef> structs_;
};

template <typename Action>
void show(std::string_view label, Action action) {
    try {
        const std::string result = action();
        std::cout << label << ": " << result << '\n';
    }
    catch (const SemanticError& error) {
        std::cout << label << ": error: " << error.what() << '\n';
    }
}

int main() {
    const auto builtins = installBuiltins();
    std::cout << "builtin get_i32: " << builtins.at("get_i32").text << '\n';
    std::cout << "builtin Vec::push: " << builtins.at("Vec::push").text << '\n';

    ConstantEvaluator constants;
    constants.addReference("A", "B");
    constants.addLiteral("B", 7);
    constants.addReference("X", "Y");
    constants.addReference("Y", "X");
    std::cout << "constant A: " << constants.evaluate("A") << '\n';
    show("constant X", [&] { return std::to_string(constants.evaluate("X")); });

    TypeRules rules;
    rules.add("Pair", {{{Storage::I32, ""}, {Storage::I32, ""}}, {"Copy", "Clone"}});
    rules.add("Owned", {{{Storage::Box, ""}}, {"Copy", "Clone"}});
    rules.add("Broken", {{{Storage::I32, ""}}, {"Copy"}});
    rules.add("Node", {{{Storage::Box, "Node"}}, {}});
    rules.add("Bad", {{{Storage::InlineStruct, "Bad"}}, {}});

    std::cout << "Pair Copy: " << (rules.supportsCopy("Pair") ? "yes" : "no") << '\n';
    std::cout << "Owned Copy: " << (rules.supportsCopy("Owned") ? "yes" : "no") << '\n';
    show("Broken derive", [&] {
        return std::string(rules.supportsCopy("Broken") ? "yes" : "no");
    });
    show("Node layout", [&] { rules.checkLayout("Node"); return std::string("finite"); });
    show("Bad layout", [&] { rules.checkLayout("Bad"); return std::string("finite"); });
}
