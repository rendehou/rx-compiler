# 示例 10：引用、访问能力、coercion 和方法

## 引用检查建立在 Place 之上

第 09 部分已经让表达式返回 `ExprInfo`。本例继续加入：

```text
Type          = i32 | unit | Struct(name) | &Type | &mut Type
ExprInfo      = type + 是否 Place + 是否可写 + 是否穿过 shared reference
```

`&mut place` 要求可写 Place；`&mut value` 可以物化一个可变临时对象。解引用 `&T` 得到不可写 Place，解引用 `&mut T` 通常得到可写 Place。

关键例子是 `&&mut i32`：第一次解引用穿过外层 shared reference，第二次即使看到内层 `&mut` 也不能恢复写权限。因此不能只看最终类型上的 `mut`，还要沿访问路径保存 `crossed_shared`。

## 集中的 coercion

`coerce()` 统一处理已知源类型如何适配 expected type。本例展示：

```text
T       → T
&mut T  → &T
```

正式实现还要在同一个入口增加 Never、重复引用层、Box 解引用后重新借用等规则；不要把 coercion 分散到 let、return、if 和调用参数的各自分支。

## 方法 receiver

`MethodTable` 把方法签名放入 struct 的关联命名空间。点调用先找到方法，再根据 receiver 要求检查：

- `&self` 可以读取 mutable 或 immutable receiver。
- `&mut self` 必须最终得到 mutable Place。
- 普通参数在 receiver 调整之后检查。
- 没有 self 的关联函数不能通过点调用。

本例接受 mutable `Counter.increase()`、接受 immutable `Counter.get()`，拒绝 immutable `Counter.increase()`。

## 正式工程映射

```text
borrow/dereference/coerce   → SemanticChecker 的统一辅助函数
crossed_shared             → ExprInfo/AccessInfo
MethodTable                → 每个 StructSymbol 的 associated scope
callMethod                 → MethodCallExpr 检查分支
```

## 运行

```bash
cd "/home/ark_en/acm/rust compiler/rx-compiler/exercises/10-references-methods"
./check.sh
```

成功时显示 `PASS: reference/method output matches expected.txt`。
