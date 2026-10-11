#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "rx/ast/node.hpp"
#include "rx/ast/statement.hpp"
#include "rx/ast/type_syntax.hpp"

namespace rx::ast {

// 生命周期参数节点，直接继承 AstNode；对应 lifetimeParam 规则，例如 'a: 'b + 'c。
// name 保存参数名，bounds 保存冒号后受约束的生命周期名。
struct LifetimeParameter final : AstNode {
    LifetimeParameter();
    [[nodiscard]] AstChildren children() const override;
    std::string name;                    // 生命周期参数名，例如 "'a"。
    std::vector<std::string> bounds;     // 该生命周期必须长于的生命周期列表。
};

// 泛型参数列表节点，直接继承 AstNode；对应 genericParams 规则。
// 当前 Rx 文法的泛型参数是生命周期参数，因此 lifetimes 保存其顺序列表。
struct GenericParameters final : AstNode {
    GenericParameters();
    [[nodiscard]] AstChildren children() const override;
    bool written = false;                           // 源码是否显式写了尖括号泛型列表。
    std::vector<LifetimeParameter> lifetimes;       // 尖括号内的生命周期参数。
};

// where 子句中的单条约束，直接继承 AstNode；对应 whereClauseItem 规则的两种形式。
// 保存约束主体（生命周期或类型）及其生命周期边界。
struct WherePredicate final : AstNode {
    WherePredicate();
    [[nodiscard]] AstChildren children() const override;
    bool subject_is_lifetime = false;               // true 表示主体是生命周期，否则是类型。
    std::string lifetime_subject;                   // 生命周期形式的主体，例如 'a。
    std::optional<TypeSyntax> type_subject;         // 类型形式的主体，例如 T。
    std::vector<std::string> lifetime_bounds;       // 冒号右侧的生命周期边界。
};

// where 子句节点，直接继承 AstNode；对应 whereClause 规则。
// written 区分没有写 where 与显式写了空 where 列表的语法状态。
struct WhereClause final : AstNode {
    WhereClause();
    [[nodiscard]] AstChildren children() const override;
    bool written = false;                           // 源码中是否出现 where。
    std::vector<WherePredicate> predicates;         // where 后面的约束，保持源码顺序。
};

// 普通函数参数节点，直接继承 AstNode；对应 functionParam 规则。
// 保存绑定名、mut 修饰和参数类型；self 接收者单独由 Receiver 表示。
struct FunctionParameter final : AstNode {
    FunctionParameter();
    [[nodiscard]] AstChildren children() const override;
    std::string name;                               // 参数绑定名。
    bool mutable_binding = false;                  // 参数绑定是否带 mut。
    TypeSyntax type;                               // 冒号后的参数类型语法。
};

// self 接收者的具体形式；这是取值标签，不是 AST 节点基类。
enum class ReceiverKind { Value, MutableValue, SharedReference, MutableReference };

// impl 方法中的 self 参数节点，直接继承 AstNode；对应 selfParam 规则。
// kind 记录 self/&self/&mut self 等形式，lifetime 保存可选生命周期。
struct Receiver final : AstNode {
    Receiver();
    [[nodiscard]] AstChildren children() const override;
    ReceiverKind kind = ReceiverKind::Value;       // self 接收者的值/引用及 mut 形式。
    std::string lifetime;                          // 引用接收者显式写出的生命周期；未写为空。
};
using ReceiverSyntax = Receiver;  // 旧名称别名；不是另一个不同的节点类。

// 顶层条目的抽象多态基类，继承 AstNode；对应 item 规则。
// UseItem、FunctionItem、StructItem、ConstantItem、ImplItem 是它的具体子类。
struct Item : AstNode {
    virtual ~Item() = default;
    [[nodiscard]] AstChildren children() const override = 0;
protected:
    explicit Item(NodeKind kind) : AstNode(kind) {}
};

enum class UseTreeKind { Path, Glob, Group };

// useTree 的递归节点，直接继承 AstNode；对应 useTree 规则中的路径、通配符和分组形式。
// prefix/group_items 保存路径前缀与花括号子项，alias 字段保存 as 重命名信息。
struct UseTree final : AstNode {
    UseTree();
    [[nodiscard]] AstChildren children() const override;
    UseTreeKind form = UseTreeKind::Path;          // 当前是普通路径、* 通配符还是 {...} 分组。
    bool leading_separator = false;                // 路径前是否有开头的 ::。
    std::vector<std::string> prefix;                // ::、* 或分组之前的路径段。
    std::optional<std::string> alias;               // as 后的别名；没有 as 时为空。
    bool alias_is_underscore = false;               // 别名是否显式写成 _。
    std::vector<UseTree> group_items;                // 花括号内递归嵌套的 useTree 子项。
};

// use 声明节点，继承抽象 Item；对应 useDeclaration 规则。
// tree 保存完整的递归导入树，而不是只保存整条声明的文本。
struct UseItem final : Item {
    UseItem();
    [[nodiscard]] AstChildren children() const override;
    UseTree tree;                                   // use 后的导入路径/分组结构。
};

// 函数定义节点，继承抽象 Item；对应 functionDefinition 规则。
// 字段分别保存签名信息、可选约束及函数体，参数和子节点保持源码顺序。
struct FunctionItem final : Item {
    FunctionItem();
    [[nodiscard]] AstChildren children() const override;
    std::string name;                               // 函数名。
    GenericParameters generic_parameters;           // 可选的泛型生命周期参数列表。
    std::optional<Receiver> receiver;                // impl 方法可选的 self 接收者。
    std::vector<FunctionParameter> parameters;      // 普通参数列表，按源码顺序。
    std::optional<TypeSyntax> return_type;           // 显式 -> 后的返回类型；未写箭头时为空。
    WhereClause where_clause;                       // 可选 where 子句。
    Block body;                                     // 函数体 blockExpression。
};

// 结构体字段节点，直接继承 AstNode；对应 structField 规则。
// name 是字段名，type 是冒号后的类型语法。
struct StructField final : AstNode {
    StructField();
    [[nodiscard]] AstChildren children() const override;
    std::string name;                               // 字段名。
    TypeSyntax type;                               // 字段类型。
};

// derive 属性可出现的 trait 名称；作为 DeriveAttribute 内的枚举值保存。
enum class DeriveTrait { Copy, Clone, PartialEq, Eq };

// 一条 #[derive(...)] 属性节点，直接继承 AstNode；对应 outerAttribute 规则。
// traits 保存括号内的 trait 名称，保留顺序和重复项以便语义检查。
struct DeriveAttribute final : AstNode {
    DeriveAttribute();
    [[nodiscard]] AstChildren children() const override;
    std::vector<DeriveTrait> traits;                // derive(...) 中的 trait 列表。
};

// 结构体定义节点，继承抽象 Item；对应 structDefinition 规则。
// 保存名称、泛型与 where 信息、derive 属性以及按源码顺序排列的字段。
struct StructItem final : Item {
    StructItem();
    [[nodiscard]] AstChildren children() const override;
    std::string name;                               // 结构体名称。
    GenericParameters generic_parameters;           // 泛型生命周期参数。
    WhereClause where_clause;                       // 可选 where 子句。
    std::vector<DeriveAttribute> attributes;        // struct 前面的外部属性，目前是 derive。
    std::vector<StructField> fields;                // 花括号内的字段列表。
};

// 常量定义节点，继承抽象 Item；对应 constantItem 规则。
// type 是声明类型，initializer 是受限 constValue 语法，不是普通表达式。
struct ConstantItem final : Item {
    ConstantItem();
    [[nodiscard]] AstChildren children() const override;
    std::string name;                               // const 后的常量名。
    TypeSyntax type;                               // 冒号后的显式类型。
    ConstValueSyntax initializer;                  // 等号后的常量值语法。
};

// impl 内的一项成员节点，直接继承 AstNode；对应 associatedItem 规则。
// value 是函数定义或常量定义二选一的多态持有方式。
struct AssociatedItem final : AstNode {
    AssociatedItem();
    [[nodiscard]] AstChildren children() const override;
    std::variant<std::unique_ptr<FunctionItem>, std::unique_ptr<ConstantItem>> value; // 被拥有的函数或常量。
};

// inherent impl 块节点，继承抽象 Item；对应 inherentImpl 规则。
// target_type 是 impl 的目标类型，items 是块内关联函数/常量。
struct ImplItem final : Item {
    ImplItem();
    [[nodiscard]] AstChildren children() const override;
    GenericParameters generic_parameters;            // impl 前的泛型生命周期参数。
    TypeSyntax target_type;                          // impl 所针对的类型。
    WhereClause where_clause;                       // 可选 where 子句。
    std::vector<AssociatedItem> items;              // impl 花括号中的成员，按源码顺序。
};

}  // namespace rx::ast
