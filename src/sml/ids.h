#pragma once

#include <supp/type_list.h>

namespace sml {

struct OnEnterEventId {};
struct OnExitEventId {};
struct TerminalStateId {};
struct BypassStateId {};
struct KeepStateId {};

using EphemeralStateIds = tl::List<BypassStateId, KeepStateId>;

}  // namespace sml
