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

}  // namespace sml

TESTS_MAIN
