#pragma once

#include "sml/model.h"

namespace sml::impl {

template <StateMachine TM, typename TId>
struct RefUId {
    using Id = TId;
    using M = TM;
};

namespace traits {

template <typename UId>
concept RefUId = requires() {
    typename UId::Id;
    StateMachine<typename UId::M>;
};

template <typename UId>
struct IsRefUId : stdlike::false_type {};

template <StateMachine M, typename Id>
struct IsRefUId<impl::RefUId<M, Id>> : stdlike::true_type {};

template <typename S>
struct IsRefState : stdlike::false_type {};

template <State S>
struct IsRefState<S> : IsRefUId<typename S::UId> {};

template <typename UId>
struct SubmachineFromUIdI {
    using type = void;
};

template <StateMachine M, typename Id>
struct SubmachineFromUIdI<impl::RefUId<M, Id>> {
    using type = M;
};

template <typename UId>
using SubmachineFromUId = typename SubmachineFromUIdI<UId>::type;

}  // namespace traits

}  // namespace sml::impl
