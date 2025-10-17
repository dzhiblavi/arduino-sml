#pragma once

#include "sml/ids.h"
#include "sml/impl/traits.h"
#include "sml/model.h"

#include <stdlike/array.h>
#include <supp/type_list.h>

namespace sml::impl {

template <size_t TrIdx, typename EId, typename SUId, typename StateUIds, typename EvTrTuple>
int accept(const EId& id, EvTrTuple& transitions) {
    static constexpr size_t NumTransitions = stdlike::tuple_size_v<EvTrTuple>;

    if constexpr (TrIdx == NumTransitions) {
        return -1;
    } else {
        using Tr = stdlike::tuple_element_t<TrIdx, EvTrTuple>;
        using TrSrcUId = typename Tr::Src::UId;
        using TrSrcSM = traits::UIdSubmachine<TrSrcUId>;
        using SrcSM = traits::UIdSubmachine<SUId>;
        using SrcId = typename SUId::Id;
        using TrSrcId = typename TrSrcUId::Id;

        static constexpr bool transition_src_matches = [] {
            constexpr auto src_matches = stdlike::same_as<SUId, TrSrcUId>;
            constexpr auto sm_matches = stdlike::same_as<TrSrcSM, SrcSM>;
            constexpr auto is_any_src_id = sml::traits::IsAnyId<TrSrcId>;

            if constexpr (src_matches) {
                return true;
            } else if constexpr (sm_matches && is_any_src_id) {
                return TrSrcId::template matches<SrcId>();
            } else {
                return false;
            }
        }();

        if constexpr (transition_src_matches) {
            Tr& transition = stdlike::get<TrIdx>(transitions);
            if (transition.template tryExecute<SrcId>(SrcId{}, id)) {
                using Dst = typename Tr::Dst;

                if constexpr (stdlike::same_as<void, Dst>) {
                    static_assert(tl::Contains<StateUIds, SUId>);
                    return tl::Find<SUId, StateUIds>;
                } else {
                    static_assert(tl::Contains<StateUIds, typename Dst::UId>);
                    return tl::Find<typename Dst::UId, StateUIds>;
                }
            }

            return accept<TrIdx + 1, EId, SUId, StateUIds>(id, transitions);
        } else {
            return accept<TrIdx + 1, EId, SUId, StateUIds>(id, transitions);
        }
    }
}

template <typename EId, tl::IsList Transitions, tl::IsList StateUIds>
class Dispatcher {
    using EvTransitions = typename traits::FilterTransitionsByEventId<EId, Transitions>::type;
    using EvTransitionsTuple = tl::ApplyToTemplate<EvTransitions, stdlike::tuple>;
    using TransitionsTuple = tl::ApplyToTemplate<Transitions, stdlike::tuple>;

    // get only states with matching outgoing transitions
    // while collecting all uids that are specified in AnyIds
    struct Mapper {
        template <typename T>
        using Map = typename traits::ExpandAnyIdsInUId<typename T::Src::UId, StateUIds>::type;
    };
    using OutboundStateUIds = tl::Unique<tl::Flatten<tl::Map<Mapper, EvTransitions>>>;
    static constexpr size_t NumOutboundStates = tl::Size<OutboundStateUIds>;

    // construct an injection from all states to outbound states
    static constexpr auto StateInjection = tl::injection(StateUIds{}, OutboundStateUIds{});

 public:
    explicit Dispatcher(TransitionsTuple& ts) : transitions_{extractEvTransitions(ts)} {
        stdlike::constexprFor<0, NumOutboundStates, 1>([this](auto I) {
            using UId = typename tl::At<I, OutboundStateUIds>::type;
            handlers_[I] = &accept<0, EId, UId, StateUIds, EvTransitionsTuple>;
        });
    }

    int dispatch(int state_idx, const EId& id) {
        state_idx = StateInjection[state_idx];
        if (state_idx == size_t(-1)) {  // NOLINT
            return -1;
        }
        return handlers_[state_idx](id, transitions_);
    }

 private:
    EvTransitionsTuple extractEvTransitions(TransitionsTuple& ts) {
        return tl::apply(
            [&]<Transition... T>(tl::Type<T>...) {
                return EvTransitionsTuple{
                    stdlike::move(stdlike::get<tl::Find<T, Transitions>>(ts))...,
                };
            },
            EvTransitions{});
    }

    using HandlerFunc = int (*)(const EId&, EvTransitionsTuple&);

    EvTransitionsTuple transitions_;
    stdlike::array<HandlerFunc, NumOutboundStates> handlers_;
};

}  // namespace sml::impl
