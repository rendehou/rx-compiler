# 示例 08：Block、tail、Never 和控制流上下文

> 本例是简化模型，不是完整 Rx 控制流实现。正式版本还必须处理有／无 tail 的发散块、向分支和 break 传递 expected type、while 条件里的跳转边界、不可达源码的检查。尤其不能用“无 tail 一律 Unit”判定 `fn f()->i32{return 1;}`。正式规则见 [控制流分类](../../compiler/RX_LANGUAGE_MAP.md#9-控制流带着上下文递归而不是执行程序)。

## 它与正式任务的关系

正式 AST 必须区分：

```rust
{ 1; } // expression statement 被丢弃，Block 类型是 unit
{ 1 }  // tail expression，Block 类型是 i32
```

本例的 `Expr::has_tail` 明确保存这个区别，并进一步实现 `if`、`return`、`while`、`loop`、`break`、`continue`。正式工程中应使用清晰的 `BlockExpr`、`ReturnExpr` 等节点，而不是依赖源码文本或最后一个分号反推。

## 三种语义上下文

`Checker` 除作用域外还保存：

- `function_result_`：检查 `return` 操作数。
- `loops_`：检查 break/continue 是否有目标，以及最近循环是否允许 `break value`。
- `expected`：函数 tail、return、if 分支和 break value 希望得到的类型。

`Never` 表示表达式不会正常产生值。`return 1` 的类型是 Never，因此它可以和另一个 `i32` 分支合并成 `i32`；Never 不是 Unit。

## 完整代码演示

- `if true { return 1 } else { 2 }` 得到 `i32`。
- `loop { break 1; }` 的 break 值决定 loop 为 `i32`。
- 循环外 break 被拒绝。
- while 中的 `break 1` 被拒绝。
- 没有 else 的 `if true { 1 }` 被拒绝，因为它必须产生 unit。

阅读 `checkCore()` 时重点看 Block、If、Return、While、Loop、Break 六个分支，以及 `commonType()` 如何处理 Never。

## 运行

```bash
cd "/home/ark_en/acm/rust compiler/rx-compiler/exercises/08-control-flow"
./check.sh
```

成功时显示 `PASS: control-flow output matches expected.txt`。
