# 示例 04：从 AST 计算表达式类型

## 输入和输出

`main.cpp` 直接构造两棵 AST，不依赖 Parser 或 AST Builder：

```text
合法：(1 + 2) == 3       → bool
非法：true + 1           → 类型错误
```

程序的精确输出是：

```text
valid: bool
invalid: operator "+" requires (i32, i32), got (bool, i32)
```

## 核心实现

完整代码在 `semantic.cpp` 的 `checkType()`：

1. `Integer` 直接返回 `Type::I32`，`Bool` 直接返回 `Type::Bool`。
2. `Binary` 先递归检查左右孩子，再根据运算符检查两个类型。
3. `+`、`*` 要求两个 `i32`，结果为 `i32`。
4. `&&` 要求两个 `bool`，结果为 `bool`。
5. `==` 要求左右类型相同，结果为 `bool`。
6. 不满足规则时抛出 `SemanticError`。

“自底向上”就是父节点先调用 `checkType(left)` 和 `checkType(right)`，取得孩子类型后再计算自己的类型。`true + 1` 可以通过 Parser，但会在这里被拒绝，这就是语法检查与语义检查的区别。

## 运行

```bash
cd "/home/ark_en/acm/rust compiler/rx-compiler/exercises/04-type-checker"
./check.sh
```

成功时显示：

```text
PASS: type checker output matches expected.txt
```

阅读时依次找到：基础类型返回、左右孩子递归、运算符规则、错误抛出。
