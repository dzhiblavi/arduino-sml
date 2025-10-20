#if __has_include(<map>)

#include <sml/make.h>
#include <sml/overloads.h>
#include <sml/sm.h>
#include <sml/syntax.h>

#include <utest/utest.h>

#include <map>
#include <string>

namespace sml {

std::map<std::string, std::string> obj;
std::string key;
std::string value;

auto log(auto name) {
    return overloads{
        [name](auto, char c) { LINFO("%s received %c", name, c); },
        [name](auto, OnEnterEventId) { LINFO("%s received onEnter", name); },
    };
};

auto chr(char c) {
    return [c](auto, char x) { return x == c; };
}

auto not_chr(char c) {
    return [c](auto, char x) { return x != c; };
}

struct invalid {};

struct KVParser {
    struct wait_for_op_quote_v {};
    struct wait_for_colon {};
    struct reading_key {};
    struct reading_value {};

    using InitialId = reading_key;  // NOLINT

    auto transitions() {
        auto clear = [](auto...) {
            key.clear();
            value.clear();
        };

        auto append = [](auto& s) { return [&s](auto, char c) { s.push_back(c); }; };
        auto add_kv = [](auto...) { obj[key] = value; };

        return table(
            src<> + ev<> != log("KVParser") = bypass,

            src<reading_key> + onEnter != clear,
            src<reading_key> + ev<char> == chr('"') = dst<wait_for_colon>,
            src<reading_key> + ev<char> != append(key),
            src<reading_key> + ev<char> = dst<invalid>,

            src<wait_for_colon> + ev<char> == chr(':') = dst<wait_for_op_quote_v>,
            src<wait_for_colon> + ev<char> = dst<invalid>,

            src<wait_for_op_quote_v> + ev<char> == chr('"') = dst<reading_value>,
            src<wait_for_op_quote_v> + ev<char> = dst<invalid>,

            src<reading_value> + ev<char> == chr('"') != add_kv = x,
            src<reading_value> + ev<char> != append(value)  //
        );
    }
};

struct MapParser {
    struct wait_for_op_br {};
    struct wait_for_next_kv {};
    struct wait_for_op_quote {};
    using InitialId = wait_for_op_br;  // NOLINT

    auto transitions() {
        return table(
            src<> + ev<> != log("MapParser") = bypass,

            src<wait_for_op_br> + ev<char> == chr('{') = dst<wait_for_op_quote>,
            src<wait_for_op_br> + ev<char> = dst<invalid>,

            src<wait_for_op_quote> + ev<char> == chr('"') = enter<KVParser>,
            src<wait_for_op_quote> + ev<char> == chr('}') = x,
            src<wait_for_op_quote> + ev<char> = dst<invalid>,

            exit<KVParser> + onEnter = dst<wait_for_next_kv>,

            src<wait_for_next_kv> + ev<char> == chr(',') = dst<wait_for_op_quote>,
            src<wait_for_next_kv> + ev<char> == chr('}') = x,
            src<wait_for_next_kv> + ev<char> = dst<invalid>  //
        );
    }
};

TEST(test_sm_kv_parser) {
    SM<MapParser> sm;
    obj.clear();

    SECTION("empty map") {
        sm.begin();
        std::string_view s("{}");
        std::map<std::string, std::string> expected{};

        for (char c : s) {
            TEST_ASSERT_TRUE(sm.feed(c));
        }

        TEST_ASSERT_TRUE((sm.is<MapParser, TerminalStateId>()));
        TEST_ASSERT_TRUE(expected == obj);
    }

    SECTION("single key-value") {
        sm.begin();
        std::string_view s(R"({"key":"value"})");
        std::map<std::string, std::string> expected{{"key", "value"}};

        for (char c : s) {
            TEST_ASSERT_TRUE(sm.feed(c));
        }

        TEST_ASSERT_TRUE((sm.is<MapParser, TerminalStateId>()));
        TEST_ASSERT_TRUE(expected == obj);
    }

    SECTION("single empty key, value") {
        sm.begin();
        std::string_view s(R"({"":""})");
        std::map<std::string, std::string> expected{{"", ""}};

        for (char c : s) {
            TEST_ASSERT_TRUE(sm.feed(c));
        }

        TEST_ASSERT_TRUE((sm.is<MapParser, TerminalStateId>()));
        TEST_ASSERT_TRUE(expected == obj);
    }

    SECTION("multiple key-values") {
        sm.begin();
        std::string_view s(R"({"a":"","":"c","d":"e"})");
        std::map<std::string, std::string> expected{{"a", ""}, {"", "c"}, {"d", "e"}};

        for (char c : s) {
            TEST_ASSERT_TRUE(sm.feed(c));
        }

        TEST_ASSERT_TRUE((sm.is<MapParser, TerminalStateId>()));
        TEST_ASSERT_TRUE(expected == obj);
    }

    SECTION("not an object") {
        sm.begin();
        sm.feed('a');
        TEST_ASSERT_TRUE((sm.is<MapParser, invalid>()));
    }

    SECTION("invalid key start") {
        sm.begin();
        sm.feed('{');
        sm.feed('a');
        TEST_ASSERT_TRUE((sm.is<MapParser, invalid>()));
    }

    SECTION("missing value") {
        sm.begin();
        std::string_view s(R"({"a"})");
        for (auto c : s) {
            sm.feed(c);
        }
        TEST_ASSERT_TRUE((sm.is<KVParser, invalid>()));
    }

    SECTION("invalid value start") {
        sm.begin();
        std::string_view s(R"({"a":b})");
        for (auto c : s) {
            sm.feed(c);
        }
        TEST_ASSERT_TRUE((sm.is<KVParser, invalid>()));
    }

    SECTION("missing object end") {
        sm.begin();
        std::string_view s(R"({"a":"b")");
        for (auto c : s) {
            sm.feed(c);
        }
        TEST_ASSERT_FALSE((sm.is<MapParser, TerminalStateId>()));
    }
}

}  // namespace sml

#endif
