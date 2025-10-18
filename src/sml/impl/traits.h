#pragma once

#include "sml/impl/make.h"

#include <supp/tuple.h>
#include <supp/type_list.h>

namespace sml::impl::traits {

template <StateMachine M>
using TransitionsTuple = decltype(std::declval<M>().transitions());

template <StateMachine M>
using Transitions = tl::ToList<TransitionsTuple<M>>;

// Retrieves a list of state machine's submachines by destionation state Tags.
// The machine M itself is not included.
template <StateMachine M, tl::IsList Seen = tl::List<>>
struct SubmachinesI {
    template <Transition, bool>
    struct MapperI {
        using type = tl::List<>;
    };

    template <Transition T>
    struct MapperI<T, true> {
        using Submachine = typename T::Dst::Tag;
        using NewSeen = tl::PushFront<Seen, Submachine>;
        using type = tl::PushBack<typename SubmachinesI<Submachine, NewSeen>::type, Submachine>;
    };

    struct Mapper {
        template <Transition T>
        using Tag = typename T::Dst::Tag;

        template <Transition T>
        using Map = typename MapperI<T, StateMachine<Tag<T>> && !tl::Contains<Seen, Tag<T>>>::type;
    };

    using type = tl::Unique<tl::Flatten<tl::Map<Mapper, Transitions<M>>>>;
};
template <StateMachine M>
using Submachines = typename SubmachinesI<M>::type;

template <tl::IsList Transitions, typename Tag>
struct TagTransitionsI {
    struct Mapper {
        template <Transition T>
        using Map = transition::Tagged<T, Tag>;
    };

    using type = tl::Map<Mapper, Transitions>;
};

template <tl::IsList Transitions, typename Tag>
using TagTransitions = typename TagTransitionsI<Transitions, Tag>::type;

template <StateMachine M>
struct CombinedTransitionsI {
    using Machines = tl::PushFront<Submachines<M>, M>;

    struct Mapper {
        template <StateMachine SM>
        using Map = TagTransitions<Transitions<SM>, SM>;
    };

    using type = tl::Flatten<tl::Map<Mapper, Machines>>;
};

template <StateMachine M>
using CombinedTransitions = typename CombinedTransitionsI<M>::type;

template <StateMachine M>
class CombinedStateMachine {
 public:
    using InitialId = typename M::InitialId;

    template <StateMachine... Machines>
    explicit CombinedStateMachine(Machines... machines) : machines_{std::move(machines)...} {}

    sml::TransitionsTuple auto transitions() {
        return supp::tuple_convert<TransitionsTuple>(
            std::apply([](auto&... m) { return std::tuple_cat(m.transitions()...); }, machines_));
    }

 private:
    using Transitions = CombinedTransitions<M>;
    using Machines = tl::PushFront<Submachines<M>, M>;

    using MachinesTuple = tl::ApplyToTemplate<Machines, std::tuple>;
    using TransitionsTuple = tl::ApplyToTemplate<Transitions, std::tuple>;

    MachinesTuple machines_;
};

// Filters transitions by event Id
template <typename Id, tl::IsList Transitions>
struct FilterTransitionsByEventIdI {
    struct Pred {
        template <typename T>
        static constexpr bool test() {
            using Ids = typename T::Event::Ids;
            return tl::Empty<Ids> || tl::Contains<Ids, Id>;
        }
    };

    using type = tl::Filter<Pred, Transitions>;
};

template <typename EId, tl::IsList Transitions>
using FilterTransitionsByEventId = typename FilterTransitionsByEventIdI<EId, Transitions>::type;

template <typename TId, typename TTag>
struct StateSpec {
    using Id = TId;
    using Tag = TTag;
};

template <typename Tag, tl::IsList StateSpecs>
struct FilterStateSpecsByTagI {
    struct Pred {
        template <typename S>
        static constexpr bool test() {
            return std::same_as<Tag, typename S::Tag>;
        }
    };

    using type = tl::Filter<Pred, StateSpecs>;
};

template <typename Tag, tl::IsList StateSpecs>
using FilterStateSpecsByTag = typename FilterStateSpecsByTagI<Tag, StateSpecs>::type;

template <tl::IsList Transitions>
struct GetStateSpecsI {
    template <Transition T>
    struct SrcSpecs {
        struct Mapper {
            template <typename Id>
            using Map = StateSpec<Id, typename T::Src::Tag>;
        };
        using type = tl::Unique<tl::Map<Mapper, typename T::Src::Ids>>;
    };

    template <Transition T>
    struct DstSpecs {
        using type = tl::List<StateSpec<typename T::Dst::Id, typename T::Dst::Tag>>;
    };

    struct Mapper {
        template <Transition T>
        using Map = tl::Concat<typename SrcSpecs<T>::type, typename DstSpecs<T>::type>;
    };

    struct Pred {
        template <typename S>
        static constexpr bool test() {
            return !tl::Contains<EphemeralStateIds, typename S::Id>;
        }
    };

    using type = tl::Filter<Pred, tl::Unique<tl::Flatten<tl::Map<Mapper, Transitions>>>>;
};

// Filters ephemeral states
template <tl::IsList Transitions>
using GetStateSpecs = typename GetStateSpecsI<Transitions>::type;

template <tl::IsList Transitions, tl::IsList AllStateSpecs>
struct GetSrcSpecsI {
    struct Mapper {
        template <SrcState S>
        struct Specs {
            using Ids = typename S::Ids;
            using Tag = typename S::Tag;

            struct Mapper {
                template <typename Id>
                using Map = StateSpec<Id, typename S::Tag>;
            };

            using type = std::conditional_t< //
                tl::Empty<Ids>,
                FilterStateSpecsByTag<Tag, AllStateSpecs>,
                tl::Map<Mapper, Ids>>;
        };

        template <Transition T>
        using Map = typename Specs<typename T::Src>::type;
    };

    using type = tl::Unique<tl::Flatten<tl::Map<Mapper, Transitions>>>;
};

template <tl::IsList Transitions, tl::IsList AllStateSpecs>
using GetSrcSpecs = typename GetSrcSpecsI<Transitions, AllStateSpecs>::type;

template <tl::IsList Transitions>
struct GetEventIdsI {
    struct Mapper {
        template <Transition T>
        using Map = typename T::Event::Ids;
    };

    using type = tl::Unique<tl::Flatten<tl::Map<Mapper, Transitions>>>;
};

template <tl::IsList Transitions>
using GetEventIds = typename GetEventIdsI<Transitions>::type;

}  // namespace sml::impl::traits
