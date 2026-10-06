# 示例 03：把 Parse Tree 压缩成 AST

## 输入

`main.cpp` 直接构造了表示下面表达式的 Parse Tree，不依赖前一部分的 Parser：

```text
12 + 3 * (4 + 5)
```

输入树仍然保留所有语法包装层和符号：

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
        ...
      RPAREN ")"
```

## 输出 AST 的具体形式

AST 只保留后续语义检查需要的信息：

```cpp
enum class AstKind {
    Integer,
    Binary,
};

struct AstNode {
    AstKind kind;
    std::string value;
    std::vector<AstNode> children;
};
```

整数节点的 `value` 是原始数字；二元运算节点的 `value` 是运算符，并且恰好有左右两个孩子。

程序的精确输出是：

```text
Binary "+"
  Integer "12"
  Binary "*"
    Integer "3"
    Binary "+"
      Integer "4"
      Integer "5"
```

可以看到，`Expression`、`Term`、`Primary` 和括号节点都消失了。括号完成了控制结合顺序的任务，但后续阶段只需要保存已经确定的树形结构。

## 核心实现

完整代码在 `ast.cpp`，阅读顺序是：

1. `buildPrimary()`：整数变成 `Integer`；括号直接返回内部表达式的 AST。
2. `buildTerm()`：把一个或多个 `Primary` 从左向右组合成乘法 `Binary`。
3. `buildExpression()`：把一个或多个 `Term` 从左向右组合成加法 `Binary`。

关键区别是：输入的 Parse Tree 节点跟随语法规则，输出的 AST 节点跟随后续程序分析所需的含义。

## 运行

```bash
cd "/home/ark_en/acm/rust compiler/rx-compiler/exercises/03-ast-builder"
./check.sh
```

完整代码会输出：

```text
PASS: AST output matches expected.txt
```

## 阅读目标

- 找到 `Primary(INTEGER)` 变成 `AstKind::Integer` 的代码。
- 找到括号节点被丢弃、内部表达式被保留的代码。
- 找到 `Binary("*", left, right)` 被创建的代码。
- 对比 Parse Tree 和 AST，说明 AST 为什么更适合类型检查。
