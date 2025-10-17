#include <sml/ids.h>

#include <utest/utest.h>

namespace sml::impl::state {

TEST(test_any_id_empty_matches_any) {
    using A = AnyId<>;

    static_assert(A::matches<void>());
    static_assert(A::matches<int>());
}

TEST(test_any_id_matches_exactly) {
    using A = AnyId<int, bool>;

    static_assert(A::matches<bool>());
    static_assert(A::matches<int>());
    static_assert(!A::matches<char>());
}

TEST(test_is_any_id) {
    static_assert(!traits::IsAnyId<void>);
    static_assert(traits::IsAnyId<AnyId<>>);
    static_assert(traits::IsAnyId<AnyId<int, bool>>);
}

}  // namespace sml::impl::state

TESTS_MAIN
