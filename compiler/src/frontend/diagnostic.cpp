#include "rx/frontend/diagnostic.hpp"

#include <sstream>

namespace rx::frontend {

const char* stageName(const DiagnosticStage stage) noexcept {
    switch (stage) {
        case DiagnosticStage::Lexer: return "lexer";
        case DiagnosticStage::Parser: return "parser";
        case DiagnosticStage::AstBuilder: return "ast-builder";
        case DiagnosticStage::Semantic: return "semantic";
    }
    return "unknown";
}

std::string formatDiagnostic(const Diagnostic& diagnostic,
                             const std::string& source_name) {
    std::ostringstream output;
    output << source_name;
    if (diagnostic.span.valid()) {
        // Internally columns are zero-based (ANTLR convention); users expect
        // the first character of a line to be column 1.
        output << ':' << diagnostic.span.begin.line << ':'
               << diagnostic.span.begin.column + 1;
    }
    output << ": error[" << stageName(diagnostic.stage) << "]: "
           << diagnostic.message;
    return output.str();
}

}  // namespace rx::frontend
