#pragma once

#include "sml/ids.h"
#include "sml/model.h"

#include <supp/type_list.h>

namespace sml::impl {

namespace transition {

template <typename T, typename A>
struct Run;

template <typename T, typename C>
struct When;

template <typename T, typename D>
struct To;

template <typename Self>
struct Mixin {
    template <DstState S>
    auto operator=(S dst) && {  // NOLINT
        return std::move(*static_cast<Self*>(this)).to(std::move(dst));
    }

    template <typename A>
    auto operator!=(A action) && {
        return std::move(*static_cast<Self*>(this)).run(std::move(action));
    }

    template <typename A>
    auto operator==(A action) && {
        return std::move(*static_cast<Self*>(this)).when(std::move(action));
    }

    template <DstState Dst>
    auto to(Dst) && {
        return To<Self, Dst>{std::move(*static_cast<Self*>(this))};
    }

    template <typename Action>
    auto run(Action a) && {
        return Run<Self, Action>{
            std::move(*static_cast<Self*>(this)),
            std::move(a),
        };
    }

    template <typename Condition>
    auto when(Condition c) && {
        return When<Self, Condition>{
            std::move(*static_cast<Self*>(this)),
            std::move(c),
        };
    }
};

template <typename T, typename Tag>
struct Tagged : T {
    using T::operator=;
    using Src = typename T::Src::template Tagged<Tag>;
    using Dst = typename T::Dst::template Tagged<Tag>;
};

template <typename T, typename D>
struct To : Mixin<To<T, D>> {
    using Src = typename T::Src;
    using Dst = D;
    using Event = typename T::Event;
    using Mixin<To<T, D>>::operator=;

    explicit To(T parent) : parent_{std::move(parent)} {}

    template <DstState Dest>
    auto to(Dest) && {
        return To<T, Dest>{std::move(parent_)};
    }

    template <typename SId, typename EId>
    bool operator()(SId sid, const EId& eid) {
        return parent_(sid, eid);
    }

 private:
    T parent_;
};

template <typename T, typename A>
struct Run : Mixin<Run<T, A>> {
    using Src = typename T::Src;
    using Dst = typename T::Dst;
    using Event = typename T::Event;
    using Mixin<Run<T, A>>::operator=;

    Run(T parent, A action) : parent_{std::move(parent)}, action_{std::move(action)} {}

    template <typename SId, typename EId>
    bool operator()(SId sid, const EId& eid) {
        if (!parent_(sid, eid)) {
            return false;
        }

        action_(sid, eid);
        return true;
    }

 private:
    T parent_;
    A action_;
};

template <typename T, typename C>
struct When : Mixin<When<T, C>> {
    using Src = typename T::Src;
    using Dst = typename T::Dst;
    using Event = typename T::Event;
    using Mixin<When<T, C>>::operator=;

    When(T parent, C condition)
        : parent_{std::move(parent)}
        , condition_{std::move(condition)} {}

    template <typename SId, typename EId>
    bool operator()(SId sid, const EId& eid) {
        return parent_(sid, eid) && condition_(sid, eid);
    }

 private:
    T parent_;
    C condition_;
};

template <typename S, typename D, Event E>
struct Make : Mixin<Make<S, D, E>> {
    using Src = S;
    using Dst = D;
    using Event = E;
    using Mixin<Make<S, D, E>>::operator=;

    template <typename SId, typename EId>
    bool operator()(SId, const EId&) {
        return true;
    }
};

}  // namespace transition

namespace state {

template <typename TTag, typename TId>
struct Dst {
    using Id = TId;
    using Tag = TTag;

    template <typename NewTag>
    using Tagged = std::conditional_t<std::same_as<void, Tag>, Dst<NewTag, Id>, Dst>;
};

template <typename TTag, typename... TIds>
struct Src {
    using Ids = tl::List<TIds...>;
    using Tag = TTag;

    template <Event E>
    auto on(E) const {
        return transition::Make<Src, Dst<Tag, KeepStateId>, E>{};
    }

    template <Event E>
    auto operator+(E event) const {
        return on(std::move(event));
    }

    template <typename NewTag>
    using Tagged = std::conditional_t<std::same_as<void, Tag>, Src<NewTag, TIds...>, Src>;
};

}  // namespace state

namespace event {

template <typename... TIds>
struct Make {
    using Ids = tl::List<TIds...>;
};

}  // namespace event

}  // namespace sml::impl
