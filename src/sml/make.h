#pragma once

#include "sml/ids.h"
#include "sml/impl/make.h"
#include "sml/impl/ref.h"

namespace sml {

template <typename Id>
constexpr State auto state = impl::state::Make<Id, Id>{};

inline constexpr State auto bypass = state<BypassStateId>;

template <typename... Ids>
constexpr auto any = state<AnyId<Ids...>>;

template <StateMachine M>
constexpr State auto enter =
    impl::state::Make<typename M::InitialId, impl::RefUId<M, typename M::InitialId>>{};

template <StateMachine M>
constexpr State auto exit = impl::state::Make<TerminalStateId, impl::RefUId<M, TerminalStateId>>{};

template <typename Id>
constexpr Event auto event = impl::event::Make<Id>{};

[[maybe_unused]] static constexpr State auto x = state<TerminalStateId>;
[[maybe_unused]] static constexpr Event auto onEnter = event<OnEnterEventId>;
[[maybe_unused]] static constexpr Event auto onExit = event<OnExitEventId>;

template <Transition... Ts>
auto make(Ts... ts) {
    return stdlike::tuple<Ts...>(stdlike::move(ts)...);
}

}  // namespace sml
