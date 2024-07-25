#pragma once
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

constexpr char pixiv_path[] = "pixiv.list";
constexpr char twitter_path[] = "twitter.list";
