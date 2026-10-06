# 示例 12：正式流水线、错误短路和覆盖审计

## 它不是新的语义规则

第 12 部分只模拟真正编译器入口应当具有的控制关系。代码用 `syntax_valid`、`semantics_valid` 等预设真假值代表结果，没有实际读取 Rx 文件、调用 ANTLR 并完成语义检查。它不是可直接用于评测的编译器：

```text
读取源文件
→ Lexer/Parser
→ AstBuilder
→ DeclarationCollector
→ SemanticChecker
→ 成功退出 0，任一阶段失败退出 1
```

`main.cpp` 用缩小的阶段对象演示控制关系：语法错误后绝不能继续构建恢复树；声明错误后不能进入 body 检查；语义错误必须稳定返回 1，不能崩溃或返回随机状态。

## 正式目录建议

```text
compiler/include/ast/          AST 数据结构
compiler/include/frontend/     AstBuilder、diagnostics
compiler/include/semantic/     Type、Symbol、Checker
compiler/src/ast/
compiler/src/frontend/
compiler/src/semantic/
compiler/src/main.cpp          只负责组装流水线和退出码
```

依赖方向应该是：

```text
rxc → rx_frontend → rx_parser
```

实验打印工具可以链接 `rx_frontend`，但正式 library 不应反向依赖实验工具。

## AST 覆盖审计

接入后不要只按失败测试打补丁，应逐条审计 `Parser.g4`：

```text
Item       use / function / struct / const / impl
Type       unit / path / reference / array
Statement  empty / let / expression statement / tail
Primary    literal / path / grouping / array / jump / block / control
Postfix    call / index / method / field
Expression unary / cast / binary / assignment
```

每一行必须能回答：AST 节点是什么、Builder 入口在哪里、Checker 入口在哪里、由哪组测试验证。只有 `use` 与不参与语义的 lifetime 信息可以按规范有意丢弃。

## 回归顺序

先运行小而基础的 semantic 组，再运行组合用例：

```text
entry
→ namespace-errors / names-and-shadowing
→ arithmetic / boolean / casts
→ blocks / calls / loops
→ structs / arrays / assignment
→ references / coercions / methods
→ Box / Vec / constants / derive / recursive-layout
→ comprehensive
```

对失败按共同根因聚类，不要为每个文件添加名字特判。

## 运行

```bash
cd "/home/ark_en/acm/rust compiler/rx-compiler/exercises/12-integration"
./check.sh
```

整个讲解系列可以运行：

```bash
cd "/home/ark_en/acm/rust compiler/rx-compiler/exercises"
./check-all.sh
```

## 读完后的真实工作

这些示例只提供部分算法思路，正式第一阶段仍需要统一的 `Program` AST 与 `SemanticChecker`，并补齐完整 Rx 语义。当前建议以 [三天实施方案](../../compiler/THREE_DAY_PLAN.md)为准：第一步就创建正式 main 和构建目标，随后逐个功能扩展并跑真实文件，不等到最后才集成。
