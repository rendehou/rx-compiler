# 可运行的编译器前端分步示例

> 正式作业请从 [第一阶段总体说明](../compiler/STAGE1_GUIDE.md) 开始。这些独立示例只用于理解动作，不是完整 Rx 实现；示例 PASS 不代表官方测试通过。05 的同层 let 重名拒绝不是 Rx 规则；08 的控制流／Never 是简化模型；10～11 未覆盖完整引用、方法和能力规则；12 使用真假标记模拟阶段结果，并未真的解析和检查 Rx 文件。不要原样复制这些语义分支进正式编译器。

这里的代码和 `rx-compiler` 的实现相互独立。每一部分都提供：

- 已经写好的具体输入；
- 可以直接编译的起始代码；
- 已经完成、可以直接阅读的实现；
- 完全确定的期望输出；
- 一条自动验收命令；
- 不依赖上一题的代码。

第 00～05 部分不需要打开 `grammar/`，也不会出现 ANTLR 的 `ParseTreeContext`。示例自身的演示代码已写完，不再保留 TODO；这不表示对应 Rx 模块完整。

| 题目 | 唯一要学的操作 | 输入 | 输出 |
| --- | --- | --- | --- |
| [00 普通树](00-tree/README.md)（答案已提供） | 递归访问一个节点及其孩子 | 已构造好的 `Node` 对象 | 缩进后的整棵树 |
| [01 微型 Lexer](01-tiny-lexer/README.md)（答案已提供） | 从字符串切出带类型的小块 | `"12 + 3 * (4 + 5)"` | `INTEGER(12) PLUS ...` |
| [02 微型 Parser](02-tiny-parser/README.md)（答案已提供） | 按优先级组合 Token | 已构造好的 Token 数组 | 一棵具体语法树 |
| [03 微型 AST Builder](03-ast-builder/README.md)（答案已提供） | 删除语法树中无意义的包装层 | 已构造好的具体语法树对象 | `Binary/Integer` AST |
| [04 微型类型检查](04-type-checker/README.md)（答案已提供） | 从孩子的类型计算父节点类型 | 已构造好的 AST 对象 | `i32`、`bool` 或固定错误 |
| [05 变量和作用域](05-scopes/README.md)（答案已提供） | 在进入/离开块时维护名字表 | 已构造好的 Block AST | 名字解析过程或固定错误 |
| [06 ANTLR 接口](06-antlr-bridge/README.md)（答案已提供） | 把真实 Context 翻译为已认识的对象 | 一段 Rx 源码 | 与第 03 部分相同的 AST |
| [07 程序与函数](07-program-functions/README.md)（答案已提供） | 两遍扫描、函数调用、main 规则 | 已构造的 Program AST | 全局签名与函数体检查 |
| [08 控制流](08-control-flow/README.md)（答案已提供） | Block tail、Never、if、循环上下文 | 已构造的控制流 AST | 结果类型或语义错误 |
| [09 聚合与 Place](09-aggregates-place/README.md)（答案已提供） | struct、数组、字段、下标、赋值目标 | 类型表与表达式信息 | `Type + Value/Place + mutable` |
| [10 引用与方法](10-references-methods/README.md)（答案已提供） | 借用、解引用、coercion、receiver 调整 | Type 与 ExprInfo | 引用访问能力或错误 |
| [11 常量与能力](11-constants-traits/README.md)（答案已提供） | 内建表、常量 DFS、derive、递归布局 | 全局声明表 | 求值结果或图错误 |
| [12 正式集成](12-integration/README.md)（答案已提供） | 组织编译阶段并在错误处停止 | 三个小程序 | 退出码与阶段记录 |

第 00～05 部分是在隔离环境中理解动作，第 06 部分才把动作接到项目。这样，看到
`RxParser::...Context` 时，只需要学习“如何从这个对象取孩子”，不需要同时第一次理解树、递归、AST 和类型检查。

建议按编号阅读。每部分都可以运行 `./check.sh`，确认当前完整代码的输出与 README 一致。

## 与正式第一阶段的关系

第 00～06 部分帮助理解基本动作；第 07～12 部分示意部分系统的职责，并非完整语言规则清单。它们与正式模块的概念对应关系是：

```text
03、06              → compiler/ast + AstBuilder
04、05、07～11      → DeclarationCollector + SemanticChecker
12                  → rxc 入口、CMake、官方 semantic 回归
```

后半部分仍然使用小型 AST 隔离规则，目的是让每种算法能单独看懂。正式实现不是复制这些玩具类型，而是建立统一的 Program AST，接入真实 ANTLR Context，并按 [Rx 分类表](../compiler/RX_LANGUAGE_MAP.md)补齐、修正各类语义规则。
