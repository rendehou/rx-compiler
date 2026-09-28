#pragma once

#include <memory>
#include <string>
#include <vector>

#include "RxParser.h"
#include "rx/frontend/diagnostic.hpp"

namespace rx::frontend {

// Owns every ANTLR object on which the returned CrateContext depends.
// A tree pointer becomes invalid when its ParsedSource is destroyed.
class ParsedSource final {
public:
    static std::unique_ptr<ParsedSource> parse(std::string source);

    ~ParsedSource();
    ParsedSource(ParsedSource&&) noexcept;
    ParsedSource& operator=(ParsedSource&&) noexcept;

    ParsedSource(const ParsedSource&) = delete;
    ParsedSource& operator=(const ParsedSource&) = delete;

    [[nodiscard]] bool ok() const noexcept;
    [[nodiscard]] RxParser::CrateContext* tree() const noexcept;
    [[nodiscard]] const std::vector<Diagnostic>& diagnostics() const noexcept;
    [[nodiscard]] const std::string& source() const noexcept;
    [[nodiscard]] std::string treeString(bool pretty = false) const;

private:
    class Impl;
    explicit ParsedSource(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_;
};

}  // namespace rx::frontend

