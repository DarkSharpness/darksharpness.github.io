#include <bits/stdc++.h>
// #include <Dark/inout>
// template <typename T>
// auto overload(T) -> std::enable_if_t<std::is_copy_constructible_v<T>>;
// template <typename T>
// auto overload(T) -> std::enable_if_t<std::is_move_constructible_v<T>>;


void overload(std::copy_constructible auto) {
    std::cout << "copy\n";
}
void overload(std::move_constructible auto) {
    std::cout << "move\n";
}

template <class T = int>
concept signed_integer = std::is_integral_v <T> && std::is_signed_v <T>;

template <signed_integer T>
auto lowbit1(T x) {
    return x & (-x);
}

template <class T>
requires signed_integer <T> && (true || requires(T x) {
    x.x;
})
auto lowbit2(T x) {
    return x & (-x);
}

template <class T>
auto lowbit3(T x) requires signed_integer <T> {
    return x & (-x);
}

auto lowbit4(signed_integer auto x) {
    return x & (-x);
}


struct bool_true {
    int x;
    // 注意，这个 constexpr 是必须的!
    constexpr operator bool() noexcept { return x; }
};

template <class T = void>
requires true
void null_func() {}


signed main() {
    overload(0);
    std::integral decltype(auto) x = 1;
    std::cout 
        << lowbit1(10) << ' '
        << lowbit2(12) << ' '
        << lowbit3(14) << ' '
        << lowbit4(16) << '\n';
    null_func();
    return 0;
}