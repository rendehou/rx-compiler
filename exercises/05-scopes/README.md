# 示例 05：变量、名字查找和嵌套作用域

> 正式 Rx 规则更正：本例代码采用了“同层重名拒绝”的教学简化，它不符合 Rx 的 let 遮蔽规则。正式实现应先检查 initializer，再新建变量记录并更新当前层映射；同层 let 可以遮蔽旧变量或参数。参数之间、同命名空间的全局声明之间仍需检查重复。下面只描述这个旧示例实际做了什么，不作为正式语义依据；见 [名字和声明](../../compiler/RX_LANGUAGE_MAP.md#3-名字和声明不是一张-map-包办所有东西)。

## 输入和输出

`main.cpp` 构造三个 Block AST。合法程序相当于：

```rust
{
    let x: i32 = 10;
    {
        let x: bool = true;
        x; // 内层 bool
    }
    x;     // 外层 i32
}
```

另外两个程序分别在同一层重复声明 `x`、使用未声明的 `missing`。程序会输出每次进入作用域、声明、查找和离开的过程。

## 符号表的具体形式

`scope.cpp` 使用：

```cpp
std::vector<std::unordered_map<std::string, Type>> scopes_;
```

每张 map 是一层 `{ ... }`。进入 Block 时 `push_back`，离开时 `pop_back`。名字查找从 vector 末尾向前，因此内层变量遮蔽外层变量；重复声明只检查最后一张 map，因此同层重名拒绝、不同层重名允许。

## 阅读顺序

1. `visitBlock()`：作用域何时增加和删除。
2. `visitLet()`：先检查初始化表达式，再检查同层重名，最后插入变量。
3. `visitName()`：从内向外倒序查找。
4. `visit()`：根据 AST 节点类型分派。

先检查初始化表达式很重要：`let x: i32 = x;` 的右侧不能找到尚未完成声明的新 `x`。

## 运行

```bash
cd "/home/ark_en/acm/rust compiler/rx-compiler/exercises/05-scopes"
./check.sh
```

成功时显示：

```text
PASS: scope checker output matches expected.txt
```
