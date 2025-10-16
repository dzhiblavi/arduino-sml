#pragma once

#include "sml/impl/ids.h"
#include "sml/impl/make.h"
#include "sml/impl/ref.h"

namespace sml {

template <typename Id>
constexpr State auto state = impl::state::Make<Id, Id>{};

template <typename... Ids>
inline constexpr auto any = state<impl::AnyId<Ids...>>;

template <StateMachine M>
constexpr State auto enter =
    impl::state::Make<typename M::InitialId, impl::RefUId<M, typename M::InitialId>>{};

template <StateMachine M>
constexpr State auto exit =
    impl::state::Make<impl::TerminalStateId, impl::RefUId<M, impl::TerminalStateId>>{};

template <typename Id>
constexpr Event auto event = impl::event::Make<Id>{};

[[maybe_unused]] static constexpr State auto x = state<impl::TerminalStateId>;
[[maybe_unused]] static constexpr Event auto onEnter = event<impl::OnEnterEventId>;
[[maybe_unused]] static constexpr Event auto onExit = event<impl::OnExitEventId>;

template <Transition... Ts>
auto make(Ts... ts) {
    return stdlike::tuple<Ts...>(stdlike::move(ts)...);
}

}  // namespace sml
