#pragma once

#include <string>
#include <vector>

struct Node {
    std::string name;
    std::vector<Node> children;
};

void printTree(const Node& node, int depth);
