# 实现文件放置

头文件接口确定后，再在这里逐步创建实现：

```text
frontend/diagnostic.cpp
frontend/ast_builder.cpp
semantic/type.cpp
semantic/scope.cpp
semantic/builtin_environment.cpp
semantic/declaration_collector.cpp
semantic/constant_evaluator.cpp
semantic/type_capabilities.cpp
semantic/semantic_checker.cpp
main.cpp
```

第一批必须同时有正式 `main.cpp`、构建目标、诊断、最小 AST Builder 和最小语义链；AST 打印工具用于观察真实输入。不要推迟到最后才接入口，也不要一次堆出一批没有功能的空 `.cpp`。

具体分工和每个函数的输入输出见 [三天实施计划](../THREE_DAY_PLAN.md)。按功能增加实现文件，但共用同一套 AST、Type 和环境。
