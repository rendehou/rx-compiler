# 示例 11：内建环境、常量、derive 和递归布局

## 为什么这是四个小模块

这些功能共享全局类型/值表，但算法不同，不应该塞进一个巨型表达式 Visitor：

1. `installBuiltins()`：在用户声明之前预装普通函数签名和受保护类型名。
2. `ConstantEvaluator`：使用 `Unvisited → Visiting → Done` 三态 DFS 支持前向引用并发现常量环。
3. `TraitChecker`：递归查询字段是否支持 Copy 等能力，并检查显式 derive 的依赖。
4. `LayoutChecker`：只沿内联存储边寻找 struct 大小递归；Box、Vec、引用会打断布局环。

## 本例输出表达的规则

- `A` 可以引用后面声明的 `B`。
- `X → Y → X` 在第二次遇到 Visiting 时报告环。
- `Pair` 显式 derive Copy+Clone 且字段都是 i32，因此支持 Copy。
- `Owned` 含 Box 字段，因此不能 Copy。
- `Broken` 只 derive Copy 没有 Clone，是非法 derive 组合。
- `Node { next: Box<Node> }` 布局有限。
- `Bad { next: Bad }` 发生内联布局递归。

Trait 递归图与 Layout 递归图不能混为一谈：Box 会打断布局，但 Box 内元素仍可能参与 Clone/PartialEq 等能力计算。

## 正式工程映射

```text
BuiltinEnvironment     → 全局声明收集开始前安装
ConstantEvaluator      → const item 与数组长度求值
TraitChecker           → Copy/Clone/PartialEq/Eq 查询
LayoutChecker          → struct 收集完成后的独立合法性检查
```

Box/Vec 的 `new`、`push`、`remove`、`len` 等也应作为表驱动的内建关联项，而不是散落在 Call/MethodCall 中的大量名字判断。

## 运行

```bash
cd "/home/ark_en/acm/rust compiler/rx-compiler/exercises/11-constants-traits"
./check.sh
```

成功时显示 `PASS: constants/traits output matches expected.txt`。
