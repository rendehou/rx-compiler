#pragma once

// Responsibility:
// - Define resolved Type: primitives, Unit, Never, StructId, Reference,
//   Array(element + evaluated length), Box, and Vec.
// - Define structural equality/hash and human-readable formatting.
// - Keep TypeSyntax-to-Type resolution out of AST and centralize it here or
//   in a closely related resolver implementation.

namespace rx::semantic {}
