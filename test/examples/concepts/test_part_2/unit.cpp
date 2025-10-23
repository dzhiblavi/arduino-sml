#include "examples/concepts/common.h"

#include <sml/make.h>
#include <sml/overloads.h>
#include <sml/sm.h>
#include <sml/syntax.h>

#include <utest/utest.h>

namespace sml {

TEST(test_sm_transition_bypass) {
    static Order o;
    o.reset();

    struct M {
        using InitialId = int;

        auto transitions() {
            return table(
                src<int> + ev<int> != o.ord(0) = bypass,
                src<int> + ev<int, char> != o.ord(1) = bypass,
                src<int> + ev<int, float> != o.ord(2) = bypass  //
            );
        }
    };

    SM<M> s;
    TEST_ASSERT_TRUE(s.feed(0));
    TEST_ASSERT_TRUE(o.finished());
}

TEST(test_sm_event_builder_syntax) {
    static Order o;
    o.reset();

    struct M {
        using InitialId = int;

        auto transitions() {
            return table(
                // clang-format off
                src<int>.on(ev<int>)
                    .when(ifge(1))
                    .run(o.ord(0))
                    .to(dst<char>),
                src<char>.on(ev<int>)
                    .when(ifge(1))
                    .when(ifge(2))
                    .run(o.ord(1))
                    .to(dst<float>),
                src<float>.on(ev<int>)
                    .run(o.ord(2, 4))
                    .when(ifge(1))
                    .run(o.ord(3, 5))
                    .when(ifge(2))
                    .run(o.ord(6)).to(dst<int>)
                // clang-format on
            );
        }
    };

    SM<M> s;
    TEST_ASSERT_FALSE(s.feed(0));
    TEST_ASSERT_TRUE(s.feed(1));

    TEST_ASSERT_FALSE(s.feed(1));
    TEST_ASSERT_TRUE(s.feed(2));

    TEST_ASSERT_FALSE(s.feed(1));
    TEST_ASSERT_TRUE(s.feed(2));
    TEST_ASSERT_TRUE(o.finished());
}

}  // namespace sml

TESTS_MAIN
