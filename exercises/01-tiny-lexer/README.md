# 示例 01：把字符串切成 Token

## 工程边界

这是一个独立的 C++ 程序，不使用 ANTLR，也不依赖第 00 题。它只认识下面五类内容：

```text
一位或多位十进制数字    INTEGER
+                       PLUS
*                       STAR
(                       LPAREN
)                       RPAREN
空格、Tab、换行          跳过，不产生 Token
```

扫描完字符串后，还要添加一个表示输入结束的 `END` Token。

## 已经提供的输入

`main.cpp` 已经写死了本题输入，不需要从键盘输入：

```text
12 + 3 * (4 + 5)
```

在内存里，它就是一个 `std::string_view`。每个字符都有从 0 开始的位置：

```text
字符:  1 2   +   3   *   ( 4   +   5 )
位置:  0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
```

## Token 的具体形式

本题没有抽象的“Token 概念”等你猜。它就是 `lexer.hpp` 中这个结构体的一个对象：

```cpp
struct Token {
    TokenKind kind;       // 类别，例如 INTEGER
    std::string text;     // 源代码中的原始文字，例如 "12"
    std::size_t position; // 第一个字符的位置，例如 0
};
```

例如，开头的 `12` 会变成这个具体对象：

```cpp
Token{TokenKind::Integer, "12", 0}
```

`+` 会变成：

```cpp
Token{TokenKind::Plus, "+", 3}
```

函数最终返回的就是 `std::vector<Token>`。

## 核心实现

`lexer.cpp` 中的 `tokenize` 已经完整实现。其他代码负责构造输入、打印结果和测试。

在 `tokenize` 中维护一个位置 `position`，从左向右重复：

1. 如果当前字符是空白，位置加一，不创建 Token。
2. 如果是数字，记住起点，连续读完后创建一个 `Integer` Token。
3. 如果是 `+`、`*`、`(` 或 `)`，创建对应 Token，然后位置加一。
4. 如果是其他字符，抛出 `std::runtime_error`。
5. 全部读完后，添加 `Token{TokenKind::End, "", source.size()}`。

数字必须连续读取。因此 `12` 是一个 Token，不是 `1`、`2` 两个 Token。

## 精确输出

```text
INTEGER text="12" position=0
PLUS text="+" position=3
INTEGER text="3" position=5
STAR text="*" position=7
LPAREN text="(" position=9
INTEGER text="4" position=10
PLUS text="+" position=12
INTEGER text="5" position=14
RPAREN text=")" position=15
END text="" position=16
```

## 编译和自动验收

```bash
cd "/home/ark_en/acm/rust compiler/rx-compiler/exercises/01-tiny-lexer"
./check.sh
```

当前完整代码会输出：

```text
PASS: lexer output matches expected.txt
```

## 阅读目标

- 能指出结果中的每个 `Token` 是在哪一次循环中创建的；
- 能解释 `12` 为什么只有一个 Token；
- 能解释空格为什么没有出现在结果中。

理解这些以后，你已经看完了一个最小 Lexer。ANTLR Lexer 做的是同一种工作，只是它从 `Lexer.g4` 自动获得更多、更复杂的识别规则。
