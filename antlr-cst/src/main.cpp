#include "RxLexer.h"
#include "RxParser.h"
#include "antlr4-runtime.h"

#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    const std::string input_path = argc > 1 ? argv[1] : "samples/minimal.rx";
    std::ifstream source_file(input_path);
    if (!source_file) {
        std::cerr << "rx-cst: cannot open input file: " << input_path << '\n';
        return 1;
    }

    antlr4::ANTLRInputStream input(source_file);
    RxLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    RxParser parser(&tokens);

    antlr4::tree::ParseTree* tree = parser.crate();
    std::cout << tree->toStringTree(&parser) << '\n';
    return parser.getNumberOfSyntaxErrors() == 0 ? 0 : 1;
}
