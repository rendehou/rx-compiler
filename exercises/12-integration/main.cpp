#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

class CompileError : public std::runtime_error {
public: using std::runtime_error::runtime_error;
};

struct Source {
    std::string name;
    bool syntax_valid;
    bool has_main;
    bool semantics_valid;
};

struct ParseTree { const Source* source; };
struct Program { const Source* source; };
struct Symbols { const Source* source; };

class ParserStage {
public:
    explicit ParserStage(std::ostream& output) : output_(output) {}
    ParseTree parse(const Source& source) const {
        output_ << "lexer/parser\n";
        if (!source.syntax_valid) throw CompileError("syntax error; stop before AST");
        return ParseTree{&source};
    }
private: std::ostream& output_;
};

class AstBuilder {
public:
    explicit AstBuilder(std::ostream& output) : output_(output) {}
    Program build(const ParseTree& tree) const {
        output_ << "ast builder\n";
        return Program{tree.source};
    }
private: std::ostream& output_;
};

class DeclarationCollector {
public:
    explicit DeclarationCollector(std::ostream& output) : output_(output) {}
    Symbols collect(const Program& program) const {
        output_ << "declaration collector\n";
        if (!program.source->has_main) throw CompileError("missing main function");
        return Symbols{program.source};
    }
private: std::ostream& output_;
};

class SemanticChecker {
public:
    explicit SemanticChecker(std::ostream& output) : output_(output) {}
    void check(const Program& program, const Symbols& symbols) const {
        (void)symbols;
        output_ << "semantic checker\n";
        if (!program.source->semantics_valid) {
            throw CompileError("type mismatch");
        }
    }
private: std::ostream& output_;
};

int compile(const Source& source, std::ostream& output) {
    output << source.name << '\n';
    try {
        const ParseTree tree = ParserStage(output).parse(source);
        const Program program = AstBuilder(output).build(tree);
        const Symbols symbols = DeclarationCollector(output).collect(program);
        SemanticChecker(output).check(program, symbols);
        output << "exit 0\n";
        return 0;
    } catch (const CompileError& error) {
        output << "error: " << error.what() << '\n';
        output << "exit 1\n";
        return 1;
    }
}

int main() {
    (void)compile(Source{"good.rx", true, true, true}, std::cout);
    (void)compile(Source{"syntax-bad.rx", false, true, true}, std::cout);
    (void)compile(Source{"semantic-bad.rx", true, true, false}, std::cout);
}
