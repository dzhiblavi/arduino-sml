#pragma once

#include "sml/ids.h"
#include "sml/impl/traits.h"

#include <supp/tuple.h>
#include <supp/type_list.h>

#include <array>
#include <tuple>

namespace sml::impl {

template <size_t TrIdx, typename EId, typename SrcSpec, typename StateSpecs, typename TrTuple>
int accept(const EId& id, TrTuple& transitions) {
    static constexpr size_t NumTransitions = std::tuple_size_v<TrTuple>;

    if constexpr (TrIdx == NumTransitions) {
        return -1;
    } else {
        using Tr = std::tuple_element_t<TrIdx, TrTuple>;

        using TrSrcTag = typename Tr::Src::Tag;
        using TrSrcIds = typename Tr::Src::Ids;
        using TrEventIds = typename Tr::Event::Ids;

        using SrcTag = typename SrcSpec::Tag;
        using SrcId = typename SrcSpec::Id;

        using Dst = typename Tr::Dst;
        using DstId = typename Dst::Id;

        static constexpr bool bypass = std::same_as<BypassStateId, DstId>;
        using DstSpec = std::conditional_t< //
            std::same_as<KeepStateId, typename Dst::Id>,
            SrcSpec,
            traits::StateSpec<typename Dst::Id, typename Dst::Tag>
        >;

        static constexpr bool transition_matches = [] {
            constexpr auto tag_matches = std::same_as<TrSrcTag, SrcTag>;
            constexpr auto id_matches = tl::Empty<TrSrcIds> || tl::Contains<TrSrcIds, SrcId>;
            constexpr auto event_matches = tl::Empty<TrEventIds> || tl::Contains<TrEventIds, EId>;
            return tag_matches && id_matches && event_matches;
        }();

        if constexpr (transition_matches) {
            Tr& t = std::get<TrIdx>(transitions);

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
    using TransitionsTuple = tl::ApplyToTemplate<Transitions, std::tuple>;

    using EvTransitions = traits::FilterTransitionsByEventId<EId, Transitions>;
    using OutboundStateSpecs = traits::GetSrcSpecs<EvTransitions, StateSpecs>;

    // number of outbound states: unique (M, Id)
    static constexpr size_t NumOutboundStates = tl::Size<OutboundStateSpecs>;

    // construct an injection from all states to outbound states
    static constexpr auto StateInjection = tl::injection(StateSpecs{}, OutboundStateSpecs{});

 public:
    explicit Dispatcher(TransitionsTuple* ts) : transitions_{ts} {
        supp::constexprFor<0, NumOutboundStates, 1>([this](auto I) {
            using Spec = tl::At<I, OutboundStateSpecs>;
            handlers_[I] = &accept<0, EId, Spec, StateSpecs, TransitionsTuple>;
        });
    }

    int dispatch(int state_idx, const EId& id) {
        state_idx = static_cast<int>(StateInjection[state_idx]);
        if (state_idx == -1) {  // NOLINT
            return -1;
        }
        return handlers_[state_idx](id, *transitions_);
    }

 private:
    using HandlerFunc = int (*)(const EId&, TransitionsTuple&);

    TransitionsTuple* transitions_;
    std::array<HandlerFunc, NumOutboundStates> handlers_;
};

}  // namespace sml::impl
