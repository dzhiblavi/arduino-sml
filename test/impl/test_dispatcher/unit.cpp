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

struct t_dispatcher {
    struct E {};
    struct S1 {};
    struct S2 {};
    struct S3 {};
};

TEST_F(t_dispatcher, test_dispatcher_empty) {
    using Ts = tl::List<>;
    using Disp = impl::Dispatcher<E, Ts>;

    auto trs = std::tuple();
    Disp d{&trs};

    // Undefined:
    // TEST_ASSERT_EQUAL(-1, d.dispatch(0, E{}));
}

TEST_F(t_dispatcher, test_dispatcher_runs) {
    struct S {
        using InitialId = S1;
        auto transitions() {
            return table(src<S1>.on(ev<E>).run(count(cnt)));
        }
        int& cnt;
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    using Disp = impl::Dispatcher<E, Ts>;

    int cnt = 0;
    M m(S{cnt});
    auto trs = m.transitions();
    Disp d{&trs};

    TEST_ASSERT_EQUAL(0, d.dispatch(0, E{}));
    TEST_ASSERT_EQUAL(1, cnt);

    TEST_ASSERT_EQUAL(0, d.dispatch(0, E{}));
    TEST_ASSERT_EQUAL(2, cnt);
}

TEST_F(t_dispatcher, test_dispatcher_destination) {
    struct S {
        using InitialId = S1;
        auto transitions() {
            return table(src<S1>.on(ev<E>).to(dst<S2>));
        }
        int& cnt;
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    using Disp = impl::Dispatcher<E, Ts>;

    int cnt = 0;
    M m(S{cnt});
    auto trs = m.transitions();
    Disp d{&trs};

    // S1, S2
    TEST_ASSERT_EQUAL(-1, d.dispatch(1, E{}));
    TEST_ASSERT_EQUAL(1, d.dispatch(0, E{}));
    TEST_ASSERT_EQUAL(-1, d.dispatch(1, E{}));
}

TEST_F(t_dispatcher, test_dispatcher_not_matched) {
    struct S {
        using InitialId = S1;
        auto transitions() {
            return table(src<S1>.on(ev<E>).when(iff(go)));
        }
        bool& go;
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    using Disp = impl::Dispatcher<E, Ts>;

    bool go = false;
    M m(S{go});
    auto trs = m.transitions();
    Disp d{&trs};

    go = false;
    TEST_ASSERT_EQUAL(-1, d.dispatch(0, E{}));

    go = true;
    TEST_ASSERT_EQUAL(0, d.dispatch(1, E{}));
}

TEST_F(t_dispatcher, test_dispatcher_multiple) {
    struct S {
        using InitialId = S1;
        auto transitions() {
            return table(
                src<S1>.on(ev<E>).when(iff(go1)).to(dst<S2>).run(count(c1)),
                src<S1>.on(ev<E>).when(iff(go2)).to(dst<S3>).run(count(c2)));
        }

        bool &go1, &go2;
        int &c1, &c2;
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    using Disp = impl::Dispatcher<E, Ts>;

    bool go1 = false, go2 = false;
    int c1 = 0, c2 = 0;

    M m(S{go1, go2, c1, c2});
    auto trs = m.transitions();
    Disp d{&trs};

    // S2, S1, S3
    go1 = go2 = true;
    TEST_ASSERT_EQUAL(0, d.dispatch(1, E{}));  // t1
    TEST_ASSERT_EQUAL(1, c1);
    TEST_ASSERT_EQUAL(0, c2);

    go1 = false;
    TEST_ASSERT_EQUAL(2, d.dispatch(1, E{}));  // t2
    TEST_ASSERT_EQUAL(1, c1);
    TEST_ASSERT_EQUAL(1, c2);

    go2 = false;
    TEST_ASSERT_EQUAL(-1, d.dispatch(0, E{}));  // neither
    TEST_ASSERT_EQUAL(1, c1);
    TEST_ASSERT_EQUAL(1, c2);
}

TEST_F(t_dispatcher, test_dispatcher_bypass) {
    struct S {
        using InitialId = S1;
        auto transitions() {
            return table(
                src<S1>.on(ev<E>).run(count(c1)).to(bypass), src<S1>.on(ev<E>).run(count(c2)));
        }

        int &c1, &c2;
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    using Disp = impl::Dispatcher<E, Ts>;

    int c1 = 0, c2 = 0;

    M m(S{c1, c2});
    auto trs = m.transitions();
    Disp d{&trs};

    TEST_ASSERT_EQUAL(0, d.dispatch(0, E{}));
    TEST_ASSERT_EQUAL(1, c1);
    TEST_ASSERT_EQUAL(1, c2);
}

TEST_F(t_dispatcher, test_dispatcher_keep) {
    struct S {
        using InitialId = S1;
        auto transitions() {
            return table(
                src<S1>.on(ev<E>).run(count(c1)), src<S1>.on(ev<E>).run(count(c2)).to(dst<int>));
        }

        int &c1, &c2;
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    using Disp = impl::Dispatcher<E, Ts>;

    int c1 = 0, c2 = 0;

    M m(S{c1, c2});
    auto trs = m.transitions();
    Disp d{&trs};

    TEST_ASSERT_EQUAL(0, d.dispatch(0, E{}));
    TEST_ASSERT_EQUAL(1, c1);
    TEST_ASSERT_EQUAL(0, c2);
}

TEST_F(t_dispatcher, test_dispatcher_wildcard_events_separated) {
    struct M1 {
        using InitialId = S1;

        auto transitions() {
            return table(
                src<S1> + ev<int> != count(c1) = bypass,
                src<S1> + ev<> != count(c2) = dst<S1>  // only int
            );
        }

        int &c1, &c2;
    };

    struct M2 {
        using InitialId = S1;

        auto transitions() {
            return table(
                src<S1> + ev<float> != count(c1) = enter<M1>,
                src<S1> + ev<> != count(c2) = dst<S1>  // only char, float
            );
        }

        int &c1, &c2;
    };

    using M = impl::traits::CombinedStateMachine<M2>;
    using Ts = impl::traits::Transitions<M>;
    // states: s1/m2, s1/m1

    int m11 = 0, m12 = 0, m21 = 0, m22 = 0;

    M m(M2{m21, m22}, M1{m11, m12});
    auto trs = m.transitions();

    {
        m11 = 0, m12 = 0, m21 = 0, m22 = 0;
        using Disp = impl::Dispatcher<int, Ts>;
        Disp d{&trs};
        TEST_ASSERT_EQUAL(-1, d.dispatch(0, 10));
        TEST_ASSERT_EQUAL(1, d.dispatch(1, 10));
        TEST_ASSERT_EQUAL(1, m11);
        TEST_ASSERT_EQUAL(1, m12);
        TEST_ASSERT_EQUAL(0, m21);
        TEST_ASSERT_EQUAL(0, m22);
    }

    {
        m11 = 0, m12 = 0, m21 = 0, m22 = 0;
        using Disp = impl::Dispatcher<float, Ts>;
        Disp d{&trs};
        TEST_ASSERT_EQUAL(1, d.dispatch(0, 1.f));
        TEST_ASSERT_EQUAL(0, m11);
        TEST_ASSERT_EQUAL(0, m12);
        TEST_ASSERT_EQUAL(1, m21);
        TEST_ASSERT_EQUAL(0, m22);
    }
}

}  // namespace sml

TESTS_MAIN
