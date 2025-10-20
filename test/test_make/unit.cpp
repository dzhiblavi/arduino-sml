#include <sml/make.h>
#include <sml/overloads.h>

#include <utest/utest.h>

namespace sml {

TEST(test_src_state) {
    auto s1 = src<>;
    auto s2 = src<int>;
    auto s3 = src<int, float>;
    static_assert(SrcState<decltype(s1)>);
    static_assert(SrcState<decltype(s2)>);
    static_assert(SrcState<decltype(s3)>);
    static_assert(SrcState<decltype(exit<conc::StateMachine>)>);
}

TEST(test_dst_state) {
    auto d1 = dst<int>;
    static_assert(DstState<decltype(d1)>);
    static_assert(DstState<decltype(bypass)>);
    static_assert(DstState<decltype(x)>);
    static_assert(DstState<decltype(enter<conc::StateMachine>)>);
    static_assert(!DstState<decltype(exit<conc::StateMachine>)>);
}

TEST(test_event) {
    auto e1 = ev<>;
    auto e2 = ev<int>;
    auto e3 = ev<int, float>;
    static_assert(Event<decltype(e1)>);
    static_assert(Event<decltype(e2)>);
    static_assert(Event<decltype(e3)>);
    static_assert(Event<decltype(onEnter)>);
    static_assert(Event<decltype(onExit)>);
}

TEST(test_state_on_event) {
    auto t1 = src<> + ev<>;
    auto t2 = src<int> + ev<int>;
    auto t3 = src<int, float> + ev<int, float>;
    static_assert(Transition<decltype(t1)>);
    static_assert(Transition<decltype(t2)>);
    static_assert(Transition<decltype(t3)>);
}

TEST(test_transition_with_action) {
    int counter = 0;
    auto t = src<int> + ev<int> != [&](auto, auto) { ++counter; };
    static_assert(Transition<decltype(t)>);

    t(0, 0);
    t(0, 0);
    t(0, 0);
    TEST_ASSERT_EQUAL(3, counter);
}

TEST(test_transition_with_condition) {
    int counter = 0;
    auto t =
        src<int> + ev<int> == [&](auto, int e) { return e > 10; } != [&](auto...) { ++counter; };
    static_assert(Transition<decltype(t)>);

    t(0, 11);
    t(0, 0);
    t(0, 11);
    TEST_ASSERT_EQUAL(2, counter);
}

TEST(test_transition_with_chain) {
    int c1 = 0, c2 = 0, c3 = 0;
    auto t = src<int> + ev<int>                      //
             == [&](auto, int e) { return e > 10; }  //
             != [&](auto...) { ++c1; }               //
             == [&](auto, int e) { return e > 20; }  //
             != [&](auto...) { ++c2; }               //
             == [&](auto...) { return true; }        //
             != [&](auto...) { ++c3; };
    static_assert(Transition<decltype(t)>);

    t(0, 0);
    t(0, 11);
    t(0, 21);
    t(0, 31);
    TEST_ASSERT_EQUAL(3, c1);
    TEST_ASSERT_EQUAL(2, c2);
    TEST_ASSERT_EQUAL(2, c3);
}

}  // namespace sml

TESTS_MAIN
