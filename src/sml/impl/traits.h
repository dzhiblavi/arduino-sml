#pragma once

#include "sml/impl/ids.h"
#include "sml/impl/make.h"
#include "sml/impl/ref.h"

#include <supp/type_list.h>

namespace sml::impl::traits {

template <StateMachine M, tl::IsList AllStateUIds>
struct MachinesStateUIds {
    struct Pred {
        template <RefUId UId>
        static constexpr bool test() {
            return stdlike::same_as<M, typename UId::M>;
        }
    };

    using type = tl::Filter<Pred, AllStateUIds>;
};

template <typename UId, tl::IsList AllStateUIds>
struct AllUIdsAnyAware {
    using type = tl::List<UId>;
};

template <StateMachine M, typename... Ids, tl::IsList AllStateUIds>
requires(sizeof...(Ids) > 0)
struct AllUIdsAnyAware<impl::RefUId<M, impl::AnyId<Ids...>>, AllStateUIds> {
    using type = tl::List<impl::RefUId<M, Ids>...>;
};

template <StateMachine M, typename AllStateUIds>
struct AllUIdsAnyAware<impl::RefUId<M, impl::AnyId<>>, AllStateUIds> {
    // Get UIds that belong to said state machine
    using MachinesStateUIds = typename MachinesStateUIds<M, AllStateUIds>::type;

    struct Mapper {
        template <RefUId UId>
        using Map = impl::RefUId<M, typename UId::Id>;
    };

    using type = tl::Map<Mapper, MachinesStateUIds>;
};

template <StateMachine M>
using TransitionsTuple = decltype(stdlike::declval<M>().transitions());

template <StateMachine M>
using Transitions = tl::ToList<TransitionsTuple<M>>;

template <tl::IsList Transitions>
struct AllEventIds {
    struct Mapper {
        template <Transition T>
        using Map = typename T::Event::Id;
    };

    using type = tl::Unique<tl::Map<Mapper, Transitions>>;
};

template <typename InitUId, tl::IsList Transitions>
struct AllStateUIds {
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

template <StateMachine M, tl::IsList Seen = tl::List<>>
struct Submachines {
    template <Transition, bool>
    struct MapperI {
        using type = tl::List<>;
    };

    template <Transition T>
    struct MapperI<T, true> {
        using Submachine = typename T::Dst::UId::M;  // UId | impl::state::RefUId
        using NewSeen = tl::PushFront<Seen, Submachine>;
        using type = tl::PushBack<typename Submachines<Submachine, NewSeen>::type, Submachine>;
    };

    struct Mapper {
        template <Transition T>
        using Dst = typename T::Dst;  // State

        template <Transition T>
        using Map =
            typename MapperI<T, IsRefState<Dst<T>>::value && !tl::Contains<Seen, Dst<T>>>::type;
    };

    using type = tl::Unique<tl::Flatten<tl::Map<Mapper, Transitions<M>>>>;
};

template <StateMachine M>
struct MarkedTransitions {
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
struct CombinedTransitions {
    using Subs = typename Submachines<M>::type;

    struct SubmMapper {
        template <StateMachine Subm>
        using Map = typename MarkedTransitions<Subm>::type;
    };

    using type = tl::Concat< //
        typename MarkedTransitions<M>::type,
        tl::Flatten<tl::Map<SubmMapper, Subs>>
    >;
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
    using Machines = tl::PushFront<typename Submachines<M>::type, M>;
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
