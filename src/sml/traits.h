#pragma once

#include "sml/impl/traits.h"
#include "sml/model.h"

#include <supp/type_list.h>

namespace sml::traits {

using impl::traits::CombinedStateMachine;
using impl::traits::Transitions;
using impl::traits::TransitionsTuple;

template <tl::IsList Transitions>
using AllEventIds = typename impl::traits::AllEventIds<Transitions>::type;

template <typename InitUId, tl::IsList Transitions>
using AllStateUIds = typename impl::traits::AllStateUIds<InitUId, Transitions>::type;

template <StateMachine M>
using Submachines = typename impl::traits::Submachines<M>::type;

}  // namespace sml::traits
