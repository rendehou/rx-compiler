# 示例 06：把真实 ANTLR Context 转成 AST

## 这一部分连接了什么

前面的 Parse Tree 是我们自己定义的 `ParseNode`。这一部分不再使用它，而是直接调用项目安装的 ANTLR、项目的 `Lexer.g4`/`Parser.g4`，以及生成的：

```cpp
rx::RxLexer
rx::RxParser
rx::RxParser::ExpressionContext
rx::RxParser::AdditiveExpressionContext
rx::RxParser::MultiplicativeExpressionContext
```

输入仍然固定为：

```text
12 + 3 * (4 + 5)
```

输出是与第 03 部分相同的 AST：

```text
Binary "+"
  Integer "12"
  Binary "*"
    Integer "3"
    Binary "+"
      Integer "4"
      Integer "5"
```

## Context 现在是一个具体对象

`Parser.g4` 中有规则：

```antlr
additiveExpression
    : multiplicativeExpression (additiveOperator multiplicativeExpression)*
    ;
```

ANTLR 据此生成了一个普通 C++ 类。它的相关接口大致是：

```cpp
class RxParser::AdditiveExpressionContext {
public:
    std::vector<MultiplicativeExpressionContext*> multiplicativeExpression();
    std::vector<AdditiveOperatorContext*> additiveOperator();
};
```

所以 `Context` 并不是另一种神秘的树。它就是 ANTLR 创建的节点对象；对象中保存了匹配这条规则时得到的孩子，并提供 getter 让我们取出孩子。

对于 `12 + 3`：

```text
multiplicativeExpression()  返回 [表示 12 的 Context, 表示 3 的 Context]
additiveOperator()          返回 [表示 + 的 Context]
```

## 完整流水线

`main.cpp` 完整展示了：

```text
源字符串
→ ANTLRInputStream
→ RxLexer
→ CommonTokenStream
→ RxParser
→ ExpressionContext*
→ buildAst(context)
→ AstNode
→ printAst
```

ANTLR 创建和拥有所有 Context；`AstBuilder` 只在 Parser 生命周期内读取这些指针，然后创建由程序自己拥有的 `AstNode` 值。

## AstBuilder 阅读顺序

完整实现在 `antlr_ast.cpp`：

1. `buildAst()` 接收根 `ExpressionContext*`。
2. `findAdditive()` 穿过 assignment、logical、comparison 等只有一个孩子的优先级包装层。
3. `buildAdditive()` 读取多个 `MultiplicativeExpressionContext*` 和中间的运算符。
4. `buildMultiplicative()` 用同样方式建立乘法节点。
5. `buildPrimary()` 最终读取 `INTEGER_LITERAL()`，或递归处理括号中的 `expression()`。

这个示例有意只接受整数、布尔值、`+`、`-`、`*`、`/`、`%` 和括号。遇到其他合法 Rx 表达式时会报告 `unsupported ... in this example`。扩展完整编译器时，就是为其他 Context 增加相应 AST 节点和构建分支。

## 为什么 G4 不等于 AST

G4 中需要很多层规则来表达优先级，因此 `12` 的 Parse Tree 外面包着 assignment、logical、comparison、shift、additive、multiplicative 等节点。AST 不需要重复保存这些层，只保留：

```text
Integer 12
```

优先级已经体现在 AST 的父子关系中，不需要再保存名为“优先级”的节点。

## 运行

这一部分通过项目根目录的 CMake 链接真正的 ANTLR runtime：

```bash
cd "/home/ark_en/acm/rust compiler/rx-compiler/exercises/06-antlr-bridge"
./check.sh
```

成功时输出：

```text
PASS: ANTLR bridge output matches expected.txt
```

## 阅读目标

- 在 `main.cpp` 找到 Lexer 输出被交给 Parser 的代码。
- 在 `antlr_ast.cpp` 找到第一次接收真实 Context 指针的函数。
- 对照 `Parser.g4` 的 `additiveExpression`，理解两个 vector getter 的来源。
- 找到括号如何递归调用 `buildExpression()`。
- 能说明 ANTLR 负责产生 Parse Tree，而项目代码负责产生 AST。
