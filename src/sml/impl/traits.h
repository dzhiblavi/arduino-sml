#pragma once

#include "sml/ids.h"
#include "sml/impl/make.h"
#include "sml/impl/ref.h"

#include <supp/type_list.h>

namespace sml::impl::traits {

template <StateMachine M>
using TransitionsTuple = decltype(stdlike::declval<M>().transitions());

template <StateMachine M>
using Transitions = tl::ToList<TransitionsTuple<M>>;

// Filters UIds list by state machine
// UIds is expected to be a list of RefUIds
template <StateMachine M, tl::IsList UIds>
struct FilterUIdsByMachineI {
    struct Pred {
        template <RefUId UId>
        static constexpr bool test() {
            return stdlike::same_as<M, typename UId::M>;
        }
    };

    using type = tl::Filter<Pred, UIds>;
};
template <StateMachine M, tl::IsList UIds>
using FilterUIdsByMachine = typename FilterUIdsByMachineI<M, UIds>::type;

// Filters transitions by event Id
template <typename EId, tl::IsList Transitions>
struct FilterTransitionsByEventIdI {
    struct Pred {
        template <typename T>
        static constexpr bool test() {
            return stdlike::same_as<EId, typename T::Event::Id>;
        }
    };

    using type = tl::Filter<Pred, Transitions>;
};
template <typename EId, tl::IsList Transitions>
using FilterTransitionsByEventId = typename FilterTransitionsByEventIdI<EId, Transitions>::type;

// Given an UId (of any kind), expands UIds of form RefUId<M, AnyId<...>>
// So that all AnyIds are expanded into multiple distinct UIds with plain Ids
template <typename UId, tl::IsList AllUIds>
struct ExpandAnyIdsInUIdI {
    using type = tl::List<UId>;
};

template <StateMachine M, typename... Ids, tl::IsList AllUIds>
requires(sizeof...(Ids) > 0)
struct ExpandAnyIdsInUIdI<impl::RefUId<M, AnyId<Ids...>>, AllUIds> {
    using type = tl::Unique<tl::List<impl::RefUId<M, Ids>...>>;
};

template <StateMachine M, typename UIds>
struct ExpandAnyIdsInUIdI<impl::RefUId<M, AnyId<>>, UIds> {
    template <RefUId UId>
    struct Map {
        using Ids = typename sml::traits::Ids<typename UId::Id>::type;

        struct Mapper {
            template <typename Id>
            using Map = impl::RefUId<M, Id>;
        };

        using type = tl::Map<Mapper, Ids>;
    };

    struct Mapper {
        template <RefUId UId>
        using Map = typename Map<UId>::type;
    };

    using type = tl::Unique<tl::Flatten<tl::Map<Mapper, FilterUIdsByMachine<M, UIds>>>>;
};

template <typename UId, tl::IsList AllUIds>
using ExpandAnyIdsInUId = typename ExpandAnyIdsInUIdI<UId, AllUIds>::type;

// Similar to ExpandAnyIdsInUId, but accepts a list of UIds
// and expands all of them
template <tl::IsList UIds, tl::IsList AllUIds>
struct ExpandAnyIdsI {
    struct Mapper {
        template <typename UId>
        using Map = ExpandAnyIdsInUId<UId, AllUIds>;
    };
    using type = tl::Unique<tl::Flatten<tl::Map<Mapper, UIds>>>;
};
template <tl::IsList UIds, tl::IsList AllUIds = UIds>
using ExpandAnyIds = typename ExpandAnyIdsI<UIds, AllUIds>::type;

// Retrieves a list of Event Ids from transitions list
template <tl::IsList Transitions>
struct EventIdsI {
    struct Mapper {
        template <Transition T>
        using Map = typename T::Event::Id;
    };

    using type = tl::Unique<tl::Map<Mapper, Transitions>>;
};
template <tl::IsList Transitions>
using EventIds = typename EventIdsI<Transitions>::type;

// Retrieves all State UIds from transitions list
// Both source and destination UIds are included
// InitUId is included separately for the case when it is not used in
// any of the transitions
template <typename InitUId, tl::IsList Transitions>
struct StateUIdsI {
    template <Transition T, bool>
    struct DstUId {
        using type = tl::List<>;
    };

    template <Transition T>
    struct DstUId<T, true> {
        using type = tl::List<typename T::Dst::UId>;
    };

    struct Mapper {
        template <Transition T>
        using Map = tl::PushFront<
            typename DstUId<T, !stdlike::same_as<void, typename T::Dst>>::type,
            typename T::Src::UId>;
    };

    using type = tl::Unique<tl::PushFront<tl::Flatten<tl::Map<Mapper, Transitions>>, InitUId>>;
};
template <typename InitUId, tl::IsList Transitions>
using StateUIds = typename StateUIdsI<InitUId, Transitions>::type;

// Retrieves all source State UIds from transitions list
template <tl::IsList Transitions>
struct SrcUIdsI {
    struct Mapper {
        template <Transition T>
        using Map = typename T::Src::UId;
    };

    using type = tl::Unique<tl::Map<Mapper, Transitions>>;
};
template <tl::IsList Transitions>
using SrcUIds = typename SrcUIdsI<Transitions>::type;

// Retrieves a list of state machine's submachines.
// The machine M itself is not included.
template <StateMachine M, tl::IsList Seen = tl::List<>>
struct SubmachinesI {
    template <Transition, bool>
    struct MapperI {
        using type = tl::List<>;
    };

    template <Transition T>
    struct MapperI<T, true> {
        using Submachine = typename T::Dst::UId::M;
        using NewSeen = tl::PushFront<Seen, Submachine>;
        using type = tl::PushBack<typename SubmachinesI<Submachine, NewSeen>::type, Submachine>;
    };

    struct Mapper {
        template <Transition T>
        using Dst = typename T::Dst;

        template <Transition T>
        using Map =
            typename MapperI<T, IsRefState<Dst<T>>::value && !tl::Contains<Seen, Dst<T>>>::type;
    };

    using type = tl::Unique<tl::Flatten<tl::Map<Mapper, Transitions<M>>>>;
};
template <StateMachine M>
using Submachines = typename SubmachinesI<M>::type;

template <StateMachine M, typename S>
struct MapState {
    using Id = typename S::Id;
    using UId = typename S::UId;
    using RefUId = impl::RefUId<M, Id>;
    static constexpr bool is_reference = IsRefUId<UId>::value;

    using type = impl::state::Make<Id, stdlike::conditional_t<is_reference, UId, RefUId>>;
};

template <StateMachine M>
struct MapState<M, void> {
    using type = void;
};

// Marks all M's transitions' states using RefUId.
// Does not touch states that are already RefUIds because they can be referencing
// another submachine (i.e. exit/enter).
template <StateMachine M>
struct MarkedTransitionsI {
    template <Transition T>
    struct MapTransition {
        using NewSrc = typename MapState<M, typename T::Src>::type;
        using NewDst = typename MapState<M, typename T::Dst>::type;
        using type = impl::transition::Replace<T, NewSrc, NewDst>;
    };

    struct Mapper {
        template <Transition T>
        using Map = typename MapTransition<T>::type;
    };

    using type = tl::Map<Mapper, Transitions<M>>;
};
template <StateMachine M>
using MarkedTransitions = typename MarkedTransitionsI<M>::type;

// 1. Finds all M's submachine including itself.
// 2. Collects transitions of each submachine and marks all states there appropriately.
// 3. Collects all resulting transitions into a single unique list.
template <StateMachine M>
struct CombinedTransitionsI {
    struct SubmMapper {
        template <StateMachine Subm>
        using Map = MarkedTransitions<Subm>;
    };

    using type = tl::Concat<MarkedTransitions<M>, tl::Flatten<tl::Map<SubmMapper, Submachines<M>>>>;
};
template <StateMachine M>
using CombinedTransitions = typename CombinedTransitionsI<M>::type;

template <StateMachine M>
class CombinedStateMachine {
 public:
    using InitialId = typename M::InitialId;

    template <StateMachine... Machines>
    explicit CombinedStateMachine(Machines... machines) : machines_{stdlike::move(machines)...} {}

    sml::TransitionsTuple auto transitions() {
        return stdlike::tuple_convert<TransitionsTuple>(stdlike::apply(
            [](auto&... m) { return stdlike::tuple_cat(m.transitions()...); }, machines_));
    }

 private:
    using Machines = tl::PushFront<Submachines<M>, M>;
    using Transitions = CombinedTransitions<M>;

    using MachinesTuple = tl::ApplyToTemplate<Machines, stdlike::tuple>;
    using TransitionsTuple = tl::ApplyToTemplate<Transitions, stdlike::tuple>;

    MachinesTuple machines_;
};

}  // namespace sml::impl::traits
