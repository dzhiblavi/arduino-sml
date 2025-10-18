#pragma once

#include "sml/ids.h"
#include "sml/impl/traits.h"

#include <supp/tuple.h>
#include <supp/type_list.h>

#include <array>
#include <tuple>

namespace sml::impl {

template <typename EId, tl::IsList Transitions>
class Dispatcher {
    using StateSpecs = traits::GetStateSpecs<Transitions>;
    using TransitionsTuple = tl::ApplyToTemplate<Transitions, std::tuple>;

    using EvTransitions = traits::FilterTransitionsByEventId<EId, Transitions>;
    using OutboundStateSpecs = traits::GetSrcSpecs<EvTransitions, StateSpecs>;

    static constexpr size_t NumOutboundStates = tl::Size<OutboundStateSpecs>;
    static constexpr auto StateInjection = tl::injection(StateSpecs{}, OutboundStateSpecs{});

 public:
    explicit Dispatcher(TransitionsTuple* ts) : transitions_{ts} {
        supp::constexprFor<0, NumOutboundStates, 1>([this](auto I) {
            handlers_[I] = &accept<tl::At<I, OutboundStateSpecs>>;
        });
    }

    int dispatch(int state_idx, const EId& id) {
        state_idx = static_cast<int>(StateInjection[state_idx]);
        if (state_idx == -1) {
            return -1;
        }
        return handlers_[state_idx](id, *transitions_);
    }

    template <typename S>
    struct MatchesSourceState {
        template <Transition T>
        static constexpr bool test() {
            using TrSrcTag = typename T::Src::Tag;
            using TrSrcIds = typename T::Src::Ids;
            constexpr auto tag_matches = std::same_as<TrSrcTag, typename S::Tag>;
            constexpr auto id_matches =
                tl::Empty<TrSrcIds> || tl::Contains<TrSrcIds, typename S::Id>;
            return tag_matches && id_matches;
        }
    };

    template <typename SrcSpec>
    static int accept(const EId& id, TransitionsTuple& transitions) {
        int dst = -1;
        auto matcher = [&]<Transition T>(tl::Type<T>) {
            using SrcId = typename SrcSpec::Id;
            using Dst = typename T::Dst;
            using DstId = typename Dst::Id;
            using DstSpec = std::conditional_t<
                std::same_as<KeepStateId, DstId>,
                SrcSpec,
                traits::StateSpec<DstId, typename Dst::Tag>>;

            if (!std::get<T>(transitions).template operator()<SrcId, EId>(SrcId{}, id)) {
                return false;
            }

            if constexpr (std::same_as<BypassStateId, DstId>) {
                return false;
            } else {
                static_assert(tl::Contains<StateSpecs, DstSpec>);
                dst = tl::Find<DstSpec, StateSpecs>;
                return true;
            }
        };

        // transitions that match both the event and the source state
        using Ts = tl::Filter<MatchesSourceState<SrcSpec>, EvTransitions>;

        tl::forEachShortCircuit(matcher, Ts{});
        return dst;
    }

 private:
    using HandlerFunc = int (*)(const EId&, TransitionsTuple&);

    TransitionsTuple* transitions_;
    std::array<HandlerFunc, NumOutboundStates> handlers_;
};

}  // namespace sml::impl
