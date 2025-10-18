#pragma once

#include "sml/model.h"

auto operator&&(auto a, auto b) {
    return [a = std::move(a), b = std::move(b)](auto s, const auto& e) {
        return a(s, e) && b(s, e);
    };
}

auto operator||(auto a, auto b) {
    return [a = std::move(a), b = std::move(b)](auto s, const auto& e) {
        return a(s, e) || b(s, e);
    };
}

auto operator!(auto a) {
    return [a = std::move(a)](auto s, const auto& e) { return !a(s, e); };
}

auto operator,(auto a, auto b) {
    return [a = std::move(a), b = std::move(b)](auto s, const auto& e) {
        a(s, e);
        b(s, e);
    };
}
