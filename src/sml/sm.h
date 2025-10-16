#pragma once

#include "sml/impl/dispatcher.h"
#include "sml/traits.h"

namespace sml {

template <StateMachine TM>
class SM {
 public:
    template <StateMachine... Machines>
    explicit SM(Machines&&... machines)
        : machine_{stdlike::move(machines)...}
        , dispatchers_{[](auto ts) {
            return tl::apply(
                [&]<typename... EId>(tl::Type<EId>...) {
                    return DispatchersTuple{impl::Dispatcher<EId, Trs, UIds>(ts)...};
                },
                EIds{});
        }(machine_.transitions())} {}

    void begin() {
        feed(impl::OnEnterEventId{});
    }

    template <typename EId>
    bool feed(const EId& event) {
        if constexpr (SupportsEvent<EId>) {
            return feedImpl(event);
        } else {
            return false;
        }
    }

 private:
    using InitialUId = impl::RefUId<TM, typename TM::InitialId>;
    using M = traits::CombinedStateMachine<TM>;
    using Trs = traits::Transitions<M>;
    using UIds = traits::AllStateUIds<InitialUId, Trs>;
    using EIds = traits::AllEventIds<Trs>;

    struct DispatcherMapper {
        template <typename EId>
        using Map = impl::Dispatcher<EId, Trs, UIds>;
    };

    using Dispatchers = tl::Map<DispatcherMapper, EIds>;
    using DispatchersTuple = tl::ApplyToTemplate<Dispatchers, stdlike::tuple>;

    template <typename EId>
    static constexpr bool SupportsEvent = tl::Contains<EIds, EId>;

    template <typename RawEvent>
    bool feedImpl(RawEvent event) {
        using Dispatcher = impl::Dispatcher<RawEvent, Trs, UIds>;
        auto&& dispatcher = stdlike::get<Dispatcher>(dispatchers_);

        int dst_state = dispatcher.dispatch(state_idx_, event);
        if (dst_state == -1) {
            return false;
        }

        if (dst_state != state_idx_) {
            feed(impl::OnExitEventId{});
            state_idx_ = dst_state;
            feed(impl::OnEnterEventId{});
        }

        return true;
    }

    M machine_;
    DispatchersTuple dispatchers_;
    int state_idx_ = static_cast<int>(tl::Find<InitialUId, UIds>);
};

}  // namespace sml
