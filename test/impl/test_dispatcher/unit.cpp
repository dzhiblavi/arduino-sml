#include <sml/impl/dispatcher.h>
#include <sml/impl/traits.h>
#include <sml/make.h>

#include <utest/utest.h>

namespace sml{

Action auto count(int& c) {
    return [&](auto, auto) { ++c; };
}

Condition<int> auto iff(bool& go) {
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
    using Ss = tl::List<>;
    using Disp = impl::Dispatcher<E, Ts, Ss>;

    auto trs = stdlike::tuple();
    Disp d{trs};

    // Undefined:
    // TEST_ASSERT_EQUAL(-1, d.dispatch(0, E{}));
}

TEST_F(t_dispatcher, test_dispatcher_runs) {
    struct S {
        using InitialId = S1;
        auto transitions() {
            return make(sml::state<S1>.on(sml::event<E>).run(count(cnt)));
        }
        int& cnt;
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    using Ss = impl::traits::StateUIds<impl::RefUId<S, S1>, Ts>;
    using Disp = impl::Dispatcher<E, Ts, Ss>;

    int cnt = 0;
    M m(S{cnt});
    auto trs = m.transitions();
    Disp d{trs};

    TEST_ASSERT_EQUAL(0, d.dispatch(0, E{}));
    TEST_ASSERT_EQUAL(1, cnt);

    TEST_ASSERT_EQUAL(0, d.dispatch(0, E{}));
    TEST_ASSERT_EQUAL(2, cnt);
}

TEST_F(t_dispatcher, test_dispatcher_destination) {
    struct S {
        using InitialId = S1;
        auto transitions() {
            return make(state<S1>.on(event<E>).to(state<S2>));
        }
        int& cnt;
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    using Ss = impl::traits::StateUIds<impl::RefUId<S, S1>, Ts>;
    using Disp = impl::Dispatcher<E, Ts, Ss>;

    int cnt = 0;
    M m(S{cnt});
    auto trs = m.transitions();
    Disp d{trs};

    // S1, S2
    TEST_ASSERT_EQUAL(-1, d.dispatch(1, E{}));
    TEST_ASSERT_EQUAL(1, d.dispatch(0, E{}));
    TEST_ASSERT_EQUAL(-1, d.dispatch(1, E{}));
}

TEST_F(t_dispatcher, test_dispatcher_not_matched) {
    struct S {
        using InitialId = S1;
        auto transitions() {
            return make(state<S1>.on(event<E>.when(iff(go))));
        }
        bool& go;
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    using Ss = impl::traits::StateUIds<impl::RefUId<S, S1>, Ts>;
    using Disp = impl::Dispatcher<E, Ts, Ss>;

    bool go = false;
    M m(S{go});
    auto trs = m.transitions();
    Disp d{trs};

    go = false;
    TEST_ASSERT_EQUAL(-1, d.dispatch(0, E{}));

    go = true;
    TEST_ASSERT_EQUAL(0, d.dispatch(1, E{}));
}

TEST_F(t_dispatcher, test_dispatcher_multiple) {
    struct S {
        using InitialId = S1;
        auto transitions() {
            return make(
                state<S1>.on(event<E>.when(iff(go1))).to(state<S2>).run(count(c1)),
                state<S1>.on(event<E>.when(iff(go2))).to(state<S3>).run(count(c2)));
        }

        bool &go1, &go2;
        int &c1, &c2;
    };

    using M = impl::traits::CombinedStateMachine<S>;
    using Ts = impl::traits::Transitions<M>;
    using Ss = impl::traits::StateUIds<impl::RefUId<S, S1>, Ts>;
    using Disp = impl::Dispatcher<E, Ts, Ss>;

    bool go1 = false, go2 = false;
    int c1 = 0, c2 = 0;

    M m(S{go1, go2, c1, c2});
    auto trs = m.transitions();
    Disp d{trs};

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

}  // namespace sml::impl

TESTS_MAIN
