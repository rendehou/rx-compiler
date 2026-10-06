#include "tree.hpp"

int main() {
    const Node root{
        "box",
        {
            Node{"apple", {}},
            Node{
                "small-box",
                {
                    Node{"coin", {}},
                    Node{"key", {}},
                },
            },
        },
    };

    printTree(root, 0);
}
