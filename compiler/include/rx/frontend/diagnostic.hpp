#pragma once

#include <cstddef>
#include <string>

#include "rx/ast/source_span.hpp"

namespace rx::frontend {

enum class DiagnosticStage {
    Lexer,
    Parser,
    AstBuilder,
    Semantic,
};

struct Diagnostic {
    DiagnosticStage stage = DiagnosticStage::Parser;
    ast::SourceSpan span{};
    std::string message;
};

const char* stageName(DiagnosticStage stage) noexcept;
std::string formatDiagnostic(const Diagnostic& diagnostic,
                             const std::string& source_name);

}  // namespace rx::frontend
