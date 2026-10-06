#include "antlr_ast.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <string>

#include "RxLexer.h"
#include "RxParser.h"
#include "antlr4-runtime.h"

namespace {

class CountingErrorListener final : public antlr4::BaseErrorListener {
public:
    void syntaxError(antlr4::Recognizer*, antlr4::Token*, std::size_t,
                     std::size_t, const std::string&, std::exception_ptr) override {
        ++count_;
    }

    std::size_t count() const { return count_; }

private:
    std::size_t count_ = 0;
};

}  // namespace

int main() {
    try {
        const std::string source = "12 + 3 * (4 + 5)";
        antlr4::ANTLRInputStream input(source);
        rx::RxLexer lexer(&input);
        CountingErrorListener errors;
        lexer.removeErrorListeners();
        lexer.addErrorListener(&errors);

        antlr4::CommonTokenStream tokens(&lexer);
        rx::RxParser parser(&tokens);
        parser.removeErrorListeners();
        parser.addErrorListener(&errors);

        rx::RxParser::ExpressionContext* parse_tree = parser.expression();
        if (errors.count() != 0 || tokens.LA(1) != antlr4::Token::EOF) {
            std::cerr << "ANTLR rejected the example input\n";
            return 1;
        }

        const AstNode ast = buildAst(parse_tree);
        printAst(ast);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ANTLR bridge error: " << error.what() << '\n';
        return 1;
    }
}
