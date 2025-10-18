#pragma once

#include "sml/ids.h"
#include "sml/impl/make.h"

namespace sml {

template <typename... Ids>
constexpr SrcState auto src = impl::state::Src<void, Ids...>{};

template <typename Id>
constexpr DstState auto dst = impl::state::Dst<void, Id>{};

inline constexpr DstState auto bypass = dst<BypassStateId>;
inline constexpr DstState auto x = dst<TerminalStateId>;

template <StateMachine M>
constexpr DstState auto enter = impl::state::Dst<M, typename M::InitialId>{};

template <StateMachine M>
constexpr SrcState auto exit = impl::state::Src<M, TerminalStateId>{};

template <typename... Id>
constexpr Event auto ev = impl::event::Make<Id...>{};

inline constexpr Event auto onEnter = ev<OnEnterEventId>;
inline constexpr Event auto onExit = ev<OnExitEventId>;

template <Transition... Ts>
TransitionsTuple auto table(Ts... ts) {
    return std::tuple<Ts...>(std::move(ts)...);
}

}  // namespace sml
