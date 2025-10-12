#include <sml/state_machine.h>

#include <log.h>

#include <utest/utest.h>

enum State {
    Off,
    Blinking,
    Constant,
    CountSentinel,
};

struct Ev1 {
    bool allow;
};

struct Ev2 {
    bool allow;
};

using namespace sm::literals;

auto foo(int x) {
    LINFO("Construct foo %d", x);
    return [x](auto /*x*/) { LINFO("Call foo %d", x); };
}

struct Inner {
    auto operator()() {
        return sm::makeTransitionTable( //
            *"i1"_s + sm::Event<int>{} / foo(10) = sm::X);
    }
};

struct Outer {
    auto operator()() {
        return sm::makeTransitionTable( //
            *"o1"_s + sm::event<int> = sm::state<Inner>,
            "o2"_s + sm::event<float> = sm::state<Inner>,
            sm::X + sm::onEnter = "o2"_s);
    }
};

static_assert(stdlike::same_as<typename sm::traits::InitialState<Outer>::type, decltype("o1"_s)>);

using Table = sm::traits::TableTypeOf<Outer>;
using Ts = sm::traits::TransitionsOf<Outer>;
static_assert(stdlike::same_as<tl::ToList<Table>, Ts>);

using Submachines = typename sm::traits::SubmachinesOf<Outer>::type;
static_assert(stdlike::same_as<Submachines, tl::List<Inner>>);

using All = typename sm::traits::AllTransitions<Outer>::type;
using Replaced = typename sm::ReplaceOutgoingTransitions<All>::type;
using Unfolded = sm::UnfoldedMachine<Outer>;
static_assert(stdlike::same_as<sm::traits::TransitionsOf<Unfolded>, Replaced>);

TEST(test_state_machine) {
    sm::SM<Outer> machine;
    machine.begin();

    // TEST_ASSERT_TRUE(machine.is("o1"_s));

    TEST_ASSERT_TRUE(machine.feed(1));
    // TEST_ASSERT_TRUE(machine.is("i1"_s));

    TEST_ASSERT_FALSE(machine.feed(1.f));

    TEST_ASSERT_TRUE(machine.feed(2));
    // TEST_ASSERT_TRUE(machine.is("o2"_s));
}

TESTS_MAIN
