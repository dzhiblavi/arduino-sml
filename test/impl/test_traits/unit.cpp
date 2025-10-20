#include <sml/impl/traits.h>
#include <sml/make.h>

#include <utest/utest.h>

namespace sml {

struct M1 {
    using InitialId = int;

    auto transitions() {
        return table(src<float> + ev<char> = dst<char>);
    }
};

struct M2 {
    using InitialId = int;

    auto transitions() {
        return table(src<> + ev<> = enter<M1>);
    }
};

struct M3 {
    using InitialId = int;

    auto transitions() {
        return table(
            src<float> + ev<float> = bypass,
            src<int> + ev<int> = enter<M2>,
            src<int> + ev<> = enter<M1>);
    }
};

static_assert(StateMachine<M1>);
static_assert(StateMachine<M2>);
static_assert(StateMachine<M3>);

TEST(test_submachines) {
    using SMs = impl::traits::Submachines<M3>;
    static_assert(std::same_as<tl::List<M2, M1>, SMs>);
}

TEST(test_tag_transitions) {
    using Ts = impl::traits::Transitions<M3>;
    using Tagged = impl::traits::TagTransitions<Ts, float>;

    using T0 = tl::At<0, Tagged>;
    using T1 = tl::At<1, Tagged>;
    using T2 = tl::At<2, Tagged>;

    static_assert(std::same_as<float, typename T0::Src::Tag>);
    static_assert(std::same_as<float, typename T1::Src::Tag>);
    static_assert(std::same_as<float, typename T2::Src::Tag>);

    static_assert(std::same_as<float, typename T0::Dst::Tag>);
    static_assert(std::same_as<M2, typename T1::Dst::Tag>);
    static_assert(std::same_as<M1, typename T2::Dst::Tag>);
}

TEST(test_combined_transitions) {
    using Ts = impl::traits::CombinedTransitions<M3>;
    using namespace sml::impl;
    static_assert(
        std::same_as<
            tl::List<
                transition::Tagged<
                    transition::To<
                        transition::Make<
                            state::Src<void, float>,
                            state::Dst<void, sml::KeepStateId>,
                            event::Make<float>>,
                        state::Dst<void, sml::BypassStateId>>,
                    sml::M3>,
                transition::Tagged<
                    transition::To<
                        transition::Make<
                            state::Src<void, int>,
                            state::Dst<void, sml::KeepStateId>,
                            event::Make<int>>,
                        state::Dst<sml::M2, int>>,
                    sml::M3>,
                transition::Tagged<
                    transition::To<
                        transition::Make<
                            state::Src<void, int>,
                            state::Dst<void, sml::KeepStateId>,
                            event::Make<>>,
                        state::Dst<sml::M1, int>>,
                    sml::M3>,
                transition::Tagged<
                    transition::To<
                        transition::Make<
                            state::Src<void>,
                            state::Dst<void, sml::KeepStateId>,
                            event::Make<>>,
                        state::Dst<sml::M1, int>>,
                    sml::M2>,
                transition::Tagged<
                    transition::To<
                        transition::Make<
                            state::Src<void, float>,
                            state::Dst<void, sml::KeepStateId>,
                            event::Make<char>>,
                        state::Dst<void, char>>,
                    sml::M1>>,
            Ts>);
}

TEST(test_filter_transitions_by_event_id) {
    using Ts = impl::traits::Transitions<M3>;
    using Filtered = impl::traits::FilterTransitionsByEventId<int, Ts>;
    using namespace sml::impl;
    static_assert(
        std::same_as<
            tl::List<
                transition::To<
                    transition::Make<
                        state::Src<void, int>,
                        state::Dst<void, KeepStateId>,
                        event::Make<int>>,
                    state::Dst<sml::M2, int>>,
                transition::To<
                    transition::
                        Make<state::Src<void, int>, state::Dst<void, KeepStateId>, event::Make<>>,
                    state::Dst<sml::M1, int>>>,
            Filtered>);
}

TEST(test_filter_state_specs_by_tag) {
    using namespace sml::impl::traits;

    using Specs = tl::List< //
        StateSpec<void, float>,
        StateSpec<void, int>,
        StateSpec<void, char>,
        StateSpec<char, int>
    >;

    using Filtered = FilterStateSpecsByTag<int, Specs>;
    using Expected = tl::List<StateSpec<void, int>, StateSpec<char, int>>;

    static_assert(std::same_as<Expected, Filtered>);
}

TEST(test_get_state_specs) {
    using namespace sml::impl::traits;

    using Ts = CombinedTransitions<M3>;
    using Specs = GetStateSpecs<Ts>;
    using Expected = tl::List<
        StateSpec<float, sml::M3>,
        StateSpec<int, sml::M2>,
        StateSpec<int, sml::M3>,
        StateSpec<int, sml::M1>,
        StateSpec<float, sml::M1>,
        StateSpec<char, sml::M1>>;

    static_assert(std::same_as<Expected, Specs>);
}

TEST(test_get_src_specs) {
    using namespace sml::impl::traits;

    using Ts = CombinedTransitions<M3>;
    using AllSpecs = GetStateSpecs<Ts>;
    using Specs = GetSrcSpecs<Ts, AllSpecs>;
    using Expected = tl::List<
        StateSpec<float, sml::M3>,
        StateSpec<int, sml::M3>,
        StateSpec<int, sml::M2>,
        StateSpec<float, sml::M1>>;

    static_assert(std::same_as<Expected, Specs>);
}

TEST(test_get_event_ids) {
    using namespace sml::impl::traits;

    using Ts = CombinedTransitions<M3>;
    using Expected = tl::List<float, int, char>;
    using Ids = GetEventIds<Ts>;

    static_assert(std::same_as<Expected, Ids>);
}

TEST(test_get_event_ids_by_src_tag) {
    using namespace sml::impl;

    using Ts = traits::CombinedTransitions<M3>;
    using Expected = tl::List<float, int>;  // no char!
    using Ids = traits::GetEventIdsBySrcTag<M3, Ts>;

    static_assert(std::same_as<Expected, Ids>);
}

TEST(test_filter_transitions_by_src_and_event) {
    using namespace sml::impl;

    using Ts = traits::CombinedTransitions<M3>;

    using RTsChar = traits::FilterTransitionsBySrcAndEvent<traits::StateSpec<float, M3>, char, Ts>;
    static_assert(std::same_as<tl::List<>, RTsChar>);

    using RTsInt = traits::FilterTransitionsBySrcAndEvent<traits::StateSpec<int, M3>, int, Ts>;
    using ExpInt = tl::List<
        transition::Tagged<
            transition::To<
                transition::Make<
                    state::Src<void, int>,
                    state::Dst<void, sml::KeepStateId>,
                    event::Make<int>>,
                state::Dst<sml::M2, int>>,
            sml::M3>,
        transition::Tagged<
            transition::To<
                transition::
                    Make<state::Src<void, int>, state::Dst<void, sml::KeepStateId>, event::Make<>>,
                state::Dst<sml::M1, int>>,
            sml::M3>>;
    static_assert(std::same_as<ExpInt, RTsInt>);
}

}  // namespace sml
