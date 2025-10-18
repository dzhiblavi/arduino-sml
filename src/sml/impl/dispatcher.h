#pragma once

#include "sml/ids.h"
#include "sml/impl/traits.h"
#include "sml/model.h"

#include <stdlike/array.h>
#include <stdlike/tuple.h>
#include <supp/type_list.h>

namespace sml::impl {

template <size_t TrIdx, typename EId, typename SrcSpec, typename StateSpecs, typename EvTrTuple>
int accept(const EId& id, EvTrTuple& transitions) {
    static constexpr size_t NumTransitions = stdlike::tuple_size_v<EvTrTuple>;

    if constexpr (TrIdx == NumTransitions) {
        return -1;
    } else {
        using Tr = stdlike::tuple_element_t<TrIdx, EvTrTuple>;

        using TrSrcTag = typename Tr::Src::Tag;
        using TrSrcIds = typename Tr::Src::Ids;

        using SrcTag = typename SrcSpec::Tag;
        using SrcId = typename SrcSpec::Id;

        using Dst = typename Tr::Dst;
        using DstId = typename Dst::Id;

        static constexpr bool bypass = stdlike::same_as<BypassStateId, DstId>;
        using DstSpec = stdlike::conditional_t< //
            stdlike::same_as<KeepStateId, typename Dst::Id>,
            SrcSpec,
            traits::StateSpec<typename Dst::Id, typename Dst::Tag>
        >;

        static constexpr bool transition_src_matches = [] {
            constexpr auto tag_matches = stdlike::same_as<TrSrcTag, SrcTag>;
            constexpr auto id_matches = tl::Empty<TrSrcIds> || tl::Contains<TrSrcIds, SrcId>;
            return tag_matches && id_matches;
        }();

        if constexpr (transition_src_matches) {
            Tr& t = stdlike::get<TrIdx>(transitions);
            if (t.template operator()<SrcId, EId>(SrcId{}, id)) {
                if constexpr (!bypass) {
                    static_assert(tl::Contains<StateSpecs, DstSpec>);
                    return tl::Find<DstSpec, StateSpecs>;
                }
            }

            return accept<TrIdx + 1, EId, SrcSpec, StateSpecs>(id, transitions);
        } else {
            return accept<TrIdx + 1, EId, SrcSpec, StateSpecs>(id, transitions);
        }
    }
}

template <typename EId, tl::IsList Transitions>
class Dispatcher {
    using StateSpecs = traits::GetStateSpecs<Transitions>;
    using TransitionsTuple = tl::ApplyToTemplate<Transitions, stdlike::tuple>;

    using EvTransitions = traits::FilterTransitionsByEventId<EId, Transitions>;
    using EvTransitionsTuple = tl::ApplyToTemplate<EvTransitions, stdlike::tuple>;

    using OutboundStateSpecs = traits::GetSrcSpecs<EvTransitions, StateSpecs>;

    // number of outbound states: unique (M, Id)
    static constexpr size_t NumOutboundStates = tl::Size<OutboundStateSpecs>;

    // construct an injection from all states to outbound states
    static constexpr auto StateInjection = tl::injection(StateSpecs{}, OutboundStateSpecs{});

 public:
    explicit Dispatcher(TransitionsTuple& ts) : transitions_{extractEvTransitions(ts)} {
        stdlike::constexprFor<0, NumOutboundStates, 1>([this](auto I) {
            using Spec = tl::At<I, OutboundStateSpecs>;
            handlers_[I] = &accept<0, EId, Spec, StateSpecs, EvTransitionsTuple>;
        });
    }

    int dispatch(int state_idx, const EId& id) {
        state_idx = static_cast<int>(StateInjection[state_idx]);
        if (state_idx == -1) {  // NOLINT
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
