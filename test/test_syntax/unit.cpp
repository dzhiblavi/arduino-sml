#include <sml/make.h>
#include <sml/syntax.h>

#include <utest/utest.h>

namespace sml {

TEST(test_condition_conjunction) {
    auto a = [](auto, int e) { return e > 10; };
    auto b = [](auto, int e) { return e > 20; };

    auto c = a && b;
    static_assert(Condition<decltype(c), int>);
    TEST_ASSERT_FALSE(c(0, 1));
    TEST_ASSERT_FALSE(c(0, 11));
    TEST_ASSERT_TRUE(c(0, 21));
}

TEST(test_condition_disjunction) {
    auto a = [](auto, int e) { return e > 10; };
    auto b = [](auto, int e) { return e > 20; };

    auto c = a || b;
    static_assert(Condition<decltype(c), int>);
    TEST_ASSERT_FALSE(c(0, 1));
    TEST_ASSERT_TRUE(c(0, 11));
    TEST_ASSERT_TRUE(c(0, 21));
}

TEST(test_condition_negation) {
    auto a = [](auto, int e) { return e > 10; };

    auto c = !a;
    static_assert(Condition<decltype(c), int>);
    TEST_ASSERT_TRUE(c(0, 1));
    TEST_ASSERT_FALSE(c(0, 11));
}

TEST(test_action_composition) {
    int count = 0;
    auto a = [&](auto, auto) { ++count; };

    auto c = (a, a);
    static_assert(Action<decltype(c)>);
    c(0, 0);
    TEST_ASSERT_EQUAL(2, count);
}

TEST(test_state_on_event) {
    auto t = state<int> + event<float>;
    static_assert(Transition<decltype(t)>);
    static_assert(stdlike::same_as<decltype(state<int>.on(event<float>)), decltype(t)>);
}

TEST(test_event_when_condition) {
    auto c = [](auto, int e) { return e > 10; };
    auto e = event<int>[c];
    static_assert(Event<decltype(e)>);
    static_assert(stdlike::same_as<decltype(event<int>.when(c)), decltype(e)>);
}

TEST(test_transition_with_condition) {
    auto c = [](auto, int e) { return e > 10; };
    auto t = state<int> + event<int>[c];
    static_assert(Transition<decltype(t)>);
    static_assert(stdlike::same_as<decltype(state<int>.on(event<int>.when(c))), decltype(t)>);
}

TEST(test_transition_with_action) {
    auto a = [](auto, auto) {};
    auto t = state<int> + event<int> | a;
    static_assert(Transition<decltype(t)>);
    static_assert(stdlike::same_as<decltype(state<int>.on(event<int>).run(a)), decltype(t)>);
}

TEST(test_transition_with_action_and_condition) {
    auto a = [](auto, auto) {};
    auto c = [](auto, int e) { return e > 10; };
    auto t = state<int> + event<int>[c] | a;
    static_assert(Transition<decltype(t)>);
    static_assert(
        stdlike::same_as<decltype(state<int>.on(event<int>.when(c)).run(a)), decltype(t)>);
}

TEST(test_transition_with_action_and_condition_and_dst) {
    auto a = [](auto, auto) {};
    auto c = [](auto, int e) { return e > 10; };
    auto t = state<int> + event<int>[c] | a = state<char>;
    static_assert(Transition<decltype(t)>);
    static_assert(stdlike::same_as<
                  decltype(state<int>.on(event<int>.when(c)).run(a).to(state<char>)),
                  decltype(t)>);
}

}  // namespace sml

TESTS_MAIN
