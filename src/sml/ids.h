#pragma once

#include <supp/type_list.h>

namespace sml {

struct OnEnterEventId {};
struct OnExitEventId {};

struct TerminalStateId {};
struct BypassStateId {};

template <typename... TIds>
struct AnyId {
    using Ids = tl::List<TIds...>;

    template <typename Id>
    static constexpr bool matches() {
        return tl::Contains<Ids, Id> || tl::Empty<Ids>;
    }
};

namespace traits {

template <typename Id>
struct IsAnyIdI : stdlike::false_type {};

template <typename... Ids>
struct IsAnyIdI<sml::AnyId<Ids...>> : stdlike::true_type {};

template <typename Id>
concept IsAnyId = IsAnyIdI<Id>::value;

}  // namespace traits

}  // namespace sml
