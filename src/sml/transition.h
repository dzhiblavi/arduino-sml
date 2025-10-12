#pragma once

#include <stdlike/utility.h>

namespace sm {

template <bool Init, typename S, typename E, typename D>
struct Transition {
    using SrcState = S;
    using DstState = D;
    using Event = E;
    static constexpr bool IsInitial = Init;

    explicit Transition(E event) : event_{stdlike::move(event)} {}

    template <bool RInit, typename RS, typename RD>
    Transition(Transition<RInit, RS, E, RD> rhs) : event_{stdlike::move(rhs.event_)} {}

    template <typename NewDst>
    auto operator=(NewDst) && {  // NOLINT
        return Transition<Init, SrcState, Event, NewDst>{stdlike::move(event_)};
    }

    template <typename NewDst>
    using ReplaceDst = Transition<Init, SrcState, Event, NewDst>;

    using NonInitial = Transition<false, SrcState, Event, DstState>;

    template <typename T>
    using TagIfUntagged = Transition<
        Init,
        typename S::template TagIfUntagged<T>,
        E,
        typename D::template TagIfUntagged<T>>;

    bool execute(const typename Event::Raw& raw) {
        if (!event_.willAccept(raw)) {
            return false;
        }

        event_.accept(raw);
        return true;
    }

 private:
    Event event_;

    template <bool, typename, typename, typename>
    friend struct Transition;
};

}  // namespace sm
