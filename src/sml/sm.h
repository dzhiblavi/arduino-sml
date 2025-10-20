#pragma once

#include "sml/impl/dispatcher.h"
#include "sml/impl/traits.h"

namespace sml {

template <StateMachine TM>
class SM {
 public:
    template <StateMachine... Machines>
    explicit SM(Machines&&... machines)
        : machine_{std::move(machines)...}
        , transitions_{machine_.transitions()}
        , dispatchers_{[this] {
            return tl::apply(
                [&]<typename... EId>(tl::Type<EId>...) {
                    return DispatchersTuple{impl::Dispatcher<EId, Trs>(&transitions_)...};
                },
                EIds{});
        }()} {}

    void begin() {
        feed(OnEnterEventId{});
    }

    template <typename EId>
    bool feed(const EId& event) {
        if constexpr (SupportsEvent<EId>) {
            return feedImpl(event);
        } else {
            return false;
        }
    }

    template <StateMachine M, typename Id>
    bool is() const {
        using Spec = impl::traits::StateSpec<Id, M>;
        return state_idx_ == tl::Find<Spec, StateSpecs>;
    }

    void reset() {
        state_idx_ = static_cast<int>(tl::Find<InitialSpec, StateSpecs>);
    }

 private:
    using InitialSpec = impl::traits::StateSpec<typename TM::InitialId, TM>;
    using M = impl::traits::CombinedStateMachine<TM>;
    using Trs = impl::traits::Transitions<M>;
    using TrsTuple = impl::traits::TransitionsTuple<M>;
    using EIds = impl::traits::GetEventIds<Trs>;
    using StateSpecs = impl::traits::GetStateSpecs<Trs>;

    struct DispatcherMapper {
        template <typename EId>
        using Map = impl::Dispatcher<EId, Trs>;
    };
    using Dispatchers = tl::Map<DispatcherMapper, EIds>;
    using DispatchersTuple = tl::ApplyToTemplate<Dispatchers, std::tuple>;

    template <typename EId>
    static constexpr bool SupportsEvent = tl::Contains<EIds, EId>;

    template <typename RawEvent>
    bool feedImpl(RawEvent event) {
        using Dispatcher = impl::Dispatcher<RawEvent, Trs>;
        auto&& dispatcher = std::get<Dispatcher>(dispatchers_);

        int dst_state = dispatcher.dispatch(state_idx_, event);
        if (dst_state == -1) {
            return false;
        }

        if (dst_state != state_idx_) {
            feed(OnExitEventId{});
            state_idx_ = dst_state;
            feed(OnEnterEventId{});
        }

        return true;
    }

    M machine_;
    TrsTuple transitions_;
    DispatchersTuple dispatchers_;
    int state_idx_ = static_cast<int>(tl::Find<InitialSpec, StateSpecs>);
};

}  // namespace sml
