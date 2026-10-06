#include "lexer.hpp"

#include <iostream>
#include <string_view>

int main() {
    constexpr std::string_view source = "12 + 3 * (4 + 5)";

    for (const Token& token : tokenize(source)) {
        std::cout << tokenKindName(token.kind)
                  << " text=\"" << token.text << '"'
                  << " position=" << token.position << '\n';
    }
}
