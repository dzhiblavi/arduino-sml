#include <sml/model.h>
#include <sml/overloads.h>

#include <utest/utest.h>

namespace sml {

TEST(test_action_concept_concrete) {
    using A = decltype([](int, float) {});
    using B = decltype([](int, int*) {});
    static_assert(Action<A, tl::List<int>, tl::List<float>>);
    static_assert(!Action<B, tl::List<int>, tl::List<float>>);
}

TEST(test_action_concept_overloaded) {
    using A = decltype(overloads{
        [](int, float) {},
        [](int, int*) {},
        [](int*, float) {},
        [](int*, int*) {},
    });

    using B = decltype(overloads{
        [](int, float) {},
        [](int, int*) {},
        [](int*, float) {},
    });

    static_assert(Action<A, tl::List<int, int*>, tl::List<float, int*>>);
    static_assert(!Action<B, tl::List<int, int*>, tl::List<float, int*>>);
}

TEST(test_condition_concept_concrete) {
    using A = decltype([](int, float) { return false; });
    using B = decltype([](int, int*) { return false; });
    using C = decltype([](int, float) { return nullptr; });
    static_assert(Condition<A, tl::List<int>, tl::List<float>>);
    static_assert(!Condition<B, tl::List<int>, tl::List<float>>);
    static_assert(!Condition<C, tl::List<int>, tl::List<float>>);
}

TEST(test_concrete_condition) {
    using C = conc::Condition<tl::Prod<tl::List<int, int*>, tl::List<float, float*>>>;
    static_assert(Condition<C, tl::List<int, int*>, tl::List<float, float*>>);
    static_assert(!Condition<C, tl::List<int, char*>, tl::List<float, float*>>);
}

TEST(test_concrete_action) {
    using A = conc::Action<tl::Prod<tl::List<int, int*>, tl::List<float, float*>>>;
    static_assert(Action<A, tl::List<int, int*>, tl::List<float, float*>>);
    static_assert(!Action<A, tl::List<int, char*>, tl::List<float, float*>>);
}

TEST(test_event_concept) {
    struct A {
        using Ids = tl::List<>;
    };
    struct B {};
    static_assert(Event<A>);
    static_assert(!Event<B>);
}

TEST(test_concrete_event) {
    static_assert(Event<conc::Event>);
}

TEST(test_src_state_concept) {
    struct A {
        using Ids = tl::List<int>;
        using Tag = void;
        void on(conc::Event e);
        void operator+(conc::Event e);
    };
    static_assert(SrcState<A>);
}

TEST(test_dst_state_concept) {
    struct A {
        using Id = int;
        using Tag = void;
    };
    static_assert(DstState<A>);
    static_assert(!DstState<void>);
}

TEST(test_transition_concept) {
    struct A {
        using Src = conc::SrcState;
        using Dst = conc::DstState;
        using Event = conc::Event;

        void run(conc::Action<tl::Prod<tl::List<int, float>, tl::List<int, float>>>);
        void operator!=(conc::Action<tl::Prod<tl::List<int, float>, tl::List<int, float>>>);
        void operator==(conc::Condition<tl::Prod<tl::List<int, float>, tl::List<int, float>>>);
        void when(conc::Condition<tl::Prod<tl::List<int, float>, tl::List<int, float>>>);
        void to(conc::DstState);
        void operator=(conc::DstState);  // NOLINT
        bool operator()(int, const float&);
    };
    static_assert(Transition<A>);
}

TEST(test_concrete_state_machine) {
    static_assert(StateMachine<conc::StateMachine>);
}

}  // namespace sml
