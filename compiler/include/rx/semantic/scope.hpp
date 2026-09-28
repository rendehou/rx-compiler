#pragma once

// Responsibility:
// - Define separate global type and value namespaces.
// - Define the stack of local value scopes used while checking a function.
// - Provide declare-current-scope and lookup-inner-to-outer operations.
// - Local bindings record Type and binding mutability.
// Do not put expression traversal in this file.

namespace rx::semantic {}
