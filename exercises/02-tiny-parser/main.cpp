#include "parser.hpp"

#include <exception>
#include <iostream>
#include <utility>
#include <vector>

int main() {
    std::vector<Token> tokens{
        {TokenKind::Integer, "12"},
        {TokenKind::Plus, "+"},
        {TokenKind::Integer, "3"},
        {TokenKind::Star, "*"},
        {TokenKind::LeftParen, "("},
        {TokenKind::Integer, "4"},
        {TokenKind::Plus, "+"},
        {TokenKind::Integer, "5"},
        {TokenKind::RightParen, ")"},
        {TokenKind::End, ""},
    };

    try {
        Parser parser(std::move(tokens));
        const ParseNode root = parser.parse();
        printParseTree(root);
    } catch (const std::exception& error) {
        std::cerr << "parser error: " << error.what() << '\n';
        return 1;
    }
}
