#pragma once

// Responsibility:
// - Define stable IDs and symbols for functions, structs, constants, methods,
//   associated functions, and associated constants.
// - FunctionSymbol contains resolved parameter/result types, not function body.
// - StructSymbol contains field table, derives, and associated value namespace.
// AST ownership remains in ast::Program; symbols may refer to AST by stable ID.

namespace rx::semantic {}
