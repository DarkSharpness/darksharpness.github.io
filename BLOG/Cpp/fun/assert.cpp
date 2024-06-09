#include <iostream>
#include <format>
#include <source_location>


template <typename _Tp, typename... _Args>
struct assert {
    assert(_Tp &&condition, std::format_string <_Args...> fmt = "", _Args &&...args,
        std::source_location location = std::source_location::current()) {
        if (condition) return;
        std::cerr << "assert failed: "
            << location.file_name() << ":" << location.line() << ": " << location.function_name() << ": ";
        std::cerr << std::format(fmt, std::forward<_Args>(args)...) << std::endl;
    }
};

template <typename _Tp, typename _Fmt, typename... _Args>
assert(_Tp &&, _Fmt &&, _Args &&...) -> assert<_Tp, _Args...>;

signed main() {
    assert(false);
    assert(1 + 1 == 3, "wtf {} {}", 1 + 1, 3);
    assert(0.1 + 0.2 == 0.3, "Hello, World!");
    return 0;
}
