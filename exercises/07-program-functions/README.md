# 示例 07：Program AST、函数和两遍扫描

## 它与正式任务的关系

此前只处理一棵表达式 AST；正式编译器的根必须是 `Program`，其下保存函数、struct、const、impl 等 Item。本例先完整演示函数这一类 Item：

```text
Program
└── Function
    ├── name
    ├── parameters
    ├── return type
    └── Body(statements + optional tail)
```

正式工程中，这对应 `crate → Program`、`functionDefinition → FunctionItem`，以及 `DeclarationCollector` 和函数体 `SemanticChecker`。

## 为什么必须两遍扫描

合法示例首先定义 `twice`，但它调用后面才出现的 `add`：

```rust
fn twice(x: i32) -> i32 { add(x, x) }
fn add(a: i32, b: i32) -> i32 { a }
fn main() { twice(21); }
```

`Analyzer::analyze()` 的第一遍只把全部函数签名放入 `functions_`；第二遍才检查函数体。因此检查 `twice` 时已经能找到 `add`。递归和相互递归也是同一个道理。

## 完整代码的检查内容

- 收集阶段拒绝重复函数名。
- 参数加入各自函数的局部名字表。
- 调用检查函数是否存在、参数数量和参数类型。
- 函数 tail 类型必须匹配声明的返回类型；没有 tail 就是 `unit`。
- 顶层 `main` 必须存在、没有参数、返回 `unit`。
- 局部非函数名字遮蔽同名全局函数时，不能继续把它当函数调用。

程序还运行一个参数数量错误的负例，展示错误出现在第二遍而不是 Parser。

## 阅读顺序

1. `Program`、`Function`、`Body` 和 `Expr` 数据结构。
2. `collectFunctions()`：第一遍。
3. `checkFunction()`：创建参数局部表并检查 body。
4. `checkExpr()` 中的 `Call` 分支。
5. `checkEntryPoint()`：独立的 main 规则。

## 运行

```bash
cd "/home/ark_en/acm/rust compiler/rx-compiler/exercises/07-program-functions"
./check.sh
```

成功时显示 `PASS: program/function output matches expected.txt`。
