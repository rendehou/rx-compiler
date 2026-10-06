#include "scope.hpp"

#include <iostream>
#include <utility>
#include <vector>

namespace {

AstNode block(std::vector<AstNode> children) {
    return {AstKind::Block, "", Type::Unit, std::move(children)};
}
AstNode let(std::string name, Type type, AstNode initializer) {
    return {AstKind::Let, std::move(name), type, {std::move(initializer)}};
}
AstNode name(std::string text) {
    return {AstKind::Name, std::move(text), Type::Unit, {}};
}
AstNode integer(std::string text) {
    return {AstKind::Integer, std::move(text), Type::Unit, {}};
}
AstNode boolean(std::string text) {
    return {AstKind::Bool, std::move(text), Type::Unit, {}};
}

void run(std::string_view title, const AstNode& root) {
    std::cout << title << '\n';
    ScopeChecker checker(std::cout);
    try {
        checker.check(root);
        std::cout << "result: ok\n";
    } catch (const SemanticError& error) {
        std::cout << "result: error: " << error.what() << '\n';
    }
}

}  // namespace

int main() {
    const AstNode valid = block({
        let("x", Type::I32, integer("10")),
        block({let("x", Type::Bool, boolean("true")), name("x")}),
        name("x"),
    });
    const AstNode duplicate = block({
        let("x", Type::I32, integer("1")),
        let("x", Type::I32, integer("2")),
    });
    const AstNode unknown = block({name("missing")});

    run("VALID PROGRAM", valid);
    run("DUPLICATE PROGRAM", duplicate);
    run("UNKNOWN PROGRAM", unknown);
}
