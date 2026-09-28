#pragma once

// Responsibility:
// - Declare installation of protected primitive/container type names.
// - Install get_i32/print_i32/println_i32 as ordinary FunctionSymbols.
// - Install Box/Vec constructors and methods as table-driven associated items.
// Call checking should query symbols, not special-case strings everywhere.

namespace rx::semantic {}
