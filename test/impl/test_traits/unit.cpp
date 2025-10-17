#include <sml/impl/traits.h>
#include <sml/make.h>

#include <utest/utest.h>

struct S {};
struct E {};

namespace sml {

auto t1 = state<S>.on(event<E>).to(state<int>);
auto t2 = state<S>.on(event<E>).run(ConcreteAction{}).to(state<int>);
auto t3 = state<S>.on(event<E>.when(ConcreteCondition<E>{})).to(state<int>);

using T1 = decltype(t1);
using T2 = decltype(t2);
using T3 = decltype(t3);

TEST(test_transitions_tuple) {
    struct SM {
        using InitialId = S;

        auto transitions() {
            return stdlike::tuple(t1, t2, t3);
        }
    };

    using TrTuple = impl::traits::TransitionsTuple<SM>;
    static_assert(stdlike::same_as<stdlike::tuple<T1, T2, T3>, TrTuple>);
}

TEST(test_transitions) {
    struct SM {
        using InitialId = S;

        auto transitions() {
            return stdlike::tuple(t1, t2, t3);
        }
    };

    using TrTuple = impl::traits::Transitions<SM>;
    static_assert(stdlike::same_as<tl::List<T1, T2, T3>, TrTuple>);
}

TEST(test_event_ids) {
    struct SM {
        using InitialId = S;

        auto transitions() {
            return stdlike::tuple(t1, t2, state<S>.on(onExit));
        }
    };

    static_assert(stdlike::same_as<
                  tl::List<E, sml::OnExitEventId>,
                  impl::traits::EventIds<impl::traits::Transitions<SM>>>);
}

TEST(test_all_state_uids) {
    struct SM {
        using InitialId = S;

        auto transitions() {
            return stdlike::tuple(t1, t2, state<S>.on(onExit));
        }
    };

    static_assert(stdlike::same_as<
                  tl::List<sml::impl::RefUId<SM, S>, int, S>,
                  impl::traits::StateUIds<impl::RefUId<SM, S>, impl::traits::Transitions<SM>>>);
}

TEST(test_submachines) {
    struct M1 {
        using InitialId = S;

        auto transitions() {
            return stdlike::tuple(  //
                state<S>.on(onEnter).to(state<S>));
        }
    };
    static_assert(stdlike::same_as<tl::List<>, impl::traits::Submachines<M1>>);

    struct M2 {
        using InitialId = S;

        auto transitions() {
            return stdlike::tuple(//
                state<S>.on(onEnter).to(state<S>),
                state<S>.on(onExit).to(enter<M1>)
            );
        }
    };
    static_assert(stdlike::same_as<tl::List<M1>, impl::traits::Submachines<M2>>);

    struct M3 {
        using InitialId = S;

        auto transitions() {
            return stdlike::tuple( //
                state<S>.on(onEnter).to(enter<M1>),
                state<S>.on(onExit).to(enter<M2>)
            );
        }
    };
    static_assert(stdlike::same_as<tl::List<M1, M2>, impl::traits::Submachines<M3>>);
}

TEST(test_marked_transitions) {
    struct M1 {
        using InitialId = S;  // NOLINT
        auto transitions() {
            return stdlike::tuple();
        }
    };

    struct M2 {
        using InitialId = S;  // NOLINT
        auto transitions() {
            return stdlike::tuple(
                state<S>.on(onEnter).to(state<S>),
                state<S>.on(onExit).to(enter<M1>),
                exit<M1>.on(onEnter).to(state<S>));
        }
    };

    using namespace sml::impl;

    static_assert(
        stdlike::same_as<
            tl::List<
                transition::Replace<
                    transition::To<
                        transition::Make<state::Make<S>, event::Make<OnEnterEventId>>,
                        state::Make<S>>,
                    state::Make<S, RefUId<M2, S>>,
                    state::Make<S, RefUId<M2, S>>>,
                transition::Replace<
                    transition::To<
                        transition::Make<state::Make<S>, event::Make<OnExitEventId>>,
                        state::Make<S, RefUId<M1, S>>>,
                    state::Make<S, RefUId<M2, S>>,
                    state::Make<S, RefUId<M1, S>>>,
                transition::Replace<
                    transition::To<
                        transition::Make<
                            state::Make<TerminalStateId, RefUId<M1, TerminalStateId>>,
                            event::Make<OnEnterEventId>>,
                        state::Make<S>>,
                    state::Make<TerminalStateId, RefUId<M1, TerminalStateId>>,
                    state::Make<S, RefUId<M2, S>>>>,
            impl::traits::MarkedTransitions<M2>>);
}

TEST(test_combined_transitions) {
    struct M1 {
        using InitialId = S;  // NOLINT
        auto transitions() {
            return stdlike::tuple( //
                state<S>.on(onExit).to(state<S>),
                state<S>.on(onEnter).to(x)
            );
        }
    };

    struct M2 {
        using InitialId = S;  // NOLINT
        auto transitions() {
            return stdlike::tuple(
                state<S>.on(onEnter).to(state<S>),
                state<S>.on(onExit).to(enter<M1>),
                exit<M1>.on(onEnter).to(state<S>));
        }
    };

    using namespace sml::impl;

    static_assert(
        stdlike::same_as<
            tl::List<
                // M2: state<S>().on(onEnter).to(state<S>())
                transition::Replace<
                    transition::To<
                        transition::Make<state::Make<S>, event::Make<OnEnterEventId>>,
                        state::Make<S>>,
                    state::Make<S, RefUId<M2, S>>,
                    state::Make<S, RefUId<M2, S>>>,
                // M2: state<S>().on(onExit).to(enter<M1>())
                transition::Replace<
                    transition::To<
                        transition::Make<state::Make<S>, event::Make<OnExitEventId>>,
                        state::Make<S, RefUId<M1, S>>>,
                    state::Make<S, RefUId<M2, S>>,
                    state::Make<S, RefUId<M1, S>>>,
                // M2: exit<M1>().on(onEnter).to(state<S>())
                transition::Replace<
                    transition::To<
                        transition::Make<
                            state::Make<TerminalStateId, RefUId<M1, TerminalStateId>>,
                            event::Make<OnEnterEventId>>,
                        state::Make<S>>,
                    state::Make<TerminalStateId, RefUId<M1, TerminalStateId>>,
                    state::Make<S, RefUId<M2, S>>>,
                // M1: state<S>().on(onExit).to(state<S>())
                transition::Replace<
                    transition::To<
                        transition::Make<state::Make<S>, event::Make<OnExitEventId>>,
                        state::Make<S>>,
                    state::Make<S, RefUId<M1, S>>,
                    state::Make<S, RefUId<M1, S>>>,
                // M1: state<S>().on(onEnter).to(x)
                transition::Replace<
                    transition::To<
                        transition::Make<state::Make<S>, event::Make<OnEnterEventId>>,
                        state::Make<TerminalStateId>>,
                    state::Make<S, RefUId<M1, S>>,
                    state::Make<TerminalStateId, RefUId<M1, TerminalStateId>>>>,
            typename impl::traits::CombinedTransitions<M2>::type>);
}

TEST(test_combined_state_uids) {
    struct M1 {
        using InitialId = S;  // NOLINT
        auto transitions() {
            return stdlike::tuple( //
                state<S>.on(onExit).to(state<S>),
                state<S>.on(onEnter).to(x)
            );
        }
    };

    struct M2 {
        using InitialId = S;  // NOLINT
        auto transitions() {
            return stdlike::tuple(
                state<S>.on(onEnter).to(state<S>),
                state<S>.on(onExit).to(enter<M1>),
                exit<M1>.on(onEnter).to(state<S>));
        }
    };

    using namespace sml::impl;

    static_assert(stdlike::same_as<
                  tl::List<RefUId<M2, S>, RefUId<M1, S>, RefUId<M1, TerminalStateId>>,
                  typename impl::traits::CombinedStateUIds<M2>::type>);
}

TEST(test_combined_event_ids) {
    struct M1 {
        using InitialId = S;  // NOLINT
        auto transitions() {
            return stdlike::tuple( //
                state<S>.on(onExit).to(state<S>),
                state<S>.on(onEnter).to(x)
            );
        }
    };

    struct M2 {
        using InitialId = S;  // NOLINT
        auto transitions() {
            return stdlike::tuple(
                state<S>.on(onEnter).to(state<S>),
                state<S>.on(onExit).to(enter<M1>),
                exit<M1>.on(onEnter).to(state<S>));
        }
    };

    using namespace sml::impl;

    static_assert(stdlike::same_as<
                  tl::List<OnExitEventId, OnEnterEventId>,
                  typename impl::traits::CombinedEventIds<M2>::type>);
}

TEST(test_combined_state_machine) {
    struct M1 {
        using InitialId = S;  // NOLINT
        auto transitions() {
            return stdlike::tuple( //
                state<S>.on(onExit).to(state<S>),
                state<S>.on(onEnter).to(x)
            );
        }
    };

    struct M2 {
        using InitialId = S;  // NOLINT
        auto transitions() {
            return stdlike::tuple(
                state<S>.on(onEnter).to(state<S>),
                state<S>.on(onExit).to(enter<M1>),
                exit<M1>.on(onEnter).to(state<S>));
        }
    };

    using M = impl::traits::CombinedStateMachine<M2>;
    using namespace sml::impl;

    static_assert(stdlike::same_as<
                  typename impl::traits::CombinedTransitions<M2>::type,
                  impl::traits::Transitions<M>>);
}

}  // namespace sml

TESTS_MAIN
