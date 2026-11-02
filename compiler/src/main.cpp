#include "antlr4-runtime.h"
#include "RxLexer.h"
#include "RxParser.h"

#include "rx/frontend/astbuilder.hpp"
#include "rx/semantic/checker.hpp"

#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

class SyntaxErrorListener final : public antlr4::BaseErrorListener {
public:
    void syntaxError(
        antlr4::Recognizer*,
        antlr4::Token*,
        std::size_t line,
        std::size_t column,
        const std::string& message,
        std::exception_ptr
    ) override {
        has_error_ = true;
        std::cerr << line << ':' << column + 1 << ": " << message << '\n';
    }

    [[nodiscard]] bool hasError() const noexcept {
        return has_error_;
    }

private:
    bool has_error_ = false;
};

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: rx_compiler_cli <source.rx>\n";
        return 1;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "cannot open source file: " << argv[1] << '\n';
        return 1;
    }

    const std::string source(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );

    try {
        antlr4::ANTLRInputStream input(source);
        RxLexer lexer(&input);
        SyntaxErrorListener errors;
        lexer.removeErrorListeners();
        lexer.addErrorListener(&errors);

        antlr4::CommonTokenStream tokens(&lexer);
        RxParser parser(&tokens);
        parser.removeErrorListeners();
        parser.addErrorListener(&errors);

        auto* syntax_tree = parser.crate();
        if (errors.hasError() || parser.getNumberOfSyntaxErrors() != 0) {
            return 1;
        }

        rx::frontend::AstBuilder ast_builder;
        auto program = ast_builder.build(syntax_tree);
        if (!program) {
            return 1;
        }

        rx::semantic::Checker checker;
        return checker.checkProgram(program) ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "compiler error: " << error.what() << '\n';
        return 1;
    }
}
