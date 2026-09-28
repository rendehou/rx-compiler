#pragma once

// Responsibility:
// - Query supportsCopy/Clone/PartialEq/Eq for any resolved Type.
// - Validate derive duplication, dependencies, and field capabilities.
// - Independently validate finite inline layout; references/Box/Vec break layout
//   recursion even when trait capability traversal may still inspect elements.

namespace rx::semantic {}
