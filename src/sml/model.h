#pragma once

#include <supp/type_list.h>

#include <stdlike/tuple.h>
#include <stdlike/utility.h>

namespace sml {

template <typename A, typename SIds, typename EIds>
concept Action = tl::CallableN<A, tl::Prod<SIds, EIds>>;

template <typename C, typename SIds, typename EIds>
concept Condition = tl::CallableNR<bool, C, tl::Prod<SIds, EIds>>;

template <typename E>
concept Event = tl::IsList<typename E::Ids>;

namespace conc {

template <typename SId, typename EId>
struct Condition1 {
    bool operator()(SId, const EId&);
};

template <typename L>
struct Condition;

template <typename... SIds, typename... EIds>
struct Condition<tl::List<tl::List<SIds, EIds>...>> : Condition1<SIds, EIds>... {
    using Condition1<SIds, EIds>::operator()...;
};

template <typename SId, typename EId>
struct Action1 {
    void operator()(SId, const EId&);
};

template <typename L>
struct Action;

template <typename... SIds, typename... EIds>
struct Action<tl::List<tl::List<SIds, EIds>...>> : Action1<SIds, EIds>... {
    using Action1<SIds, EIds>::operator()...;
};

struct Event {
    using Ids = tl::List<int, float>;
};

struct SrcState {
    using Ids = tl::List<int, float>;
    using Tag = void;
};

struct DstState {
    using Id = int;
    using Tag = void;
};

struct StateMachine {
    using InitialId = int;

    auto transitions() {
        return stdlike::tuple{};
    }
};

}  // namespace conc

template <typename S>
concept SrcState = requires(S s, conc::Event event) {
    requires tl::IsList<typename S::Ids>;
    typename S::Tag;  // Internal
    { stdlike::move(s).on(event) };
    { stdlike::move(s) + event };
};

template <typename S>
concept DstState = requires(S s) {
    typename S::Id;
    typename S::Tag;
};

template <typename T>
concept Transition = requires(
    T t,
    conc::Action<tl::Prod<typename T::Src::Ids, typename T::Event::Ids>> a,
    conc::Condition<tl::Prod<typename T::Src::Ids, typename T::Event::Ids>> c,
    conc::DstState dst) {
    { SrcState<typename T::Src> };
    { DstState<typename T::Dst> };
    { Event<typename T::Event> };

    { stdlike::move(t).run(a) };
    { stdlike::move(t) != a };
    { stdlike::move(t).to(dst) };
    { stdlike::move(t) = dst };
    { stdlike::move(t).when(c) };
    { stdlike::move(t) == c };
    { tl::CallableNR<bool, T, tl::Prod<typename T::Src::Ids, typename T::Event::Ids>> };
};

template <typename T>
struct IsTransitionsTuple : stdlike::false_type {};

template <Transition... Ts>
struct IsTransitionsTuple<stdlike::tuple<Ts...>> : stdlike::true_type {};

template <typename T>
concept TransitionsTuple = IsTransitionsTuple<T>::value;

template <typename T>
concept StateMachine = requires(T s) {
    typename T::InitialId;
    { s.transitions() } -> TransitionsTuple;
};

}  // namespace sml
