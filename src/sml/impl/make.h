#pragma once

#include "sml/ids.h"
#include "sml/model.h"

namespace sml::impl {

namespace transition {

template <typename T, Action A>
struct Run;

template <typename T, State D>
struct To;

template <typename Self>
struct Mixin {
    template <State S>
    auto operator=(S dst) && {  // NOLINT
        return stdlike::move(*static_cast<Self*>(this)).to(stdlike::move(dst));
    }

    template <Action A>
    auto operator|(A action) && {
        return stdlike::move(*static_cast<Self*>(this)).run(stdlike::move(action));
    }

    template <Action Action>
    auto run(Action a) && {
        return Run<Self, Action>{
            stdlike::move(*static_cast<Self*>(this)),
            stdlike::move(a),
        };
    }

    template <State Dst>
    auto to(Dst) && {
        return To<Self, Dst>{stdlike::move(*static_cast<Self*>(this))};
    }
};

template <typename T, State D>
struct To : Mixin<To<T, D>> {
    using Src = typename T::Src;
    using Dst = D;
    using Event = typename T::Event;
    using Mixin<To<T, D>>::operator=;

    explicit To(T parent) : parent_{stdlike::move(parent)} {}

    template <State Dest>
    auto to(Dest) && {
        return To<T, Dest>{stdlike::move(parent_)};
    }

    template <typename SId, typename EId>
    bool tryExecute(SId sid, const EId& eid) {
        if constexpr (stdlike::same_as<typename Dst::Id, BypassStateId>) {
            parent_.tryExecute(sid, eid);
            return false;
        } else {
            return parent_.tryExecute(sid, eid);
        }
    }

 private:
    T parent_;
};

template <typename T, Action A>
struct Run : Mixin<Run<T, A>> {
    using Src = typename T::Src;
    using Dst = typename T::Dst;
    using Event = typename T::Event;
    using Mixin<Run<T, A>>::operator=;

    Run(T parent, A action) : parent_{stdlike::move(parent)}, action_{stdlike::move(action)} {}

    template <typename SId, typename EId>
    bool tryExecute(SId sid, const EId& eid) {
        if (!parent_.tryExecute(sid, eid)) {
            return false;
        }

        action_(sid, eid);
        return true;
    }

 private:
    T parent_;
    A action_;
};

template <typename S, Event E>
struct Make : Mixin<Make<S, E>> {
    using Src = S;
    using Dst = void;
    using Event = E;
    using Mixin<Make<S, E>>::operator=;

    explicit Make(E event) : event_{stdlike::move(event)} {}

    template <typename SId, typename EId>
    bool tryExecute(SId sid, const EId& eid) {
        return event_.match(sid, eid);
    }

 private:
    E event_;
};

template <typename T, State NewSrc, typename NewDst>
struct Replace : Mixin<Replace<T, NewSrc, NewDst>> {
    using Src = NewSrc;
    using Dst = NewDst;
    using Event = typename T::Event;
    using Mixin<Replace<T, Src, Dst>>::operator=;

    explicit Replace(T parent) : parent_{stdlike::move(parent)} {}

    template <typename SId, typename EId>
    bool tryExecute(SId sid, const EId& eid) {
        return parent_.tryExecute(sid, eid);
    }

 private:
    T parent_;
};

}  // namespace transition

namespace state {

template <typename Self>
struct Mixin {
    template <Event E>
    auto operator+(E event) const {
        return static_cast<const Self*>(this)->on(stdlike::move(event));
    }
};

template <typename TId, typename TUId = TId>
struct Make : Mixin<Make<TId, TUId>> {
    using Id = TId;
    using UId = TUId;

    template <Event E>
    auto on(E event) const {
        return transition::Make<Make, E>{stdlike::move(event)};
    }
};

}  // namespace state

namespace event {

template <typename E, typename C, typename... TIds>
struct When {
    using Ids = typename E::Ids;

    When(E parent, C condition)
        : parent_{stdlike::move(parent)}
        , condition_{stdlike::move(condition)} {}

    template <typename SId, IdMatches<Ids> EId>
    bool match(SId sid, const EId& eid) {
        return parent_.match(sid, eid) && condition_(sid, eid);
    }

    template <Condition<TIds...> Cond>
    auto when(Cond condition) {
        return When<When, Cond, TIds...>{
            stdlike::move(*this),
            stdlike::move(condition),
        };
    }

    template <Condition<TIds...> Cond>
    auto operator[](Cond cond) && {
        return stdlike::move(*this).when(stdlike::move(cond));
    }

 private:
    E parent_;
    C condition_;
};

template <typename... TIds>
struct Make {
    using Ids = AnyId<TIds...>;

    template <typename SId, IdMatches<Ids> EId>
    bool match(SId, const EId&) const {
        return true;
    }

    template <Condition<TIds...> C>
    auto when(C condition) const {
        return When<Make, C, TIds...>{
            stdlike::move(*this),
            stdlike::move(condition),
        };
    }

    template <Condition<TIds...> C>
    auto operator[](C cond) const {
        return this->when(stdlike::move(cond));
    }
};

}  // namespace event

}  // namespace sml::impl
