#pragma once

// Responsibility:
// - Declare the second semantic pass that checks every function/method body.
// - Own current function context, local ScopeStack, and loop context stack.
// - checkExpr returns ExprInfo and accepts an optional expected Type.
// - Centralize coercion, common-type/LUB, borrow/deref, and receiver adjustment.
// - Check all AST nodes even after Never for static errors in unreachable code.
// Keep built-in tables, constant DFS, and capability algorithms in their own
// modules and call them from here.

namespace rx::semantic {}
