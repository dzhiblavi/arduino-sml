#include <sml/impl/dispatcher.h>
#include <sml/impl/traits.h>
#include <sml/make.h>

#include <utest/utest.h>

namespace sml {

auto count(int& c) {
    return [&](auto, auto) { ++c; };
}

auto iff(bool& go) {
    return [&](auto, auto) { return go; };
}

auto never() {
    return [](auto...) { return false; };
}

TEST(test_dispatcher_respects_source_state_tags) {
    static int c1, c2;

    struct S1 {
        using InitialId = int;

        auto transitions() {
            return table(
                src<int> + ev<int> != count(c1) = bypass  //
            );
        }
    };

    struct S2 {
        using InitialId = int;

        auto transitions() {
            return table(
                src<int> + ev<float> = enter<S1>,         // for S1 inclusion only
                src<int> + ev<int> != count(c2) = bypass  //
            );
        }
    };

    using M = impl::traits::CombinedStateMachine<S2>;
    using Ts = impl::traits::Transitions<M>;
    using Disp = impl::Dispatcher<int, Ts>;
    // S2, S1

    c1 = c2 = 0;
    M m;
    auto trs = m.transitions();
    Disp d{&trs};

    SECTION("chooses submachine's transition") {
        TEST_ASSERT_EQUAL(1, d.dispatch(1, 10));
        TEST_ASSERT_EQUAL(1, c1);
        TEST_ASSERT_EQUAL(0, c2);
    }

    SECTION("chooses outer machine's transition") {
        TEST_ASSERT_EQUAL(0, d.dispatch(0, 10));
        TEST_ASSERT_EQUAL(0, c1);
        TEST_ASSERT_EQUAL(1, c2);
    }
}

TEST(test_dispatcher_wildcard_events_only_local) {
    static int c1, c2;

    struct S1 {
        using InitialId = int;

        auto transitions() {
            return table(
                src<int> + ev<char, int> != count(c1) = bypass  //
            );
        }
    };

    struct S2 {
        using InitialId = int;

        auto transitions() {
            return table(
                src<int> + ev<> != count(c2),  // only float should be respected
                src<int> + ev<float> = enter<S1>,
                exit<S1> + onEnter  //
            );
        }
    };

    using M = impl::traits::CombinedStateMachine<S2>;
    using Ts = impl::traits::Transitions<M>;
    // int(S2), term(S1), int(S1)

    c1 = c2 = 0;
    M m;
    auto trs = m.transitions();

    SECTION("matches outer machine's wildcard event") {
        using Disp = impl::Dispatcher<float, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(0, d.dispatch(0, 1.f));
        TEST_ASSERT_EQUAL(0, c1);
        TEST_ASSERT_EQUAL(1, c2);
    }

    SECTION("submachine's event ids are not included") {
        using Disp = impl::Dispatcher<char, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(-1, d.dispatch(0, 'a'));
        TEST_ASSERT_EQUAL(0, c1);
        TEST_ASSERT_EQUAL(0, c2);
    }

    SECTION("exit<> source state does not introduce an event to wildcard") {
        using Disp = impl::Dispatcher<OnExitEventId, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(-1, d.dispatch(0, OnExitEventId{}));
        TEST_ASSERT_EQUAL(0, c1);
        TEST_ASSERT_EQUAL(0, c2);
    }
}

}  // namespace sml

TESTS_MAIN
