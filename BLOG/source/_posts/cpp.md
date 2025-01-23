---
title: (Modern) C++ 小技巧汇总
date: 2024-06-09 23:01:00
updated:
tags: [C++,基础知识]
categories: [C++,基础知识]
keywords: [C++, Modern C++,基础知识]
cover: https://s3.bmp.ovh/imgs/2024/06/10/16be8fd59a1768c7.jpg
mathjax: false
description: 本文是笔者写 C++ 代码得出的一些实践经验，会长期更新
---

众所周知, 笔者 (DarkSharpness) 是一个 Modern C++ 的狂热爱好者. 笔者自高中信息竞赛以来, 主力编程语言一直都是 C++, 在大学的学习过程中, 积累了不少的实践经验, 故开一个帖子计划长期维护. 每次更新会在头部显示.

## small size optimization

C++ 人最喜欢的一点就是优化性能. 既然要优化性能, 少不了的一点就是数据的局部化. 众所周知, 指针间接访问是一件不太好的事情, 一般来说连续的内存访问显然要好于不连续的, 这是体系结构告诉我们的.

那么, 基于 `locality`, 我们能做哪些优化呢? 最 naive 的想法就是: 直接把数据存在自己的结构体里面. 这样面临着一个问题: 如果数据的大小是未知的, 比如 `std::string` 长度是可变的, 那你不可能开一个无穷大的结构体. 但是没关系, small size optimization 的核心就在于, 对于大小比较小, 能在结构体内放得下的, 就尽量放在里面.

在笔者的 `gcc 13.2` 中, `std::string` 的实现就采用了 SSO 的优化, 即对于长度不超过 15 的字符串, 开在自己内部的一个 buffer 里面.

```cpp
// a sample code 
struct string {
    char *ptr;
    std::size_t length;
    union {
        char buffer[16];
        std::size_t capacity;
    };
};
```

~~这就是全部~~ 这当然不是全部. `std::any` 也采用了类似的优化, 对于不超过 8 byte 的小对象, 就放在自己的结构体内部. 不仅如此, 在工程实践中, SSO 也被广泛运用. 在 llvm 中, 有一个模板类 `small_vector`, 顾名思义就是一个针对一般大小比较小的 `vector` 进行了优化. 比如 `llvm::small_vector<int, 8>` 即表示一个最多可以在内部存储 8 个 int 的小 vector, 如果通过 profiling 发现程序中确实基本上 `vector` 大小不会超过 8, 那么就可以享受几乎和数组一样的性能 (局部存储, 连续访问). 在 `PyTorch 2.5.0` 源码中, `IValue` 类型 (Interpreter Value) 也采用了类似的优化 (类似 std::any). `IValue` 作为 `python` 解释器中的动态类型, 具有很强的动态性, 但是 `PyTorch` 涉及的类型基本上只有 `Tensor` (一个指针大小), `int`, `float`, `device` 等等, 这些实际都不会超过 `8 byte`. 因此, 使用 `IValue` 来表示解释器中的某个值, 可以尽可能地避免间接引用, 并且可以减少潜在的堆内存分配.

总结一句话: 当数据不大的时候, 尽可能放结构体内的 buffer 里来增强数据局部性.

## bit-field

这个其实算不上 modern C++ 的部分, 这是 C 继承下来的一个重要 feature.

在一些偏底层且空间/性能敏感的领域, 我们可能需要把多个数据压缩存储到一起. 举个例子, int4 量化的时候, 我们可能需要把 8 个 4 bit 的数(表示范围是 -8 ~ 7)压缩到一个 int 中 (4 * 8 = 32). 再比如说, 在嵌入式开发中, 某些硬件寄存器每个 bit 可能对应不同的 flag, 我们在读出这个寄存器的值的时候, 可能需要把这些 flag 读出来.

以上这些需求, 最容易想到的做法是使用位运算, 取出一个数字的特定几位. 然而, 这样的代码难以维护, 各种左右移, 以及掩码操作, 稍微复杂一点代码就会变得难以阅读, 即使设计了对应接口, 其直观性还是一般, 如下所示.

```cpp
struct int4_8 {
    int data;
};

// 获取第 i 个 4 bit 数
template <int which>
int get(const int4_8 &x) {
    return (x.data >> (which * 4)) & 0xf;
}

// 把第 i 个 4 bit 数设置为 value
template <int which>
void set(int4_8 &x, int value) {
    x.data &= ~(0xf << (which * 4));
    x.data |= (value & 0xf) << (which * 4);
}
```

我们希望我们能想操纵一个普通的变量那样, 操控一些 bit. 遗憾的是, 计算机中的最小寻址单元是 byte, 我们并不存在 bit 的引用. 但是, C++ 提供了一个很好的解决方案: bit-field. 我们可以使用 bit-field 来定义一个结构体, 其中的成员变量可以指定其占用的 bit 数, 如下所示.

```cpp
struct int4_8 {
    int x0 : 4; // 0 ~ 3 bit
    int x1 : 4; // 4 ~ 7 bit
    int x2 : 4; // 8 ~ 11 bit
    int x3 : 4; // 12 ~ 15 bit
    int x4 : 4; // 16 ~ 19 bit
    int x5 : 4; // 20 ~ 23 bit
    int x6 : 4; // 24 ~ 27 bit
    int x7 : 4; // 28 ~ 31 bit
};

void test() {
    int4_8 x;
    x.x0 = 1;
    x.x1 = -3;
    std::cout << x.x0 << " " << x.x1 << std::endl;
}
```

通过这样的方式, 我们可以直接访问到一个 int 中的特定几位, 而不需要手动进行位运算. 我们可以在 [cppreference](https://en.cppreference.com/w/cpp/language/bit_field#:~:text=The%20type%20of%20a%EE%80%80%20bit-field%EE%80%81) 上查看到更多关于 bit-field 的细节.

在笔者的实践中, 一般不会太在意 cppreference 上说到的所有细节, 但是笔者认为以下这些还是比较重要的:

首先, bit-field 的类型必须是整数类型. 这还是比较好理解的, 因为其本质就是对于整数位运算的某种语法糖.

其次, 如果希望达到节约空间的目的, 被压缩在同一个 int 中的 bit-field 之和显然不能超过 int 的 bit 数量, 超过的 bit-field 部分一般来说会被放到下一个 int 中. 自然, 这中间可能存在一些 padding, 以保证对齐.

```cpp
struct bit_pack {
    int x : 16;
    int y : 14;

    // 这里有 2 bit 的 padding,
    // 因为下一个 z : 16 放不下了, 第一个 int 只剩下2 bit
    // 请注意, 这是一个实现定义行为, 不同的编译器可能会有不同的行为.
    // 一般的编译器还是会选择不要让 z 跨越两个 int
    // 因为如果跨越 int 存储, 会导致访问效率降低, 性能下降

    int z : 16;
    int   : 15; // 手动添加 padding, 不需要名字

    // 这里有 1 bit 的 padding, 因为无论如何都要对齐到 int
    int w;
};
```

当然, bit-field 也支持类型混用, 即不一定要是同一种整数类型, 但是要求整数的位宽相同, 否则会先把前面的类型 padding 到整数位宽, 然后再放入后面的类型.

```cpp
struct bit_pack_2 {
    int x       : 16; // 16 bit
    unsigned y  : 16; // OK, 和 x 在同一个 int 中
    int      z  : 8;
    // 这里有 24 bit 的 padding,
    // 因为 uint8_t 只有 8 bit, 和 int 不一样
    // 因此, 前一个 int 会先被 padding 到 32 bit
    // 再放入 uint8_t
    uint8_t w : 3;
};
```

说到这里, 就不得不提 C++ 中的 `<bit>` 这个头文件了. 这个头文件是 C++ 20 新增的, 其提供了一些 bit 操作的函数, 如 `std::bit_cast`, `std::rotl`, `std::rotr`, `std::countr_zero`, `std::countr_one` 等等. 这些函数可以帮助我们更加方便地进行 bit 操作. 基本上, 你能想到的 bit 操作, 这个头文件都有.

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
        if constexpr (sizeof...(args) != 0)
            ((std::cerr << args), ...) << std::endl;
    }
};

template <typename _Tp, typename... _Args>
assert(_Tp &&, _Args &&...) -> assert<_Tp, _Args...>;

void test() {
    assert(0.1 + 0.2 == 0.3, "Hello, World!");
}
```

为了避免使用危险的宏, 我们最后只能选择了这种扭曲的方式实现了我们的 `assert`. 但是, 这种方式也有一些优点. 首先, 我们可以自定义输出信息, 而且只有在错误时才会生成输出字符串, 而不会有性能开销. 如果你觉得太丑了, 你甚至可以借助 `format` 来实现更加优雅的格式化输出. 其自由度还是非常高的, 比起 C 原生的 `assert`. 最后, 附上一个使用了 `format` 的实现:

```cpp
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

void test() {
    assert(false);
    assert(1 + 1 == 3, "wtf {} {}", 1 + 1, 3);
    assert(0.1 + 0.2 == 0.3, "Hello, World!");
}
```
