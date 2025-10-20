#if __has_include(<vector>)

#include <sml/make.h>
#include <sml/overloads.h>
#include <sml/sm.h>
#include <sml/syntax.h>

#include <utest/utest.h>

#include <vector>

namespace sml {

std::vector<std::string> words;
char boundary;

auto new_word = [](auto...) { words.emplace_back(); };
auto push_word = [](auto, char c) { words.back().push_back(c); };
auto is_boundary = [](auto, char c) { return c == boundary; };
auto log = [](auto name) {
    return overloads{
        [name](auto, char c) { LINFO("%s received %c", name, c); },
        [name](auto, OnEnterEventId) { LINFO("%s received onEnter", name); },
    };
};

struct Word {
    struct reading_word {};
    using InitialId = reading_word;  // NOLINT

    auto transitions() {
        return table(
            src<> + ev<> != log("Word") = bypass,
            src<reading_word> + onEnter != new_word,          // new word started
            src<reading_word> + ev<char> == is_boundary = x,  // word ended
            src<reading_word> + ev<char> != push_word         // new word character
        );
    }
};

struct Splitter {
    struct init {};
    using InitialId = init;  // NOLINT

    auto transitions() {
        return table(
            src<> + ev<> != log("Splitter") = bypass,
            src<init> + onEnter = enter<Word>,
            exit<Word> + onEnter = dst<init>  //
        );
    }
};

TEST(test_sm_string_splitter) {
    SM<Splitter> sm;

    words.clear();
    boundary = ',';

    SECTION("single word") {
        sm.begin();
        std::string_view s = "abacaba";
        std::vector<std::string> expected{"abacaba"};

        for (char c : s) {
            TEST_ASSERT_TRUE(sm.feed(c));
        }

        TEST_ASSERT_TRUE((sm.is<Word, Word::reading_word>()));
        TEST_ASSERT_TRUE(expected == words);
    }

    SECTION("word list") {
        sm.begin();
        std::string_view s = "a,b,c,d";
        std::vector<std::string> expected{"a", "b", "c", "d"};

        for (char c : s) {
            TEST_ASSERT_TRUE(sm.feed(c));
        }

        TEST_ASSERT_TRUE((sm.is<Word, Word::reading_word>()));
        TEST_ASSERT_TRUE(expected == words);
    }

    SECTION("empty word accepted") {
        sm.begin();
        std::string_view s = "a,,,d";
        std::vector<std::string> expected{"a", "", "", "d"};

        for (char c : s) {
            TEST_ASSERT_TRUE(sm.feed(c));
        }

        TEST_ASSERT_TRUE((sm.is<Word, Word::reading_word>()));
        TEST_ASSERT_TRUE(expected == words);
    }
}

}  // namespace sml

#endif
