#include "rx/frontend/parser.hpp"

#include <cstddef>
#include <exception>
#include <utility>

#include "RxLexer.h"
#include "antlr4-runtime.h"

namespace rx::frontend {
namespace {

class CollectingErrorListener final : public antlr4::BaseErrorListener {
public:
    CollectingErrorListener(std::vector<Diagnostic>& diagnostics,
                            const DiagnosticStage stage)
        : diagnostics_(diagnostics), stage_(stage) {}

    void syntaxError(antlr4::Recognizer*, antlr4::Token*, const std::size_t line,
                     const std::size_t column, const std::string& message,
                     std::exception_ptr) override {
        diagnostics_.push_back(
            Diagnostic{stage_, ast::SourceSpan::point(line, column), message});
    }

private:
    std::vector<Diagnostic>& diagnostics_;
    DiagnosticStage stage_;
};

}  // namespace

class ParsedSource::Impl {
public:
    explicit Impl(std::string source_text)
        : source_text_(std::move(source_text)),
          input_(source_text_),
          lexer_(&input_),
          tokens_(&lexer_),
          parser_(&tokens_) {
        CollectingErrorListener lexer_errors(diagnostics_, DiagnosticStage::Lexer);
        CollectingErrorListener parser_errors(diagnostics_, DiagnosticStage::Parser);

        lexer_.removeErrorListeners();
        lexer_.addErrorListener(&lexer_errors);
        parser_.removeErrorListeners();
        parser_.addErrorListener(&parser_errors);

        tree_ = parser_.crate();

        // The listeners above live only for this parse.  Remove their pointers
        // before leaving the constructor so the recognizers never retain
        // dangling references.
        lexer_.removeErrorListeners();
        parser_.removeErrorListeners();
    }

    std::string source_text_;
    std::vector<Diagnostic> diagnostics_;
    antlr4::ANTLRInputStream input_;
    RxLexer lexer_;
    antlr4::CommonTokenStream tokens_;
    RxParser parser_;
    RxParser::CrateContext* tree_ = nullptr;
};

std::unique_ptr<ParsedSource> ParsedSource::parse(std::string source) {
    return std::unique_ptr<ParsedSource>(
        new ParsedSource(std::make_unique<Impl>(std::move(source))));
}

ParsedSource::ParsedSource(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
ParsedSource::~ParsedSource() = default;
ParsedSource::ParsedSource(ParsedSource&&) noexcept = default;
ParsedSource& ParsedSource::operator=(ParsedSource&&) noexcept = default;

bool ParsedSource::ok() const noexcept {
    return impl_->tree_ != nullptr && impl_->diagnostics_.empty();
}

RxParser::CrateContext* ParsedSource::tree() const noexcept {
    return impl_->tree_;
}

const std::vector<Diagnostic>& ParsedSource::diagnostics() const noexcept {
    return impl_->diagnostics_;
}

const std::string& ParsedSource::source() const noexcept {
    return impl_->source_text_;
}

std::string ParsedSource::treeString(const bool pretty) const {
    if (impl_->tree_ == nullptr) {
        return {};
    }
    return impl_->tree_->toStringTree(&impl_->parser_, pretty);
}

}  // namespace rx::frontend
