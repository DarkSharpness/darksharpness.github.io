#include <iostream>

template <std::size_t _Base = 131>
constexpr auto my_hash(std::string_view view) -> std::size_t {
    auto hash = std::size_t{0};
    for (auto c : view) hash = hash * _Base + c;
    return hash;
}

void example2(std::string_view input) {
    #define match(str) \
    case my_hash(str):  if (input != str) break; else

    switch (my_hash(input)) {
        match("hello") {
            std::cout << "hello" << std::endl;
            break;
        }

        match("world") {
            std::cout << "world" << std::endl;
            break;
        }

        match("return") return; // Allow one-liner

        default:
            std::cout << "default" << std::endl;
            break;
    }

    #undef match
}

signed main() {
    std::string str;
    std::cin >> str;
    example2(str);
    return 0;
}
