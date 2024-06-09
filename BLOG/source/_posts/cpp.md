---
title: (Modern) C++ 小知识汇总
date: 2024-06-09 23:01:00
updated:
tags: [C++,基础知识]
categories: [C++,基础知识]
keywords: [C++, Modern C++,基础知识]
cover:
mathjax: false
description: 本文是笔者写 C++ 代码得出的一些实践经验，会长期更新
---

众所周知, 笔者 (DarkSharpness) 是一个 Modern C++ 的狂热爱好者. 笔者自高中信息竞赛以来, 主力编程语言一直都是 C++, 在大学的学习过程中, 积累了不少的实践经验, 故开一个帖子计划长期维护. 每次更新会在头部显示.

## string switch

在 C/C++ 中, 你应该用过 `switch` 语句, 其可以高效而直观地表示多分支的逻辑. 但是, `switch` 语句只能接受整数类型的参数, 不能接受字符串类型的参数.

我们自然是无法从语言层面上改变什么, 但是我们可以基于已有的技术实现一个类似的 `string_switch`. 注意到 `switch` 里面只能接受整数或者枚举类型, 我们的思路就是把字符串转换为整数或者枚举类型. 一个非常 naive 的思路是用 `std::unordered_map` (或者 `std::map`) 来实现. 但是, 这样可能存在一些问题: 首先 `std::unordered_map` 并不支持 `constexpr` 的静态对象, 因为其涉及了动态内存分配. 而且, `case` 里面的整数也要求是 `constexpr` 的, 如何在编译器就能得到具体的哈希值, 如何解决哈希冲突, 都是需要考虑的问题.

虽然 `constexpr std::unordered_map` 看起来是不行了, 但是这个思路是没问题的. 我们最核心的思路就是把字符串转化为可以枚举的整数类. 因此, 我们可以自己手写一个 `hash` 函数, 或者调用 `std::hash` 函数, 来得到一个 `constexpr` 的整数值, 然后我们只需要存储这些整数值就行了. 如下所示:

```cpp
template <std::size_t _Base = 131>
constexpr auto my_hash(std::string_view view) -> std::size_t {
    auto hash = std::size_t{0};
    for (auto c : view) hash = hash * _Base + c;
    return hash;
}

void example(std::string_view input) {
    switch (my_hash(input)) {
        case my_hash("hello"):
            std::cout << "hello" << std::endl;
            break;
        case my_hash("world"):
            std::cout << "world" << std::endl;
            break;
        default:
            std::cout << "default" << std::endl;
            break;
    }
}
```

这就是我们的实现的原型了, 事情似乎有点太简单了. 现实中, 可能并没有这么简单. 对于任意输入的字符串, 我们可能需要考虑哈希冲突的问题. 对于要 match 的那些字符串, 如果出现了冲突, 在编译期间就会直接出错, 而我们只需要简单的把模板中的 `_Base` 替换一下就行了. 比较麻烦的是, 即使我们进入了某个 `case`, 我们也不能保证输入的字符串和要匹配的一样. 我们需要额外的判等.

```c++
void example_1(std::string_view input) {
    switch (my_hash(input)) {
        case my_hash("hello"):
            if (input == "hello") {
                std::cout << "hello" << std::endl;
            }
            break;
        case my_hash("world"):
            if (input == "world") {
                std::cout << "world" << std::endl;                
            }
            break;
        default:
            std::cout << "default" << std::endl;
            break;
    }
}
```

这样以后, 其基本就是一个完美的 `string switch` 了, 有需要的话可以自行修改 `my_hash` 函数. 但是, 我们还是要手写一遍判等, 这样非常麻烦, 而且容易出错. 这时候, 我们可以请出 C 语言的最终杀器: 宏. 以下是作者自己的实现:

```c++
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
```

当然, 既然都用到宏了, 自然可以再结合 VA_ARGS 来实现更加通用的 `string switch`, 如果有需求可以自己定制.

简而言之, 借助 `constexpr hash` 函数, 以及宏, 我们可以实现一个类似于 `string switch` 的功能. 如果有需求, 也可以自行修改.

## assert in C++

如果你写过 C, 那你可能用过 `assert` 这个宏, 用于在运行时检查某个条件是否满足, 如果不满足, 则会终止程序, 并且详细地输出错误信息. 但是, 既然我们都用了 C++ 了, 为什么不用 C++ 的方式来实现呢?

首先, 我们先看一下 C 的 `assert` 都输出了些什么. 文件, 行号, 函数名...... 这些在 C++ 里面怎么获取呢? 如果用 `__LINE__` 这类 C 里面的宏, 那又违背了我们的初衷. 幸运的是, C++ 20 提供了 `std::source_location` 类, 可以获取到文件名, 行号, 函数名等信息. 以下是一个简单的实现:

```cpp
template <typename _Tp>
void assert(_Tp &&condition,
    std::source_location location = std::source_location::current()) {
    if (condition) return;
    std::cerr << "Assertion failed: " << location.file_name() << ":"
              << location.line() << " " << location.function_name() << std::endl;
}
```

当然, 这样的实现可能还有一些不够完美的地方. 用户不能自定义输出信息, 光秃秃的报错信息可能不够友好. 而如果要在运行时生成输出信息字符串, 可能又会映入不小的性能开销. 因此, 我们应该支持 assert 传入多个参数来自定义输出信息. 以下是一个更加完善的实现:

```cpp
template <typename _Tp, typename... _Args>
void assert(_Tp &&condition, _Args &&...args,
    std::source_location location = std::source_location::current()) {
    if (condition) return;
    std::cerr << "Assertion failed: " << location.file_name() << ":"
              << location.line() << " " << location.function_name() << std::endl;
    if constexpr (sizeof...(args) > 0) {
        (std::cerr << ... << args) << std::endl;
    }
}
```

然而, 如果你真的这么写了, 你会发现这种代码无法通过编译. 这是因为在调用 `assert` 的时候, 类型替换会失败. 你传入的最后一个参数会被尝试与 `std::source_location` 匹配, 但是显然是不行的. 这听起来非常令人沮丧, 难道我们在每个调用处都必须要手写一个 `std::source_location::current()` 吗? 当然不是! 除了函数模板, 我们还有类模板. 配合类模板的推导模板, 我们可以实现这个功能. 以下是一个完整的实现:

```cpp
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

void test() {
    assert(0.1 + 0.2 == 0.3, "Hello, World!");
}
```

为了避免使用危险的宏, 我们最后只能选择了这种扭曲的方式实现了我们的 `assert`. 但是, 这种方式也有一些优点. 首先, 我们可以自定义输出信息, 其次, 我们可以在运行时生成输出信息, 而不会有性能开销. 如果你觉得太丑了, 你甚至可以借助 `format` 来实现更加优雅的格式化输出. 其自由度还是非常高的, 比起 C 的 `assert`.
