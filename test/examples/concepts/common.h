#pragma once

#include <algorithm>
#include <array>

#include <utest/utest.h>

namespace sml {

struct Order {
    template <std::same_as<int>... Is>
    auto ord(Is... is) {
        std::array<int, sizeof...(Is)> arr{is...};

        for (size_t i = 0; i < arr.size(); ++i) {
            max_ord = std::max(max_ord, arr[i]);
        }

        return [this, arr = std::move(arr), index = 0](auto...) mutable {
            TEST_ASSERT_EQUAL(next, arr[index]);
            ++index;
            ++next;
        };
    }

    void reset() {
        next = 0;
    }

    bool finished() {
        return next == max_ord + 1;
    }

    int next = 0;
    int max_ord = 0;
};

auto ifge(int x) {
    return [x](auto, int y) { return y >= x; };
}

}  // namespace sml
