#pragma once

// Responsibility:
// - Declare the first semantic pass over ast::Program.
// - First reserve all struct names, then resolve fields/signatures/constants,
//   then collect impl associated items.
// - Reject duplicate/protected names and invalid impl targets.
// - Never enter function bodies in this pass.

namespace rx::semantic {}
