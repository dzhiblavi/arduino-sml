#pragma once

#include "sml/ids.h"
#include "sml/impl/make.h"
#include "sml/impl/ref.h"

#include <supp/type_list.h>

namespace sml::impl::traits {

template <StateMachine M, tl::IsList UIds>
struct FilterUIdsByMachine {
    struct Pred {
        template <RefUId UId>
        static constexpr bool test() {
            return stdlike::same_as<M, typename UId::M>;
        }
    };

    using type = tl::Filter<Pred, UIds>;
};

template <typename EId, tl::IsList Transitions>
struct FilterTransitionsByEventId {
    struct Pred {
        template <typename T>
        static constexpr bool test() {
            return stdlike::same_as<EId, typename T::Event::Id>;
        }
    };

    using type = tl::Filter<Pred, Transitions>;
};

template <typename UId, tl::IsList AllUIds>
struct ExpandAnyIdsInUId {
    using type = tl::List<UId>;
};

template <StateMachine M, typename... Ids, tl::IsList AllUIds>
requires(sizeof...(Ids) > 0)
struct ExpandAnyIdsInUId<impl::RefUId<M, AnyId<Ids...>>, AllUIds> {
    using type = tl::Unique<tl::List<impl::RefUId<M, Ids>...>>;
};

template <StateMachine M, typename UIds>
struct ExpandAnyIdsInUId<impl::RefUId<M, AnyId<>>, UIds> {
    using MachinesUIds = typename FilterUIdsByMachine<M, UIds>::type;

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

    using type = tl::Unique<tl::Flatten<tl::Map<Mapper, MachinesUIds>>>;
};

template <tl::IsList UIds>
struct ExpandAnyIdsI {
    struct Mapper {
        template <typename UId>
        using Map = typename ExpandAnyIdsInUId<UId, UIds>::type;
    };
    using type = tl::Unique<tl::Flatten<tl::Map<Mapper, UIds>>>;
};

template <tl::IsList UIds>
using ExpandAnyIds = typename ExpandAnyIdsI<UIds>::type;

template <StateMachine M>
using TransitionsTuple = decltype(stdlike::declval<M>().transitions());

template <StateMachine M>
using Transitions = tl::ToList<TransitionsTuple<M>>;

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

template <StateMachine M>
struct MarkedTransitionsI {
    template <typename S>
    struct MapState {
        using Id = typename S::Id;
        using UId = typename S::UId;
        using RefUId = impl::RefUId<M, Id>;
        static constexpr bool is_reference = IsRefUId<UId>::value;

        using type = impl::state::Make<Id, stdlike::conditional_t<is_reference, UId, RefUId>>;
    };

    template <>
    struct MapState<void> {
        using type = void;
    };

    template <Transition T>
    struct MapTransition {
        using NewSrc = typename MapState<typename T::Src>::type;
        using NewDst = typename MapState<typename T::Dst>::type;
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

template <StateMachine M>
struct CombinedTransitions {
    struct SubmMapper {
        template <StateMachine Subm>
        using Map = MarkedTransitions<Subm>;
    };

    using type = tl::Concat<MarkedTransitions<M>, tl::Flatten<tl::Map<SubmMapper, Submachines<M>>>>;
};

template <StateMachine M>
struct CombinedStateUIds {
    using Trs = typename CombinedTransitions<M>::type;

    struct TrMapper {
        template <Transition T>
        using Map = tl::List<typename T::Src::UId, typename T::Dst::UId>;
    };

    using type = tl::Unique<tl::Flatten<tl::Map<TrMapper, Trs>>>;
};

template <StateMachine M>
struct CombinedEventIds {
    using Trs = typename CombinedTransitions<M>::type;

    struct TrMapper {
        template <Transition T>
        using Map = typename T::Event::Id;
    };

    using type = tl::Unique<tl::Map<TrMapper, Trs>>;
};

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
    using Transitions = typename CombinedTransitions<M>::type;

    using MachinesTuple = tl::ApplyToTemplate<Machines, stdlike::tuple>;
    using TransitionsTuple = tl::ApplyToTemplate<Transitions, stdlike::tuple>;

    MachinesTuple machines_;
};

template <typename UId>
struct UIdSubmachineI {
    using type = void;
};

template <StateMachine M, typename Id>
struct UIdSubmachineI<impl::RefUId<M, Id>> {
    using type = M;
};

template <typename UId>
using UIdSubmachine = typename UIdSubmachineI<UId>::type;

}  // namespace sml::impl::traits
