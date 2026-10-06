# 示例 00：打印一棵普通的树

## 这不是编译器题

本题只有普通 C++ 数据结构，不使用 ANTLR、Token、Parser、AST 或项目文法。目的只是让你亲手确认“树”在程序里究竟是什么。

## 已经提供的输入

`main.cpp` 已经构造了下面这个对象，运行程序时不需要输入任何内容：

```cpp
Node root{
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
```

`Node` 的真实定义就在 `tree.hpp`：

```cpp
struct Node {
    std::string name;
    std::vector<Node> children;
};
```

因此 `root` 是一个节点，它的名字是 `box`，它的 `children` 数组里有
`apple` 和 `small-box` 两个节点。`small-box` 自己又有两个孩子。

## 核心实现

`tree.cpp` 中的 `printTree` 已经完整实现。

函数收到两个具体值：

- `node`：当前正在打印的节点；
- `depth`：当前节点在第几层，根节点为第 0 层。

它必须依次完成：

1. 输出 `depth * 2` 个空格；
2. 输出当前节点的 `name` 和换行；
3. 对 `node.children` 中的每个孩子调用 `printTree(child, depth + 1)`。

这里的“递归”没有隐藏含义，就是函数在处理孩子时再次调用自己。

## 精确输出

完成后，程序输出必须逐字符等于：

```text
box
  apple
  small-box
    coin
    key
```

缩进是空格：第二层前面 2 个，第三层前面 4 个。

## 编译和自动验收

在当前目录运行：

```bash
./check.sh
```

当前代码运行后，最后一行会显示：

```text
PASS: tree output matches expected.txt
```

## 阅读目标

阅读后应能说明：函数先访问当前节点，再逐个访问它的孩子，直到没有孩子；并能指出 `main.cpp` 中的 `box`、`small-box` 和 `coin` 分别是哪一个实际 C++ 对象。
