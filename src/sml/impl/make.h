#pragma once

#include "sml/impl/ids.h"
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

    bool tryExecute(const typename Event::Id& id) {
        if constexpr (stdlike::same_as<typename Dst::Id, BypassStateId>) {
            parent_.tryExecute(id);
            return false;
        } else {
            return parent_.tryExecute(id);
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

    bool tryExecute(const typename Event::Id& id) {
        if (!parent_.tryExecute(id)) {
            return false;
        }

        action_(typename Src::Id{}, id);
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

    bool tryExecute(const typename E::Id& id) {
        return event_.match(typename Src::Id{}, id);
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

    bool tryExecute(const typename Event::Id& id) {
        return parent_.tryExecute(id);
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

template <typename Self>
struct Mixin {
    template <Condition<typename Self::Id> C>
    auto operator[](C cond) && {
        return stdlike::move(*static_cast<Self*>(this)).when(stdlike::move(cond));
    }

    template <Condition<typename Self::Id> C>
    auto operator[](C cond) const {
        return static_cast<const Self*>(this)->when(stdlike::move(cond));
    }
};

template <typename E, Condition<typename E::Id> C>
struct When : Mixin<When<E, C>> {
    using Id = typename E::Id;

    When(E parent, C condition)
        : parent_{stdlike::move(parent)}
        , condition_{stdlike::move(condition)} {}

    template <typename SId>
    bool match(SId sid, const Id& eid) {
        return parent_.match(sid, eid) && condition_(sid, eid);
    }

    template <Condition<Id> Cond>
    auto when(Cond condition) {
        return When<When, Cond>{
            stdlike::move(*this),
            stdlike::move(condition),
        };
    }

 private:
    E parent_;
    C condition_;
};

template <typename TId>
struct Make : Mixin<Make<TId>> {
    using Id = TId;

    template <typename SId>
    bool match(SId, const Id&) const {
        return true;
    }

    template <Condition<Id> C>
    auto when(C condition) const {
        return When<Make, C>{
            stdlike::move(*this),
            stdlike::move(condition),
        };
    }
};

}  // namespace event

}  // namespace sml::impl
