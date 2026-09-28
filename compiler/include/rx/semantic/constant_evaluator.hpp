#pragma once

// Responsibility:
// - Evaluate restricted constValue AST into a typed 32-bit value.
// - Use Unvisited/Visiting/Done states for forward references and cycle errors.
// - Serve top-level/associated constants, array lengths, and repeat counts.
// Runtime expression evaluation does not belong here.

namespace rx::semantic {}
