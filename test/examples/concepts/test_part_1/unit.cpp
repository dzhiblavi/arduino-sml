#include "examples/concepts/common.h"

#include <sml/make.h>
#include <sml/overloads.h>
#include <sml/sm.h>
#include <sml/syntax.h>

#include <utest/utest.h>

namespace sml {

TEST(test_sm_submachine_concept) {
    static Order o;
    o.reset();

    struct S {
        using InitialId = float;  // NOLINT

        auto transitions() {
            return table(
                src<float> + ev<float> != o.ord(1, 4) = x  // x = exit submachine
            );
        }
    };

    struct M {
        using InitialId = int;  // NOLINT

        auto transitions() {
            return table(
                src<int> + ev<int> != o.ord(0, 3) = enter<S>,  // enter submachine
                exit<S> + onEnter != o.ord(2, 5) = dst<int>    // exit submachine
            );
        }
    };

    SM<M> s;
    TEST_ASSERT_TRUE(s.feed(10));
    TEST_ASSERT_TRUE(s.feed(1.f));
    TEST_ASSERT_TRUE(s.feed(10));
    TEST_ASSERT_TRUE(s.feed(1.f));
    TEST_ASSERT_TRUE(o.finished());
}

TEST(test_sm_event_id_specifications) {
    static Order o;
    o.reset();

    struct M {
        using InitialId = int;

        auto transitions() {
            return table(
                src<int> + ev<int> != o.ord(0) = dst<char>,            // concrete event
                src<char> + ev<char, float> != o.ord(1) = dst<float>,  // multi-event
                src<float> + ev<> != o.ord(2) = dst<int>               // wildcard event
            );
        }
    };

    SM<M> s;

    SECTION("event specs demonstration") {
        TEST_ASSERT_TRUE(s.feed(10));
        TEST_ASSERT_TRUE(s.feed('a'));
        TEST_ASSERT_TRUE(s.feed(1.f));
        TEST_ASSERT_TRUE(o.finished());
    }

    SECTION("unknown event does not make it to wildcard") {
        TEST_ASSERT_FALSE(s.feed(nullptr));
        TEST_ASSERT_FALSE(o.finished());
    }
}

TEST(test_sm_event_conditions_specification) {
    static Order o;
    o.reset();

    struct M {
        using InitialId = int;

        auto transitions() {
            return table(
                // simple condition
                src<int> + ev<int> == ifge(1) != o.ord(0) = dst<char>,
                // multiple conditions
                src<char> + ev<int> == ifge(1) == ifge(2) != o.ord(1) = dst<float>,
                // chain actions and conditions
                src<float> + ev<int>           //
                    != o.ord(2, 4) == ifge(1)  //
                    != o.ord(3, 5) == ifge(2)  //
                    != o.ord(6)                //
                = dst<int>                     //
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
