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

TEST(test_dispatcher_empty) {
    using Ts = tl::List<>;
    using Disp = impl::Dispatcher<int, Ts>;

    auto trs = std::tuple();
    Disp d{&trs};

    // Undefined:
    // TEST_ASSERT_EQUAL(-1, d.dispatch(0, E{}));
}

TEST(test_dispatcher_no_transition_keeps_state) {
    static int c;

    struct S {
        using InitialId = int;

        auto transitions() {
            return table(
                src<char> + ev<char> != count(c) = dst<float>,
                src<int> + ev<float> == never() != count(c)  //
            );
        }
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    // char=0, int=1

    c = 0;
    M m;
    auto trs = m.transitions();

    SECTION("source state does not match") {
        using Disp = impl::Dispatcher<char, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(-1, d.dispatch(1, 'a'));
        TEST_ASSERT_EQUAL(0, c);
    }

    SECTION("no transition matches event") {
        using Disp = impl::Dispatcher<int, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(-1, d.dispatch(1, 10));
        TEST_ASSERT_EQUAL(0, c);
    }

    SECTION("blocked by transition condition") {
        using Disp = impl::Dispatcher<float, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(-1, d.dispatch(1, 1.f));
        TEST_ASSERT_EQUAL(0, c);
    }
}

TEST(test_dispatcher_transition_correct_dst_state) {
    static int c;

    struct S {
        using InitialId = int;

        auto transitions() {
            return table(
                src<char> + ev<char> != count(c),                // keep state
                src<int> + ev<int> != count(c) = dst<char>,      // change state
                src<float> + ev<float> != count(c) = dst<float>  // keep state explicitly
            );
        }
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    // int=0, char=1, float=2

    c = 0;
    M m;
    auto trs = m.transitions();

    SECTION("no transition destination specified") {
        using Disp = impl::Dispatcher<char, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(1, d.dispatch(1, 'a'));
        TEST_ASSERT_EQUAL(1, c);
    }

    SECTION("transition destination differs from source") {
        using Disp = impl::Dispatcher<int, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(1, d.dispatch(0, 'a'));
        TEST_ASSERT_EQUAL(1, c);
    }

    SECTION("transition destination is same as source") {
        using Disp = impl::Dispatcher<float, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(2, d.dispatch(2, 'a'));
        TEST_ASSERT_EQUAL(1, c);
    }
}

TEST(test_dispatcher_respects_transition_order) {
    static int c;
    static auto ifg = [](char c) { return [c](auto, char x) { return x >= c; }; };

    struct S {
        using InitialId = int;

        auto transitions() {
            return table(
                src<char> + ev<char> == ifg('b') != count(c) = dst<int>,   //
                src<char> + ev<char> == ifg('a') != count(c) = dst<float>  //
            );
        }
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    // int, char, float

    c = 0;
    M m;
    auto trs = m.transitions();

    SECTION("first transition matches") {
        using Disp = impl::Dispatcher<char, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(0, d.dispatch(1, 'b'));
        TEST_ASSERT_EQUAL(1, c);
    }

    SECTION("second transition matches") {
        using Disp = impl::Dispatcher<char, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(2, d.dispatch(1, 'a'));
        TEST_ASSERT_EQUAL(1, c);
    }
}

TEST(test_dispatcher_bypass) {
    static int c;

    struct S {
        using InitialId = int;

        auto transitions() {
            return table(
                src<char> + ev<char> != count(c) = bypass,     //
                src<char> + ev<char> != count(c) = dst<float>  //
            );
        }
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    // char, float

    c = 0;
    M m;
    auto trs = m.transitions();

    SECTION("executes transition and tests the next one") {
        using Disp = impl::Dispatcher<char, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(1, d.dispatch(0, 'b'));
        TEST_ASSERT_EQUAL(2, c);
    }
}

TEST(test_dispatcher_multi_event) {
    static int c;

    struct S {
        using InitialId = int;

        auto transitions() {
            return table(
                src<char> + ev<char, float> != count(c) = dst<int>,
                src<int> + ev<int> != count(c) = dst<char>);
        }
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    // int, char

    c = 0;
    M m;
    auto trs = m.transitions();

    SECTION("matches multi event transition") {
        using Disp = impl::Dispatcher<char, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(0, d.dispatch(1, 'b'));
        TEST_ASSERT_EQUAL(1, c);
    }

    SECTION("does not match, event exists") {
        using Disp = impl::Dispatcher<int, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(-1, d.dispatch(1, 10));
    }

    SECTION("event does not exist") {
        using Disp = impl::Dispatcher<float*, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(-1, d.dispatch(1, nullptr));
    }
}

TEST(test_dispatcher_wildcard_event) {
    static int c;

    struct S {
        using InitialId = int;

        auto transitions() {
            return table(
                src<char> + ev<> != count(c) = dst<int>,
                src<int> + ev<int> != count(c) = dst<char>);
        }
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    // int, char

    c = 0;
    M m;
    auto trs = m.transitions();

    SECTION("matches wildcard event transition") {
        using Disp = impl::Dispatcher<int, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(0, d.dispatch(1, 'b'));
        TEST_ASSERT_EQUAL(1, c);
    }

    SECTION("does not match an unknown event") {
        using Disp = impl::Dispatcher<float, Ts>;
        Disp d{&trs};

        TEST_ASSERT_EQUAL(-1, d.dispatch(1, 'b'));
        TEST_ASSERT_EQUAL(0, c);
    }
}

TEST(test_dispatcher_multi_source) {
    static int c;

    struct S {
        using InitialId = int;

        auto transitions() {
            return table(
                src<char, int> + ev<int> != count(c) = dst<float>,
                src<int> + ev<int> = dst<char>,
                src<char> + ev<int> = dst<char>  //
            );
        }
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    using Disp = impl::Dispatcher<int, Ts>;
    // float, int, char

    c = 0;
    M m;
    auto trs = m.transitions();
    Disp d{&trs};

    SECTION("matches multi source transition (1)") {
        TEST_ASSERT_EQUAL(0, d.dispatch(1, 10));
        TEST_ASSERT_EQUAL(1, c);
    }

    SECTION("matches multi source transition (2)") {
        TEST_ASSERT_EQUAL(0, d.dispatch(2, 10));
        TEST_ASSERT_EQUAL(1, c);
    }

    SECTION("does not match multi source transition") {
        TEST_ASSERT_EQUAL(-1, d.dispatch(0, 10));
        TEST_ASSERT_EQUAL(0, c);
    }
}

TEST(test_dispatcher_wildcard_source) {
    static int c;

    struct S {
        using InitialId = int;

        auto transitions() {
            return table(
                src<> + ev<int> != count(c) = dst<float>,
                src<int> + ev<int> = dst<char>,
                src<char> + ev<int> = dst<char>  //
            );
        }
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    using Disp = impl::Dispatcher<int, Ts>;
    // float, int, char

    c = 0;
    M m;
    auto trs = m.transitions();
    Disp d{&trs};

    SECTION("matches wildcard source transition (1)") {
        TEST_ASSERT_EQUAL(0, d.dispatch(1, 10));
        TEST_ASSERT_EQUAL(1, c);
    }

    SECTION("matches multi source transition (2)") {
        TEST_ASSERT_EQUAL(0, d.dispatch(2, 10));
        TEST_ASSERT_EQUAL(1, c);
    }

    SECTION("matches multi source transition (3)") {
        TEST_ASSERT_EQUAL(0, d.dispatch(0, 10));
        TEST_ASSERT_EQUAL(1, c);
    }
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
