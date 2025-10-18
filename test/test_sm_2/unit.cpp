#include <sml/make.h>
#include <sml/overloads.h>
#include <sml/sm.h>
#include <sml/syntax.h>

#include <utest/utest.h>

namespace sml {

struct t_sm {
    struct E {};
    struct S1 {};
    struct S2 {};
    struct S3 {};

    auto count(int& c) {
        return [&](auto, auto) { ++c; };
    }

    auto iff(bool& go) {
        return [&](auto, auto) { return go; };
    }
};

TEST_F(t_sm, test_sm_bypass_before) {
    struct M {
        using InitialId = S1;  // NOLINT

        auto transitions() {
            return table(
                src<> + ev<int> != [&](auto, auto) { ++c1; } = bypass,
                src<S1> + ev<int> != [&](auto, auto) { ++c2; });
        }

        int& c1;
        int& c2;
    };

    int c1{}, c2{};
    SM<M> sm{M{c1, c2}};

    TEST_ASSERT_TRUE(sm.feed(10));
    TEST_ASSERT_EQUAL(1, c1);
    TEST_ASSERT_EQUAL(1, c2);
}

TEST_F(t_sm, test_sm_bypass_after) {
    struct M {
        using InitialId = S1;  // NOLINT

        auto transitions() {
            return table(
                src<S1> + ev<int> != [&](auto, auto) { ++c2; },
                src<> + ev<int> != [&](auto, auto) { ++c1; } = bypass);
        }

        int& c1;
        int& c2;
    };

    int c1{}, c2{};
    SM<M> sm{M{c1, c2}};

    TEST_ASSERT_TRUE(sm.feed(10));
    TEST_ASSERT_EQUAL(0, c1);
    TEST_ASSERT_EQUAL(1, c2);
}

TEST_F(t_sm, test_sm_example_with_syntax) {
    struct S {
        using InitialId = S2;  // NOLINT

        auto transitions() {
            auto inc = [](int& x) { return [&x](auto, auto) { ++x; }; };
            auto cond = [](auto, char c) { return c == 'x'; };
            auto log = [](auto, int x) { LINFO("S transition event=%d", x); };

            return table(
                src<> + ev<int> != log = bypass,  // logging bypass transition
                src<S1> + ev<int> != inc(c1) = x,
                src<S2> + ev<float> != inc(c2) = dst<S1>,
                src<S1, S2> + ev<char> == cond != inc(c3) = dst<S2>,
                src<S2> + ev<char> != inc(c4) = dst<S1>);
        }

        int& c1;
        int& c2;
        int& c3;
        int& c4;
    };

    struct M {
        using InitialId = S1;  // NOLINT

        auto transitions() {
            auto inc = [](int& x) { return [&x](auto, auto) { ++x; }; };
            auto cond = [](auto, char c) { return c == 'y'; };
            auto log = overloads{
                [](auto, int x) { LINFO("M transition event int=%d", x); },
                [](auto, float x) { LINFO("M transition event float=%f", x); },
                [](auto, char x) { LINFO("M transition event char=%d", (int)x); },
                [](auto, auto) { LINFO("M anon transition"); },
                [](auto, OnEnterEventId) { LINFO("M onEnter transition"); },
                [](auto, OnExitEventId) { LINFO("M onExit transition"); },
            };

            return table(
                src<> + ev<> != log = bypass,  // logging bypass transition
                src<S1> + ev<int> != inc(c1) = dst<S2>,
                src<S2> + ev<float> != inc(c2) = enter<S>,
                exit<S> + onEnter != inc(c3) = dst<S1>,
                src<> + ev<char> == cond != inc(c4) = dst<S1>);
        }

        int& c1;
        int& c2;
        int& c3;
        int& c4;
    };

    int s1{}, s2{}, s3{}, s4{};
    int m1{}, m2{}, m3{}, m4{};
    SM<M> sm{M{m1, m2, m3, m4}, S{s1, s2, s3, s4}};

    // M: S1
    TEST_ASSERT_FALSE(sm.feed('z'));
    TEST_ASSERT_TRUE(sm.feed('y'));  // S1 -> S1, c4
    TEST_ASSERT_EQUAL(1, m4);
    TEST_ASSERT_TRUE(sm.feed(10));  // S1 -> S2, c1
    TEST_ASSERT_EQUAL(1, m1);
    TEST_ASSERT_FALSE(sm.feed('z'));
    TEST_ASSERT_TRUE(sm.feed('y'));  // S2 -> S1, c4
    TEST_ASSERT_EQUAL(2, m4);
    TEST_ASSERT_TRUE(sm.feed(20));  // S1 -> S2, c1
    TEST_ASSERT_EQUAL(2, m1);
    TEST_ASSERT_TRUE(sm.feed(1.f));  // S2 -> enter<S>, c2
    TEST_ASSERT_EQUAL(1, m2);

    // S: S2
    TEST_ASSERT_TRUE(sm.feed(1.f));  // S2 -> S1, c2
    TEST_ASSERT_EQUAL(1, s2);
    TEST_ASSERT_FALSE(sm.feed('z'));
    TEST_ASSERT_TRUE(sm.feed('x'));  // S1 -> S2, c3
    TEST_ASSERT_EQUAL(1, s3);
    TEST_ASSERT_TRUE(sm.feed('z'));  // S2 -> S1, c4
    TEST_ASSERT_EQUAL(1, s4);
    TEST_ASSERT_TRUE(sm.feed(30));  // S1 -> x, c1
    TEST_ASSERT_EQUAL(1, s1);

    // M: exit<S> -> S1, c3
    TEST_ASSERT_EQUAL(1, m3);
    TEST_ASSERT_TRUE(sm.feed('y'));  // S1 -> S1, c4
    TEST_ASSERT_EQUAL(3, m4);

    LINFO("SM size: %d", sizeof(SM<M>));
}

}  // namespace sml

TESTS_MAIN
