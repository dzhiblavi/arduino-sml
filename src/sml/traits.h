#pragma once

#include "sml/model.h"

#include <supp/type_list.h>

namespace sm::traits {

template <model::StateMachine Machine>
using TableTypeOf = decltype(stdlike::declval<Machine>()());  // TransitionsTuple

template <model::StateMachine Machine>
using TransitionsOf = tl::ToList<TableTypeOf<Machine>>;

template <model::StateMachine Machine, typename Seen = tl::List<>>
struct SubmachinesOf {
    template <model::Transition, bool>
    struct Helper {
        using type = tl::List<>;
    };

    template <model::Transition T>
    struct Helper<T, true> {
        using Submachine = typename T::DstState::type;
        using NewSeen = tl::PushFront<Seen, Submachine>;

        using type = tl::PushBack<
            // Search submachines recursively. Exclude current one to avoid cycles.
            typename SubmachinesOf<Submachine, NewSeen>::type,
            // Add this submachine
            Submachine>;
    };

    // Transition -> list of submachines (found recursively)
    struct Mapper {
        template <model::Transition T>
        using Dst = typename T::DstState::type;

        template <model::Transition T>
        using Map = typename Helper< //
            T,
            // Do not visit submachines that we have already seen.
            model::StateMachine<Dst<T>> && !tl::Contains<Seen, Dst<T>>
        >::type;
    };

    using Transitions = TransitionsOf<Machine>;
    using type = tl::Unique<tl::Flatten<tl::Map<Mapper, Transitions>>>;
};

// Collects all transitions from all submachines (recursively, cycles are tolerated).
// Makes Submachines' states all non initial and tags them appropriately.
template <model::StateMachine Machine>
struct AllTransitions {
    using Transitions = TransitionsOf<Machine>;
    using Submachines = typename SubmachinesOf<Machine>::type;

    // Adds tag to all untagged states. Tag is meant to be the machine type.
    template <typename Tag>
    struct TagMapper {
        template <model::Transition T>
        using Map = typename T::template TagIfUntagged<Tag>;
    };

    // Makes transitions not initial (for submachines).
    struct UnInitialMapper {
        template <model::Transition T>
        using Map = typename T::NonInitial;
    };

    // TagMapper + UnInitialMapper
    struct SubmachineMapper {
        template <model::StateMachine Submachine>
        using Map =
            tl::Map<UnInitialMapper, tl::Map<TagMapper<Submachine>, TransitionsOf<Submachine>>>;
    };

    using type = tl::Concat< //
        tl::Map<TagMapper<Machine>, Transitions>,
        tl::Flatten<tl::Map<SubmachineMapper, Submachines>>>;

    // There should be no duplicates
    static_assert(tl::Set<type>);
};

template <typename Machine>
struct InitialState {
    using type = void;
};

template <model::StateMachine Machine>
struct InitialState<Machine> {
    struct InitialTransitionPredicate {
        template <model::Transition T>
        static constexpr bool test() {
            return T::IsInitial;
        }
    };

    using Transitions = TransitionsOf<Machine>;
    using InitialTransition =
        typename tl::Head<tl::Filter<InitialTransitionPredicate, Transitions>>::type;

    using type = typename InitialTransition::SrcState;
};

}  // namespace sm::traits
