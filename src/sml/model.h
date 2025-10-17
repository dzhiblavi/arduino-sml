#pragma once

#include <stdlike/tuple.h>
#include <stdlike/utility.h>

namespace sml {

struct AnyConstRef {
    template <typename T>
    operator const T&() {
        return stdlike::declval<T>();
    }
};

template <typename A>
concept Action = requires(A a, AnyConstRef arg) {
    { a(arg, arg) } -> stdlike::same_as<void>;
};

template <typename C, typename EId>
concept Condition = requires(C c, const EId& arg, AnyConstRef s) {
    { c(s, arg) } -> stdlike::same_as<bool>;
};

using ConcreteAction = decltype([](auto, auto) {});

template <typename E>
using ConcreteCondition = decltype([](auto, const E&) { return false; });

// clang-format off
template <typename E>
concept Event = requires(E e, ConcreteCondition<typename E::Id> c, AnyConstRef s) {
    typename E::Id;
    { e.match(s, stdlike::declval<const typename E::Id&>()) } -> stdlike::same_as<bool>;
    { stdlike::move(e).when(c) }; /*-> Event;*/
    { stdlike::move(e)[c] }; /*-> Event;*/
};
// clang-format on

struct ConcreteEvent {
    using Id = int;  // Id may hold data that will be passed to actions and conditions
    bool match(auto, auto&&);
    ConcreteEvent when(Condition<int> auto);
    ConcreteEvent operator[](Condition<int> auto);
};

struct ConcreteTransition;

struct ConcreteState {
    using Id = int;
    using UId = int;
    ConcreteTransition on(Event auto);
    ConcreteTransition operator+(Event auto);
};

template <typename S>
concept State = requires(S s, ConcreteEvent event) {
    typename S::Id;   // Simple tag, passed to actions and conditions, not used by sml.
    typename S::UId;  // Internal identifier
    { stdlike::move(s).on(event) }; /*-> Transition;*/
    { stdlike::move(s) + event };   /*-> Transition;*/
};

struct ConcreteTransition {
    using Src = ConcreteState;
    using Dst = void;
    using Event = ConcreteEvent;

    ConcreteTransition run(Action auto);
    ConcreteTransition operator|(Action auto);
    ConcreteTransition to(State auto);
    ConcreteTransition operator=(State auto);  // NOLINT
    bool tryExecute(const typename Event::Id&);
};

template <typename T>
concept Transition = requires(T t, ConcreteAction a, ConcreteState s) {
    { State<typename T::Src> };
    { State<typename T::Dst> || stdlike::same_as<void, typename T::Dst> };
    { Event<typename T::Event> };

    { stdlike::move(t).run(a) }; /*-> Transition;*/
    { stdlike::move(t) | a };    /*-> Transition;*/
    { stdlike::move(t).to(s) };  /*-> Transition;*/
    { stdlike::move(t) = s };    /*-> Transition;*/
    { t.tryExecute(stdlike::declval<const typename T::Event::Id&>()) } -> stdlike::same_as<bool>;
};

template <typename T>
struct IsTransitionsTuple : stdlike::false_type {};

template <Transition... Ts>
struct IsTransitionsTuple<stdlike::tuple<Ts...>> : stdlike::true_type {};

template <typename T>
concept TransitionsTuple = IsTransitionsTuple<T>::value;

template <typename T>
concept StateMachine = requires(T s) {
    typename T::InitialId;  // S::Id, where S is the initial SM state
    { s.transitions() } -> TransitionsTuple;
};

static_assert(Action<ConcreteAction>);
static_assert(Condition<ConcreteCondition<int>, int>);
static_assert(Event<ConcreteEvent>);
static_assert(Transition<ConcreteTransition>);
static_assert(State<ConcreteState>);

}  // namespace sml
