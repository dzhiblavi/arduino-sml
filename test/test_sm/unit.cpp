#include <sml/make.h>
#include <sml/sm.h>
#include <sml/syntax.h>

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

TEST(test_sm_begin_empty_does_nothing) {
    struct M {
        using InitialId = int;  // NOLINT

        auto transitions() {
            return table();
        }
    };

    SM<M> sm;
    sm.begin();
}

TEST(test_sm_begin_feeds_onenter_on_begin) {
    static int c1, c2;

    struct M {
        using InitialId = int;  // NOLINT

        auto transitions() {
            return table(
                src<float> + onEnter != count(c2) = bypass,  // non-initial state
                src<int> + onEnter != count(c1) = bypass     //
            );
        }
    };

    c1 = c2 = 0;
    SM<M> sm;

    TEST_ASSERT_EQUAL(0, c1);
    TEST_ASSERT_EQUAL(0, c2);
    sm.begin();
    TEST_ASSERT_EQUAL(1, c1);
    TEST_ASSERT_EQUAL(0, c2);
}

TEST(test_sm_no_transition) {
    static int c;

    struct M {
        using InitialId = int;  // NOLINT

        auto transitions() {
            return table(src<int> + ev<int> != count(c) == never());
        }
    };

    c = 0;
    SM<M> sm;

    SECTION("unknown event") {
        TEST_ASSERT_FALSE(sm.feed(1.f));
        TEST_ASSERT_EQUAL(0, c);
    }

    SECTION("blocked by condition") {
        TEST_ASSERT_FALSE(sm.feed(10));
        TEST_ASSERT_EQUAL(1, c);
    }
}

TEST(test_sm_transition) {
    static int c;

    struct M {
        using InitialId = int;  // NOLINT

        auto transitions() {
            return table(
                src<int> + ev<int> != count(c) = bypass,
                src<char> + ev<> != count(c) = bypass,
                src<int> + ev<float> != count(c) = bypass,
                src<int> + ev<> != count(c)  //
            );
        }
    };

    c = 0;
    SM<M> sm;
    TEST_ASSERT_TRUE(sm.feed(10));
    TEST_ASSERT_EQUAL(2, c);
}

TEST(test_sm_feeds_enter_and_exit_events) {
    static int c;
    static bool go;

    struct M {
        using InitialId = int;  // NOLINT

        auto transitions() {
            return table(
                src<int> + ev<int> == iff(go) != count(c) = dst<float>,
                src<int> + onExit != count(c),
                src<float> + onEnter != count(c)  //
            );
        }
    };

    c = 0;
    SM<M> sm;

    SECTION("feeds onEnter and onExit events when transition happens") {
        go = true;
        TEST_ASSERT_TRUE(sm.feed(10));
        TEST_ASSERT_EQUAL(3, c);
    }

    SECTION("does not feed onEnter and onExit events when no transition happens") {
        go = false;
        TEST_ASSERT_FALSE(sm.feed(10));
        TEST_ASSERT_EQUAL(0, c);
    }
}

TEST(test_sm_saves_new_state_if_needed) {
    static int c;
    static bool go;

    struct M {
        using InitialId = int;  // NOLINT

        auto transitions() {
            return table(
                src<int> + ev<int> == iff(go) = dst<float>,
                src<int> + ev<char> != count(c),
                src<float> + ev<float> != count(c)  //
            );
        }
    };

    c = 0;
    SM<M> sm;

    SECTION("saves new state when transition happens") {
        go = true;
        TEST_ASSERT_TRUE(sm.feed(10));
        TEST_ASSERT_FALSE(sm.feed('a'));
        TEST_ASSERT_TRUE(sm.feed(1.f));
        TEST_ASSERT_EQUAL(1, c);
    }

    SECTION("does not change state when no transition") {
        go = false;
        TEST_ASSERT_FALSE(sm.feed(10));
        TEST_ASSERT_FALSE(sm.feed(1.f));
        TEST_ASSERT_TRUE(sm.feed('a'));
        TEST_ASSERT_EQUAL(1, c);
    }
}

}  // namespace sml

TESTS_MAIN
