#include <sml/make.h>
#include <sml/model.h>

#include <utest/utest.h>

struct S {};
struct E {};

namespace sml {

TEST(test_state_factory) {
    auto s = state<S>;
    static_assert(State<decltype(s)>);
}

TEST(test_state_on) {
    auto s = state<S>;
    auto t = stdlike::move(s).on(ConcreteEvent{});
    static_assert(Transition<decltype(t)>);
}

TEST(test_event_factory) {
    auto e = event<E>;
    static_assert(Event<decltype(e)>);
}

TEST(test_event_match) {
    auto e = event<E>;
    TEST_ASSERT_TRUE(e.match(S{}, E{}));
}

TEST(test_event_when) {
    bool called = false;
    bool run = GENERATE(false, true);
    auto cond = [&](auto, auto) {
        called = true;
        return run;
    };

    auto e = event<E>.when(cond);
    static_assert(Event<decltype(e)>);
    TEST_ASSERT_EQUAL(run, e.match(S{}, E{}));
    TEST_ASSERT_TRUE(called);
}

TEST(test_event_when_chain) {
    int called = 0;
    auto cond = [&](auto, auto) { return called++ == 0; };

    auto e = event<E>.when(cond).when(cond);
    static_assert(Event<decltype(e)>);
    TEST_ASSERT_FALSE(e.match(S{}, E{}));
    TEST_ASSERT_EQUAL(2, called);
}

TEST(test_transition_make_valid) {
    auto t = state<S>.on(event<E>);
    static_assert(Transition<decltype(t)>);
    static_assert(stdlike::same_as<void, typename decltype(t)::Dst>);
}

TEST(test_transition_make_try_execute) {
    bool run = GENERATE(false, true);
    auto cond = [&](auto, auto) { return run; };
    auto t = state<S>.on(event<E>.when(cond));
    TEST_ASSERT_EQUAL(run, t.tryExecute(E{}));
}

TEST(test_transition_to_valid) {
    auto t = state<S>.on(event<E>).to(state<S>);
    static_assert(Transition<decltype(t)>);
}

TEST(test_transition_to_correct_dst) {
    auto t = state<S>.on(event<E>).to(state<S>);
    static_assert(Transition<decltype(t)>);
    static_assert(stdlike::same_as<decltype(state<S>), const typename decltype(t)::Dst>);
}

TEST(test_transition_run_valid) {
    auto t = state<S>.on(event<E>).run(ConcreteAction{});
    static_assert(Transition<decltype(t)>);
}

TEST(test_transition_run_runs) {
    int called = 0;
    auto cb = [&](auto, auto) { ++called; };
    auto t = state<S>.on(event<E>).run(cb);
    t.tryExecute(E{});
    TEST_ASSERT_EQUAL(1, called);
}

TEST(test_transition_run_chain_runs) {
    int called = 0;
    auto cb = [&](auto, auto) { ++called; };
    auto t = state<S>.on(event<E>).run(cb).run(cb).run(cb);
    t.tryExecute(E{});
    TEST_ASSERT_EQUAL(3, called);
}

TEST(test_state_machine) {
    struct SM {
        using InitialId = S;

        auto transitions() {
            return stdlike::tuple(
                state<S>.on(event<E>).to(state<int>),
                state<S>.on(event<E>).run(ConcreteAction{}).to(state<int>),
                state<S>.on(event<E>.when(ConcreteCondition<E>{})).to(state<int>));
        }
    };

    static_assert(StateMachine<SM>);
}

}  // namespace sml

TESTS_MAIN
