#pragma once

// Responsibility:
// - Define the result returned by expression checking.
// - Store resolved Type, Value/Place category, place writability, and access
//   information such as whether a shared reference was crossed.
// - This is where assignment, borrow, field, and index share one vocabulary.

namespace rx::semantic {}
