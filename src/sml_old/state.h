#pragma once

#include "sml/transition.h"

namespace sm {

template <typename S, typename Tag = void, bool Init = false>
struct State {
    using type = S;
    static constexpr bool IsInitial = Init;

    template <typename NewTag>
    using TagIfUntagged =
        stdlike::conditional_t<stdlike::is_same_v<Tag, void>, State<S, NewTag, Init>, State>;

    template <typename E>
    auto operator+(E event) const {
        using EffectiveState = State<S, Tag, false>;
        return Transition<Init, EffectiveState, E, EffectiveState>{stdlike::move(event)};
    }

    auto operator*() const {
        return State<S, Tag, true>{};
    }
};

template <typename S>
[[maybe_unused]] static constexpr State<S> state;

class TerminalState {};
[[maybe_unused]] static constexpr State<TerminalState> X;

template <auto S, typename Tag>
using Ref = State<typename decltype(S)::type, Tag>;

// Reference into a state of another machine
template <auto S, typename Tag>
[[maybe_unused]] static constexpr Ref<S, Tag> ref;

template <char... Chars>
class NamedState {};

namespace literals {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wgnu-string-literal-operator-template"
#endif

template <class T, T... Chars>
constexpr State<NamedState<Chars...>> operator""_s() {
    return {};
}

#pragma GCC diagnostic pop

}  // namespace literals

}  // namespace sm
