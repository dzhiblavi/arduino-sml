#pragma once

#include <stdlike/utility.h>
#include <stdlike/tuple.h>

namespace sm::model {

struct AnyConstRef {
    template <typename T>
    operator const T&() {
        return stdlike::declval<T>();
    }
};

template <typename A, typename Event>
concept Action = requires(A action, const Event& event) {
    { action(event) } -> stdlike::same_as<void>;
};

template <typename A>
concept SomeAction = requires(A action, AnyConstRef any) {
    { action(any) } -> stdlike::same_as<void>;
};

template <typename G, typename Event>
concept Guard = requires(G guard, const Event& event) {
    { guard(event) } -> stdlike::same_as<bool>;
};

template <typename G>
concept SomeGuard = requires(G guard, AnyConstRef any) {
    { guard(any) } -> stdlike::same_as<bool>;
};

template <typename T>
concept Transition = requires(T& t, AnyConstRef any) {
    typename T::SrcState;
    typename T::DstState;
    typename T::Event;
    T::IsInitial;

    { t.execute(any) } -> stdlike::same_as<bool>;
};

template <typename T>
struct IsTransitionsTuple : stdlike::false_type {};

template <model::Transition... Ts>
struct IsTransitionsTuple<stdlike::tuple<Ts...>> : stdlike::true_type {};

template <typename T>
concept TransitionsTuple = IsTransitionsTuple<T>::value;

template <typename T>
concept StateMachine = requires(T s) {
    { s() } -> TransitionsTuple;
};

}  // namespace sm::model
