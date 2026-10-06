#include "tree.hpp"

#include <iostream>

void printTree(const Node& node, int depth) {
    std::cout << std::string(static_cast<std::size_t>(depth * 2), ' ')
              << node.name << '\n';

    for (const Node& child : node.children) {
        printTree(child, depth + 1);
    }
}
