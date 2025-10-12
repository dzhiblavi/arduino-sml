#pragma once

#include "sml/model.h"

template <sm::model::SomeAction A, sm::model::SomeAction B>
sm::model::SomeAction auto operator,(A a, B b) {
    return [a = stdlike::move(a), b = stdlike::move(b)](const auto& e) {
        a(e);
        b(e);
    };
}

template <sm::model::SomeGuard A>
sm::model::SomeGuard auto operator!(A a) {
    return [a = stdlike::move(a)](const auto& e) { return !a(e); };
}

template <sm::model::SomeGuard A, sm::model::SomeGuard B>
sm::model::SomeGuard auto operator&&(A a, B b) {
    return [a = stdlike::move(a), b = stdlike::move(b)](const auto& e) { return a(e) && b(e); };
}

template <sm::model::SomeGuard A, sm::model::SomeGuard B>
sm::model::SomeGuard auto operator||(A a, B b) {
    return [a = stdlike::move(a), b = stdlike::move(b)](const auto& e) { return a(e) || b(e); };
}
