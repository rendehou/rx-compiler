# 示例 09：Struct、数组、Place 和赋值

## 为什么 Type 已经不够

检查 `x = 2` 时，只知道左边是 `i32` 不够；还必须知道它是否对应一块可写存储。因此正式表达式检查应返回：

```cpp
ExprInfo {
    Type type;
    Category category; // Value 或 Place
    bool mutable_place;
}
```

字面量、算术结果和函数返回值通常是 Value；局部变量是 Place；字段和下标会沿 base 继承 Place 与可变性。

## 本例覆盖的正式规则

- Struct 用名字比较，字段形状相同的 `Point` 与 `Pair` 仍是不同类型。
- struct 初始化拒绝缺失、重复、未知或错误类型字段。
- 数组类型同时包含元素类型和长度。
- 数组下标要求 `usize`。
- `field()` 与 `index()` 沿 base 传播 Place 信息。
- `assign()` 要求左侧是 mutable Place、左右类型相同，结果为 unit。

示例接受对 `mut Point` 字段的赋值，拒绝把算术 Value 当目标、拒绝修改不可变数组元素，并拒绝缺字段的 struct 初始化。

## 正式工程映射

```text
Type/StructId/ArrayType       → compiler/include/semantic/type.*
ExprInfo                      → SemanticChecker::checkExpr 的返回值
structConstruct/field/index   → 各 AST 表达式检查分支
assign                        → 普通与复合赋值检查分支
```

## 运行

```bash
cd "/home/ark_en/acm/rust compiler/rx-compiler/exercises/09-aggregates-place"
./check.sh
```

成功时显示 `PASS: aggregate/place output matches expected.txt`。
