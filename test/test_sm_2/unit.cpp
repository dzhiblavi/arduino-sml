#include <sml/make.h>
#include <sml/sm.h>
#include <sml/syntax.h>

#include <utest/utest.h>

namespace sml {

struct t_sm {
    struct E {};
    struct S1 {};
    struct S2 {};
    struct S3 {};

    Action auto count(int& c) {
        return [&](auto, auto) { ++c; };
    }

    Condition<int> auto iff(bool& go) {
        return [&](auto, auto) { return go; };
    }
};

TEST_F(t_sm, test_sm_begin_empty) {
    struct M {
        using InitialId = S1;  // NOLINT

        auto transitions() {
            return make();
        }
    };

    SM<M> sm;
    sm.begin();
}

TEST_F(t_sm, test_sm_begin_calls_onenter) {
    struct M {
        using InitialId = S1;  // NOLINT

        auto transitions() {
            return make(
                state<S1>.on(onEnter).run([&](auto, auto) { c1 = true; }),
                state<S2>.on(onEnter).run([&](auto, auto) { c2 = true; }));
        }

        bool& c1;
        bool& c2;
    };

    bool c1 = false, c2 = false;
    SM<M> sm{M{c1, c2}};

    TEST_ASSERT_FALSE(c1 || c2);
    sm.begin();
    TEST_ASSERT_TRUE(c1 && !c2);
}

TEST_F(t_sm, test_sm_no_transition) {
    struct M {
        using InitialId = S1;  // NOLINT

        auto transitions() {
            return make(
                state<S1>.on(event<int>).run([&](auto, auto) { c1 = true; }),
                state<S2>.on(event<float>).run([&](auto, auto) { c2 = true; }));
        }

        bool& c1;
        bool& c2;
    };

    bool c1 = false, c2 = false;
    SM<M> sm{M{c1, c2}};

    TEST_ASSERT_FALSE(c1 || c2);
    sm.begin();
    TEST_ASSERT_FALSE(c1 || c2);

    TEST_ASSERT_FALSE(sm.feed(1.0));
    TEST_ASSERT_FALSE(c1 || c2);
}

TEST_F(t_sm, test_sm_transition) {
    struct M {
        using InitialId = S1;  // NOLINT

        auto transitions() {
            return make(
                state<S1>.on(event<int>).run([&](auto, auto) { ++c1; }).to(state<S2>),
                state<S2>.on(event<float>).run([&](auto, auto) { ++c2; }).to(state<S1>));
        }

        int& c1;
        int& c2;
    };

    int c1 = 0, c2 = 0;
    SM<M> sm{M{c1, c2}};

    // sm.begin();

    // none
    TEST_ASSERT_FALSE(sm.feed(1.0));
    TEST_ASSERT_FALSE(c1 || c2);

    // S1 -> S2
    TEST_ASSERT_TRUE(sm.feed(10));
    TEST_ASSERT_EQUAL(1, c1);
    TEST_ASSERT_EQUAL(0, c2);

    // S2 -> S1
    TEST_ASSERT_TRUE(sm.feed(1.0f));
    TEST_ASSERT_EQUAL(1, c1);
    TEST_ASSERT_EQUAL(1, c2);

    // S1 -> S2
    TEST_ASSERT_TRUE(sm.feed(20));
    TEST_ASSERT_EQUAL(2, c1);
    TEST_ASSERT_EQUAL(1, c2);
}

TEST_F(t_sm, test_sm_transitions_order_matters) {
    struct M {
        using InitialId = S1;  // NOLINT

        auto transitions() {
            return make(
                state<S1>.on(event<int>).run([&](auto, auto) { ++c1; }).to(state<S2>),
                state<S1>.on(event<int>).run([&](auto, auto) { ++c2; }).to(state<S2>));
        }

        int& c1;
        int& c2;
    };

    int c1 = 0, c2 = 0;
    SM<M> sm{M{c1, c2}};

    TEST_ASSERT_TRUE(sm.feed(10));
    TEST_ASSERT_EQUAL(1, c1);
    TEST_ASSERT_EQUAL(0, c2);
}

TEST_F(t_sm, test_sm_submachine) {
    struct S {
        using InitialId = S2;  // NOLINT

        auto transitions() {
            return make(state<S2>.on(event<float>).run([&](auto, auto) { ++c1; }).to(x));
        }

        int& c1;
    };

    struct M {
        using InitialId = S1;  // NOLINT

        auto transitions() {
            return make(
                state<S1>.on(event<int>).run([&](auto, auto) { ++c1; }).to(enter<S>),
                exit<S>.on(event<char>).run([&](auto, auto) { ++c2; }).to(state<S1>));
        }

        int& c1;
        int& c2;
    };

    int m1{}, m2{}, s1{};
    SM<M> sm{M{m1, m2}, S{s1}};

    // M: none
    TEST_ASSERT_FALSE(sm.feed(1.0));
    TEST_ASSERT_FALSE(sm.feed('a'));

    // M: S1 -> S (S2)
    TEST_ASSERT_TRUE(sm.feed(10));
    TEST_ASSERT_EQUAL(1, m1);
    TEST_ASSERT_EQUAL(0, m2);
    TEST_ASSERT_EQUAL(0, s1);

    // S: none
    TEST_ASSERT_FALSE(sm.feed(10));

    // S: S2 -> exit<S>
    TEST_ASSERT_TRUE(sm.feed(1.f));
    TEST_ASSERT_EQUAL(1, m1);
    TEST_ASSERT_EQUAL(0, m2);
    TEST_ASSERT_EQUAL(1, s1);

    // M: none
    TEST_ASSERT_FALSE(sm.feed(10));

    // M: exit<S> -> S1
    TEST_ASSERT_TRUE(sm.feed('a'));
    TEST_ASSERT_EQUAL(1, m1);
    TEST_ASSERT_EQUAL(1, m2);
    TEST_ASSERT_EQUAL(1, s1);
}

TEST_F(t_sm, test_sm_more_transitions) {
    struct S {
        using InitialId = S2;  // NOLINT

        auto transitions() {
            return make(
                state<S1>.on(event<int>).run([&](auto, auto) { ++c1; }).to(x),
                state<S2>.on(event<float>).run([&](auto, auto) { ++c2; }).to(state<S1>));
        }

        int& c1;
        int& c2;
    };

    struct M {
        using InitialId = S1;  // NOLINT

        auto transitions() {
            return make(
                state<S1>.on(event<int>).run([&](auto, auto) { ++c1; }).to(state<S2>),
                state<S2>.on(event<float>).run([&](auto, auto) { ++c2; }).to(enter<S>),
                exit<S>.on(event<char>).run([&](auto, auto) { ++c3; }).to(state<S1>));
        }

        int& c1;
        int& c2;
        int& c3;
    };

    int m1{}, m2{}, m3{}, s1{}, s2{};
    SM<M> sm{M{m1, m2, m3}, S{s1, s2}};

    TEST_ASSERT_TRUE(sm.feed(10));
    TEST_ASSERT_EQUAL(1, m1);
    TEST_ASSERT_EQUAL(0, m2);
    TEST_ASSERT_EQUAL(0, m3);
    TEST_ASSERT_EQUAL(0, s1);
    TEST_ASSERT_EQUAL(0, s2);

    TEST_ASSERT_TRUE(sm.feed(1.f));
    TEST_ASSERT_EQUAL(1, m1);
    TEST_ASSERT_EQUAL(1, m2);
    TEST_ASSERT_EQUAL(0, m3);
    TEST_ASSERT_EQUAL(0, s1);
    TEST_ASSERT_EQUAL(0, s2);

    TEST_ASSERT_TRUE(sm.feed(1.f));
    TEST_ASSERT_EQUAL(1, m1);
    TEST_ASSERT_EQUAL(1, m2);
    TEST_ASSERT_EQUAL(0, m3);
    TEST_ASSERT_EQUAL(0, s1);
    TEST_ASSERT_EQUAL(1, s2);

    TEST_ASSERT_TRUE(sm.feed(10));
    TEST_ASSERT_EQUAL(1, m1);
    TEST_ASSERT_EQUAL(1, m2);
    TEST_ASSERT_EQUAL(0, m3);
    TEST_ASSERT_EQUAL(1, s1);
    TEST_ASSERT_EQUAL(1, s2);

    TEST_ASSERT_TRUE(sm.feed('a'));
    TEST_ASSERT_EQUAL(1, m1);
    TEST_ASSERT_EQUAL(1, m2);
    TEST_ASSERT_EQUAL(1, m3);
    TEST_ASSERT_EQUAL(1, s1);
    TEST_ASSERT_EQUAL(1, s2);
}

}  // namespace sml

TESTS_MAIN
