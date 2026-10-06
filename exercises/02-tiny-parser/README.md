# 示例 02：把 Token 数组组织成树

## 工程边界

这是一个独立程序，不调用第 01 题的 Lexer。`main.cpp` 已经直接构造好 Token 数组，因此本题没有字符串扫描问题。

本题只学习 Parser 的一个动作：查看当前位置的 Token，按照规则创建节点，并把递归结果放进 `children`。

## 已经提供的输入

`main.cpp` 中的输入是这些真实 C++ 对象：

```cpp
std::vector<Token>{
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
}
```

它表示：

```text
12 + 3 * (4 + 5)
```

Parser 不会接触原始字符串，只会读取这个数组。

## 输出节点的具体形式

Parser 创建的每个节点都是：

```cpp
struct ParseNode {
    ParseKind kind;
    std::string text;
    std::vector<ParseNode> children;
};
```

例如，Token `12` 会成为：

```cpp
ParseNode{ParseKind::IntegerToken, "12", {}}
```

一个只包含 `12` 的 `Primary` 节点会是：

```cpp
ParseNode{
    ParseKind::Primary,
    "",
    {ParseNode{ParseKind::IntegerToken, "12", {}}},
}
```

这就是本题所说的“树”：一个 `ParseNode` 的 `children` 里保存着另外一些 `ParseNode`。

## 本题仅有的三条语法规则

```text
Expression = Term (PLUS Term)*
Term       = Primary (STAR Primary)*
Primary    = INTEGER | LPAREN Expression RPAREN
```

读法如下：

- `Expression`：先读一个 `Term`；后面每出现一次 `PLUS`，就再读一个 `Term`。
- `Term`：先读一个 `Primary`；后面每出现一次 `STAR`，就再读一个 `Primary`。
- `Primary`：要么读取一个整数；要么依次读取左括号、一个完整 `Expression`、右括号。

因为 `Expression` 调用 `Term`，而 `Term` 会先处理完所有乘法，所以乘法自然比加法结合得更紧。

## 核心实现

`parser.cpp` 已经完整实现下面三个函数：

```cpp
ParseNode Parser::parseExpression();
ParseNode Parser::parseTerm();
ParseNode Parser::parsePrimary();
```

已经提供的操作：

- `peek()`：查看当前位置的 Token，但不前进；
- `consume(expected)`：取得当前位置的 Token，将位置向后移动一格，并检查类型；
- `tokenNode(token)`：把一个 Token 转成没有孩子的 ParseNode。

阅读时先看 `parseExpression()`，再看 `parseTerm()`，最后看 `parsePrimary()`；之后再看 `peek()` 和 `consume()` 如何管理当前位置。

## 精确输出

```text
Expression
  Term
    Primary
      INTEGER "12"
  PLUS "+"
  Term
    Primary
      INTEGER "3"
    STAR "*"
    Primary
      LPAREN "("
      Expression
        Term
          Primary
            INTEGER "4"
        PLUS "+"
        Term
          Primary
            INTEGER "5"
      RPAREN ")"
```

注意：括号、`Expression`、`Term` 和 `Primary` 全都保留在输出中。这是一棵贴近语法规则的 Parse Tree，还没有压缩成 AST。

## 编译和自动验收

```bash
cd "/home/ark_en/acm/rust compiler/rx-compiler/exercises/02-tiny-parser"
./check.sh
```

当前完整代码会输出：

```text
PASS: parser output matches expected.txt
```

## 阅读目标

- 能指出根节点就是 `parseExpression()` 返回的那个具体对象；
- 能指出 `3 * (...)` 为什么一起位于第二个 `Term` 的 `children` 中；
- 能指出处理括号时，`parsePrimary()` 在什么地方递归调用了 `parseExpression()`。
