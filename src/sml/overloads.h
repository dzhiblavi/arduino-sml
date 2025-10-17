#pragma once

namespace sml {

template <typename... Ts>
struct overloads : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
overloads(Ts...) -> overloads<Ts...>;

}  // namespace sml
