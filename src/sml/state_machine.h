#pragma once

#include "sml/event.h"
#include "sml/model.h"
#include "sml/operators.h"  // IWYU pragma: keep
#include "sml/state.h"      // IWYU pragma: keep
#include "sml/traits.h"
#include "sml/transition.h"  // IWYU pragma: keep

#include <stdlike/array.h>
#include <stdlike/tuple.h>

#include <supp/type_list.h>
#include <supp/verify.h>

namespace sm {

// Replace outgoing transitions of type
//   state -> State<SM>
// to
//   state -> initial(SM)
template <typename Transitions>  // tl::List
struct ReplaceOutgoingTransitions {
    struct Mapper {
        template <model::Transition T>
        using Dst = typename T::DstState::type;  // may be a StateMachine

        template <model::Transition T>
        using Initial = typename traits::InitialState<Dst<T>>::type;

        template <model::Transition T>
        using TaggedInitial = typename Initial<T>::template TagIfUntagged<Dst<T>>;

        template <model::Transition T, bool>
        struct MapT {
            using type = T;
        };

        template <model::Transition T>
        struct MapT<T, true> {
            using type = typename T::template ReplaceDst<TaggedInitial<T>>;
        };

        template <model::Transition T>
        using Map = typename MapT<T, model::StateMachine<typename T::DstState::type>>::type;
    };

    using type = tl::Map<Mapper, Transitions>;
};

template <model::StateMachine Machine>
struct UnfoldedMachine {
    using Transitions = typename ReplaceOutgoingTransitions<  //
        typename traits::AllTransitions<Machine>::type        //
        >::type;

    using Table = tl::ApplyToTemplate<Transitions, stdlike::tuple>;
    using Submachines = typename traits::SubmachinesOf<Machine>::type;

    template <model::StateMachine... Machines>
    UnfoldedMachine(Machines... machines) : submachines_{stdlike::move(machines)...} {}

    Table operator()() {
        return stdlike::tuple_convert<Table>(stdlike::apply(
            []<model::StateMachine... M>(M&... m) { return stdlike::tuple_cat(m()...); },
            submachines_));
    }

 private:
    using MachinesTuple = tl::ApplyToTemplate<tl::PushFront<Submachines, Machine>, stdlike::tuple>;
    MachinesTuple submachines_;
};

template <typename TableType>
struct Traits;

template <typename... Ts>
struct Traits<stdlike::tuple<Ts...>> {
    using TransitionsTuple = stdlike::tuple<Ts...>;
    using Transitions = tl::List<Ts...>;
    using RawEvents = tl::Unique<tl::List<typename Ts::Event::Raw...>>;
    using States = tl::Unique<tl::List<typename Ts::SrcState..., typename Ts::DstState...>>;
};

template <size_t I, typename State, typename RawEvent, typename TsTuple, typename AllStates>
int accept(RawEvent event, TsTuple& ts) {
    static constexpr size_t NumTransitions = std::tuple_size_v<TsTuple>;

    if constexpr (I == NumTransitions) {
        return -1;
    } else {
        using T = std::tuple_element_t<I, TsTuple>;

        if constexpr (!stdlike::is_same_v<State, typename T::SrcState>) {
            return accept<I + 1, State, RawEvent, TsTuple, AllStates>(stdlike::move(event), ts);
        } else {
            T& transition = get<I>(ts);

            if (transition.execute(event)) {
                static_assert(tl::Contains<AllStates, typename T::DstState>);
                return tl::Find<typename T::DstState, AllStates>;
            } else {
                return accept<I + 1, State, RawEvent, TsTuple, AllStates>(stdlike::move(event), ts);
            }
        }
    }
}

template <typename RawEvent, typename Traits>
class EventDispatcher {
    using AllTransitions = typename Traits::Transitions;
    using AllStates = typename Traits::States;

    // Transitions with matching RawEvent
    struct TransitionPredicate {
        template <typename T>
        static constexpr bool test() {
            return stdlike::same_as<RawEvent, typename T::Event::Raw>;
        }
    };
    using Transitions = tl::Filter<TransitionPredicate, AllTransitions>;
    using TransitionsTuple = tl::ApplyToTemplate<Transitions, stdlike::tuple>;
    static constexpr size_t NumTransitions = tl::Size<Transitions>;

    // States with outgoing RawEvent transitions
    struct SourceStateMapper {
        template <typename Tr>
        using Map = typename Tr::SrcState;
    };
    using States = tl::Unique<tl::Map<SourceStateMapper, Transitions>>;
    static constexpr size_t NumStates = tl::Size<States>;

    // all states -> states
    static constexpr auto StateInjection = tl::injection(AllStates{}, States{});

 public:
    // Moves out Transitions that will be used by this instance
    explicit EventDispatcher(typename Traits::TransitionsTuple& ts)
        : transitions_{tl::apply(
              [&]<model::Transition... T>(tl::Type<T>...) {
                  return TransitionsTuple{
                      stdlike::move(stdlike::get<tl::Find<T, AllTransitions>>(ts))...,
                  };
              },
              Transitions{})} {
        stdlike::constexprFor<0, NumStates, 1>([this](auto I) {
            using State = typename tl::At<I, States>::type;
            handlers_[I] = &accept<0, State, RawEvent, TransitionsTuple, AllStates>;
        });
    }

    // Returns the state index in all states
    int dispatch(int all_state_idx, RawEvent event) {
        size_t state_idx = StateInjection[all_state_idx];

        if (state_idx == size_t(-1)) {
            // Current state has no candidate transitions.
            return -1;
        }

        return handlers_[state_idx](stdlike::move(event), transitions_);
    }

 private:
    using Handler = int (*)(RawEvent event, TransitionsTuple&);

    TransitionsTuple transitions_;
    stdlike::array<Handler, NumStates> handlers_;
};

template <model::StateMachine OriginalMachine>
class SM {
    using Machine = UnfoldedMachine<OriginalMachine>;
    using TableType = decltype(stdlike::declval<Machine>()());
    using Tr = Traits<TableType>;
    using InitialState = typename traits::InitialState<Machine>::type;
    using States = typename Tr::States;

    struct EventDispatcherMapper {
        template <typename RawEvent>
        using Map = EventDispatcher<RawEvent, Tr>;
    };
    using EventDispatchers = tl::Map<EventDispatcherMapper, typename Tr::RawEvents>;
    using EventDispatchersTuple = tl::ApplyToTemplate<EventDispatchers, stdlike::tuple>;

 public:
    template <typename RawEvent>
    static constexpr bool SupportsEvent = tl::Contains<typename Tr::RawEvents, RawEvent>;
    static constexpr size_t NumStates = tl::Size<States>;
    static constexpr size_t NumTransitions = tl::Size<typename Tr::Transitions>;

    // Machines should be passed in the order of their appearance
    // in the depth first search in transition tables.
    template <model::StateMachine... Machines>
    SM(Machines&&... machines)
        : machine_{Machine{stdlike::forward<Machines>(machines)...}}
        , ev_dispatchers_{[](auto ts) {
            return tl::apply(
                [&]<typename... RawEvent>(tl::Type<RawEvent>...) {
                    return EventDispatchersTuple{EventDispatcher<RawEvent, Tr>(ts)...};
                },
                typename Tr::RawEvents{});
        }(machine_())}
        , state_idx_{tl::Find<InitialState, typename Tr::States>} {}

    void begin() {
        feed(OnEnterEvent{});
    }

    template <typename State>
    [[nodiscard]] bool is(State = {}) const {
        return state_idx_ == tl::Find<State, typename Tr::States>;
    }

    template <typename RawEvent>
    bool feed(RawEvent event) {
        if constexpr (SupportsEvent<RawEvent>) {
            return feedImpl(stdlike::move(event));
        } else {
            return false;
        }
    }

 private:
    template <typename RawEvent>
    bool feedImpl(RawEvent event) {
        using Dispatcher = EventDispatcher<RawEvent, Tr>;
        auto&& dispatcher = get<Dispatcher>(ev_dispatchers_);

        int dst_state = dispatcher.dispatch(state_idx_, stdlike::move(event));
        if (dst_state == -1) {
            return false;
        }

        if (dst_state != state_idx_) {
            feed(OnExitEvent{});
            state_idx_ = dst_state;
            feed(OnEnterEvent{});
        }

        return true;
    }

    Machine machine_;
    EventDispatchersTuple ev_dispatchers_;
    int state_idx_ = 0;
};

template <model::Transition... Ts>
auto makeTransitionTable(Ts... ts) {
    return stdlike::tuple<Ts...>(stdlike::move(ts)...);
}

}  // namespace sm
