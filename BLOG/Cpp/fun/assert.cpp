#include <iostream>
#include <source_location>


template <typename _Tp, typename... _Args>
struct assert {
    assert(_Tp &&condition, _Args &&...args, std::source_location location = std::source_location::current()) {
        if (condition) return;
        std::cerr << "assert failed: "
            << location.file_name() << ":" << location.line() << ": " << location.function_name() << ": ";
        if constexpr (sizeof...(args) == 0)
            ((std::cerr << args), ...) << std::endl;
    }
};

template <typename _Tp, typename... _Args>
assert(_Tp &&, _Args &&...) -> assert<_Tp, _Args...>;

signed main() {
    assert(0.1 + 0.2 == 0.3, "Hello, World!");
    return 0;
}