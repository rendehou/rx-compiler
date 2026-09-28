#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

#include "rx/frontend/diagnostic.hpp"
#include "rx/frontend/parser.hpp"

namespace {

std::string readSource(const std::string& path) {
    if (path == "-") {
        return {std::istreambuf_iterator<char>(std::cin),
                std::istreambuf_iterator<char>()};
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open source file: " + path);
    }
    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

void printUsage(const char* program) {
    std::cerr << "usage: " << program
              << " (--dump-parse-tree|--check-syntax) <source.rx|->\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        printUsage(argv[0]);
        return 2;
    }

    const std::string mode = argv[1];
    const std::string source_name = argv[2];
    if (mode != "--dump-parse-tree" && mode != "--check-syntax") {
        printUsage(argv[0]);
        return 2;
    }

    try {
        auto parsed = rx::frontend::ParsedSource::parse(readSource(source_name));
        for (const auto& diagnostic : parsed->diagnostics()) {
            std::cerr << rx::frontend::formatDiagnostic(diagnostic, source_name)
                      << '\n';
        }
        if (!parsed->ok()) {
            return 1;
        }
        if (mode == "--dump-parse-tree") {
            std::cout << parsed->treeString() << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << source_name << ": error[driver]: " << error.what() << '\n';
        return 2;
    }
}

