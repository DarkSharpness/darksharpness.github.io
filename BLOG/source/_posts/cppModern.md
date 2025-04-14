---
title: Effective Modern C++
date: 2025-04-07 09:42:39
tags: [C++, 基础知识]
categories: [C++, 基础知识]
keywords: [C++, Modern C++, 基础知识]
cover: https://s3.bmp.ovh/imgs/2025/04/07/52e46b1c151e1b77.jpg
mathjax: false
description: 被人拉着去讲 Modern C++ 了, 故作此文.
---

难绷的被下一届的同学拉去给下下届讲 Modern C++ (不过笔者咕咕了). 想了半天, 最后还是决定讲讲 modern effective C++, 也算是笔者的启蒙读本了... 那也是 23 年春节的回忆, 转眼就过去两年了, 这两年笔者又学了些什么呢...

anyway, 不说废话了, 管理每小节开头给出所有用到的 `cppreference` 或者其他相关链接.

一些笔者平时看的链接: [cppreference](https://en.cppreference.com/), [rust-lang](https://www.rust-lang.org/), [cppweekly 群友版](https://wanghenshui.github.io/cppweeklynews/)

笔者写 C++ 的核心要义就这些:

1. best effort 尽力优化性能, 但是不要试图认为自己比编译器聪明. 养成习惯就是随手的事情了.
2. 做好抽象和封装, 把一些内部的逻辑/可能是不安全的操作封装起来, 对外只暴露必要的接口 API, 不要留任何可能带来问题的后门, 这些都是给自己埋雷...
3. 尽量做好解耦, 每个模块只干自己的事情. 最好的办法就是模块内的变量/函数不要对外公开, 多分函数/作用域尽量减少变量名泄露的到处都是, 多用 lambda 函数且只捕获需要的.
4. 做好防御式编程, 可以开一个只在 debug 模式验证的宏, 做好边界检查.
5. ~~永远用最新的 C++ 标准~~

## 从值类型到类型推导

参考内容 [Effective Modern C++](https://cntransgroup.github.io/EffectiveModernCppChinese/1.DeducingTypes/item1.html) Item 1, 23, 24. 虽然本文基本都是笔者口胡.

### 左值和右值

[Value Category](https://en.cppreference.com/w/cpp/language/value_category)

> Remark: 请牢记: C++ 移动在语义希望达成的是, 所有权的转移. 推荐读者在实践中, 要么是 fallback 到 copy, 要么就是移走资源.

学完程序设计课之后, 大部分人都知道了 C++11 出了左值和右值这个东西, 但大概率是没有分太清楚的. 这里会简单介绍一下.

首先, 笔者假定读者能区分一个表达式的返回类型是不是值类型, 比如字面量 (除了字符串), 整数加减法 `a + b`, 或者返回类型是不含引用 `&` 的函数, 又或者是 `static_cast` 为值类型, 这些表达式返回的一定是值类型.

区分不同的值类型, 一个最简单的规则是, **有名字的东西**一定是左值引用 `lvalue`, `std::move` 几乎必须作用于一个有名字的变量, 返回的是右值引用 `xvalue`, 而其他的返回值的表达式, 返回的是纯右值 `prvalue`, 函数结果则完全视其返回类型. 我们一般把后两者统称 `rvalue`, 前两者统称 `glvalue`. 这三个东西有什么区别呢? 如果你学过 rust, 你就知道有一个概念叫做生命周期, 而这里可以用生命周期来不严谨的解释一下: 对于 `lvalue` 和 `xvalue`, 他们都不涉及生命周期的相关的事情, 但对于 `prvalue`, 在整个表达式被执行结束之后, 它返回的值类型需要被析构.

```cpp
#include <string>

int &func();
int test();

void test(int x, int &&y) {
    int(x); // as prvalue
    x;      // as lvalue
    func(); // as lvalue
    test(); // as prvalue
    x + x;  // as prvalue (but custom operator may not follow this)
    static_cast<long>(x);   // as value
    std::move(x);           // as xvalue
    static_cast<int &&>(x); // same as above
    static_cast<int &&>(y); // as xvalue
    y;                      // as left reference (because named as `y`)
}

std::string str();
void func(std::string);
std::string &&strange_func();
void strange_func2(std::string &&);

void lifespan() {
    {
        std::string x;
    } // x will call ~string(); after it dies.

    func(str());
    // the code above is very similar to the code below
    {
        auto tmp = str();
        func(static_cast<std::string &&>(tmp));
    } // after evaluation of func, the temporary value returned will be destructed
    strange_func2(t); // similar to above, the temporary will be binded and then destroyed

    func(strange_func()); // this will construct an std::string, then destroy after func
    strange_func2(strange_func()); // this will not call any ~string(); no lifespan is ended
}
```

特别注意的是, 右值引用的变量, 它看起来是一个 `rvalue` 之类的东西, 但根据我们的 "有名字" 原则, 它返回的其实是一个 `lvalue`. 事实上, 所谓的右值引用, 和左值引用, 他们起到的只是影响重载决议的作用. 两者都是引用, 实现上几乎 100% 是由指针实现 (事实上, 笔者一直把引用当作一种保证非空的指针的语法糖). 换句话说, 只有在编译期选择对应的函数的时候, 这两个东西才会起到区分的作用. 只是一般来说, 大家会认为接收到右值引用的时候, 传入的对象即将被析构, 所以可以 "移走" 它的资源, 故称作 `move`.

所以, 读者想必可以理解为什么 `std::move` 几乎一定作用在一个有名字的变量上, 它的作用就是告诉编译器, 请选择右值引用相关的重载. 实际上, 他就等于 `static_cast<T &&>` 强行转化, 算是标准库提供的语法糖. **一般来说**, 通过右值进行的移动构造, 因为允许从传入的对象中接管资源 (e.g. `std::vector` 的内部数据的指针, `std::shared_ptr` 的指针), 往往会比 copy 更加高效. (当然, 这也只是君子协议, 如果用户没有实现移动构造, 或者故意在移动构造里面 `while(1)` 卡住, 那也是没有办法的事, 毕竟编译器只负责帮你选择应该调用的函数.)

**特别需要注意的是**, `std::move` 无法改变 `const` 属性, 如果你希望通过 `move` 收益, 请确保入参没有 `const`.

```cpp
// A sample implementation of std::move
template<typename _Tp>
constexpr typename std::remove_reference<_Tp>::type&&
move(_Tp&& __t) noexcept
{ return static_cast<typename std::remove_reference<_Tp>::type&&>(__t); }

struct MyClass {
public:
    // good, always need to move since named parameter is `lvalue`
    MyClass(std::string a, std::string &b, std::string &&c):
        m_a(std::move(a)), m_b(std::move(b)), m_c(std::move(c)) {}

    // bad, move a const will not work. `const &&` is completely useless! never use it!
    MyClass(const std::string a, const  std::string &b, const std::string &&c):
        m_a(std::move(a)), m_b(std::move(b)), m_c(std::move(c)) {}

private:
    std::string m_a;
    std::string m_b;
    std::string m_c;
};
```

在铺垫了这么多值类型之后, 可以来讲讲模板的类型推导了. 在这里, 笔者将忽略 `volatile` 这个病态的东西.

### 推导的类型是一个引用

[Item 1](https://cntransgroup.github.io/EffectiveModernCppChinese/1.DeducingTypes/item1.html)

```cpp
template <typename T>
void func(T &);
template <typename T>
void cfunc(const T &);

int &ref();

int x = 0;
int &y = x;
const int z = x;
const int &w = y;

func(x); // [T = int]
func(y); // [T = int]
func(z); // [T = const int]
func(w); // [T = const int]
func(ref()); // [T = int]

// func(1); // prvalue not acceptable
// func(std::move(x)); // xvalue not acceptable

cfunc(x); // [T = int]
cfunc(y); // [T = int]
cfunc(z); // [T = int]
cfunc(w); // [T = int]
cfunc(ref()); // [T = int]
cfunc(1);     // [T = int], the integer will be destroyed after func returns, though do nothing.
cfunc(std::move(x)); // [T = int]
```

这个 case 相对比较简单, `func` 要求传入的值一定是一个 `lvalue`, `T` 会保留传入参数是否是 `const`, 并且是不含引用的值类型. 而 `cfunc` 则比较特殊, 它可以接受任何类型的值, `T` 会被推导为不含 `const` 和引用的类型. 读者或许可以联想到上学期讲到的 `const &` 可以绑定一切并延长 `prvalue` 的生命周期, 道理的确是类似的.

### 推导的类型是一个指针

[Item 1](https://cntransgroup.github.io/EffectiveModernCppChinese/1.DeducingTypes/item1.html)

```cpp
template <typename T>
void func(T *);
template <typename T>
void cfunc(const T *);

int x = 0;
const int &y = x;
func(&x); // [T = int]
func(&y); // [T = const int]
cfunc(&x); // [T = int]
cfunc(&y); // [T = int]
```

规则几乎和引用完全一样. 唯一特殊的是, 入参必须是指针类型, 并且不存在 `const *` 可以延长生命周期之类的, 这是因为我们不存在左右指针这回事.

特别注意的是, 只有 `lvalue` 可以被 take address, `xvalue` 和 `prvalue` (即 `rvalue`) 是不行的. 举例:

```cpp
int x = 0;
int &&f();
int g();
// all the code below is illegal
&(f());
&(g());
&(f() + g());
&(static_cast<int &&>(x))
```

### 推导的类型是一个通用引用

[Item 1](https://cntransgroup.github.io/EffectiveModernCppChinese/1.DeducingTypes/item1.html)

```cpp
template <typename T>
void func(T &&param);
```

这是一个非常特殊的 case, 要理解这个 case, 我们首先要了解万能引用, 以及理解 `std::forward`, 以及引用折叠.

考虑以下的引用场景: 你为用户提供了一个包装函数, 你可能会把用户的入参传给内部调用的另一个函数. 同时, 你希望有更好的泛化性, 用的是模板. 自然, 用户有的时候传入的是左值, 有的时候是右值, 你希望把这个左还是右的信息原封不动的传给内部调用的那个函数. 这时候, 就需要用到 `std::forward`.

简单来说, `std::forward` 功能是, 如果传入的是 `lvalue reference`, 那么就返回一个 `lvalue`; 如果传入的是 `rvalue reference`, 那就返回一个 `xvalue`, 此时类似 `std::move`. (需要注意, 入参不做任何操作的话, 一定是被当作 `lvalue` 的, 因为它 "有名字")

而入参同时接受左/右值, 这就需要用到 **通用引用**, 或者说 **万能引用**. 在上述代码中, 如果传入的是一个 `lvalue` 类型比如 `int &`, 那么 `T` 会被推导为 `int &`, 同时 `param` 的类型应当是 `int & &&`. 这看起来很奇怪, 因为这里需要 **引用折叠** 的概念. 简单来说, 在推导的过程中, 这里 `param` 的类型会被折叠为 `int &`. 而如果传入的是一个 `rvalue` 类型比如 `int &&` 或者 `int`, 那么 `T` 会被推导为 `int` 本身, `param` 就是 `int &&` 类型, 没有什么歧义.

那 `std::forward` 是怎么工作的呢, 我们需要在 `func` 内部调用 `std::forward<T>`. 当 `T` 是引用类型的时候, 他会返回一个 `lvalue`; 反之, 则类似 `std::move` 返回一个 `xvalue`. 这其实就对应了入参推导 `T` 的两种 case. 参考实现如下:

```cpp
template<typename _Tp>
constexpr _Tp&&
forward(typename std::remove_reference<_Tp>::type& __t) noexcept
{ return static_cast<_Tp&&>(__t); }

template<typename _Tp>
constexpr _Tp&&
forward(typename std::remove_reference<_Tp>::type&& __t) noexcept
{ return static_cast<_Tp&&>(__t); }
```

现在来看看 Modern Effective C++ Item 1 具体的例子:

```cpp
template<typename T>
void f(T&& param);
int x = 27;
const int cx = x;
const int &rx = cx;
f(x);   // [T = int &, T&& = int &]
f(cx);  // [T = const int &, T&& = const int &]
f(rx);  // [T = const int &, T&& = const int &]
f(27);  // [T = int, T&& = int &&]
```

### 推导的类型是一个值类型

[Item 1](https://cntransgroup.github.io/EffectiveModernCppChinese/1.DeducingTypes/item1.html)

```cpp
template<typename T>
void func(T);

int x = 0;
int &y = x;
const int z = x;
const int &w = y;
func(x); // [T = int]
func(y); // [T = int]
func(z); // [T = int]
func(w); // [T = int]
```

这个非常特别, 这意味着无论入参是什么, 如果是推导的话一定会拷贝一份新的对象, 并且 `T` 类型推导出来不会含有引用和 `const`. (当然, 用户也可以强行指定模板的类型 `T` 为含有引用的类型).

需要注意, 这里推导出来不会含有 `const` 指的是值类型本身, 如果传入的类型是 `const int *` 之类的 `const` 指针, 指向内容的 `const` 性自然是不能变的.

### 边角料

[Item 1](https://cntransgroup.github.io/EffectiveModernCppChinese/1.DeducingTypes/item1.html)

比较恶心的是数组实参和函数实参.

在推导值类型或者指针类型的时候, 数组会退化为指针, 函数同理. 在推导引用相关类型的时候, 数组会被推导为特殊的数组引用, 函数同理.

### auto 推导

[Item 1](https://cntransgroup.github.io/EffectiveModernCppChinese/1.DeducingTypes/item1.html)

`auto` 作为 C++11 的一大亮点, 自然是不会拉下的. `auto` 的类型推导规则几乎和函数一致, 例如 `auto` 对应的是推导值类型, `auto &` 和 `const auto &` 是推导一般的引用类型, 而 `auto &&` 则是万能引用推导. 特别地, 相信大家也在程序设计课上了解过, `const auto &` 可以接受一个 `rvalue` 对象. 更加特别地, 如果 `const auto &` 或者 `auto &&` 绑定的是一个 `prvalue`, 那么它可以延长这个 `prvalue` 的生命周期, 直到 `auto` 的这个变量离开作用域之后, 才析构. 对于 `xvalue`, 由于其并非返回一个临时的值, 编译器不会去管它的生命周期, 因此也不存在延长不延长一说.

比较恶心的是, `auto` 如果用初始化列表初始化, 会有一些奇怪的行为. 对于一个函数模板推导, 传入一个初始化列表一样的东西比如 `{1, 2, 3}` 是无法工作的. 但对于 `auto` 声明变量, 这个是允许的.

```cpp
auto x = {1, 2, 3}; // deduced as std::initializer_list<int>
auto y = {1, 2.0};  // failure in deduction
auto z {1};         // deduced as int
```

特别地, C++14 以后也允许了 `auto` 作为返回类型 (即不含尾置的返回类型), C++11 lambda 函数不写返回类型的话也是默认以 `auto` 返回, 此时会按照类似的规则进行推导. 如果是返回的是 `auto &` 则同理. lambda 函数的参数中的 `auto` 和 C++20 以后函数参数中的 `auto` 也同理.

```cpp
auto func(int &x, int &y) -> auto & {
    if (x > y)
        return x;
    else
        return y;
}
[](const auto &x) { return x; } (1); // lambda, auto deduced as `int`, return `int`.
```

## 语言特性

### inline 和 static

[inline](https://en.cppreference.com/w/cpp/language/inline), [static](https://en.cppreference.com/w/cpp/language/storage_duration#Static_block_variables)

`inline` 和 `static` 都属于是语言中存在很久的关键词了, 早在 C 里面就已经存在. 然而, 很多人对这两个关键词存在一定的误区.

`inline` 关键词 在 C++ 中和所谓的内联优化可以说没有一点关系. 这么说可能比较绝对, 但是为了便于读者区分, 建议读者也这么来理解. `inline` 的作用是告诉编译器, 这个符号允许被多次定义, 即在多个编译单元中出现.

这里首先要铺垫一下, 编译单元是什么. 在传统的算法竞赛题里面, 只有一个 `main.cpp`, 那么编译单元就只有这一个 `main.cpp`. 其他的文件都是被 `#include` 加进来的, 而众所周知, `#include` 其实就是文本替换, 把代码里面的文本复制了进来. 而一般大一点的 C++ 项目, 我们往往会看到一个 `CMakeLists.txt`, 其中经常会列出若干 `cpp` 文件, 例如 `src/1.cpp, src/2.cpp`. 这时候, 其中每个 `.cpp` 都是一个独立的编译单元, 在处理不同的单元的时候, 编译器可以并行编译, 这样在一个多核心的服务器上并行编译可以大大的减少编译的时间. 在多文件编译的时候, 往往编译器先会编译到 `.o` 文件, 然后把多个编译单元生成的多个 `.o` 文件链接为一个二进制可执行文件例如 `.exe`, `.out`. 在这个过程中, 每一个全局变量/函数都会生成一个符号, 其他的编译单元如果调用了一个声明的符号, 需要在链接期间找到符号对应的变量/函数的地址.

如果多个编译单元都看到了某个函数的声明和定义, 那么在编译到 `.o` 的过程中, 这些单元都会把这个函数的符号记下来, 读者可以认为是每个单元都维护了一个符号表 `map`, 而 `map` 里面 `key` 为这个函数名字的一项记录了这个函数的地址 (这是一个不严谨的说法, 请不要细究细节). 而在链接的阶段, 不同的编译单元的符号表需要合并, 但如果合并的时候发现某一个 `key` 有两个对应的记录, 那么就会报错. 事实上, C/C++ 要求最后所有编译单元的结果中, 每个符号 (包括全局变量/函数) 只有一处定义, 这也就是所谓的 **One Definition Rule** (ODR), 即一个函数只能有一个定义.

然而, 很多时候, 对于一些简单的函数, 比如 `int add_1(int x) { return x + 1; }`, 我们想把它放到头文件里面, 而不是某个 `.cpp` 里面. 一般情况下, 当多个编译单元包含了这个文件的时候, 这会违反 ODR. 这时候, 我们就需要用到 `inline` 关键词. `inline` 关键词的作用是, 在一个符号在多个编译单元里面出现时, 编译器随机保留其中的一份, 丢弃其他的. 因为多个编译单元中包括的是同一个头文件, 看到的也是同一个函数的实现 (比如上述例子中的 `add_1`), 因此保留哪一份不会影响正确性. 特别地, C++ 默认类内提供实现的成员函数都是 `inline` 的, ~~所以大家多用面向对象吧~~.

```cpp
struct MyStructTest {
    void func() {} // default set as inline, safe to be included in a header.
    void func2();
};

// if in .h/.hpp, need to manually add `inline` here, because definition is out of the class
// if in .cpp, we shouldn't add inline here!
inline void MyStructTest::func2() {}
```

`static` 修饰一个 `class` 成员函数/变量比较特殊, 这里讨论的是 `static` 修饰一个全局变量/函数. `static` 要求修饰的这个符号变成内部符号, 即最终这个符号不会对外暴露, 当前编译单元内所有用到这个符号的地方, 都会变成对内部符号的调用. 换句话说, 它不会在最终 `.o` 里面的符号表里面出现. 因此, 别的编译单元无论如何都无法直接调用这一个函数.

总结一下, `inline` 是允许多个定义, 编译器保留其中任意一份, 而 `static` 是让符号变成内部符号, 类似 private, 不再对外暴露. 对于在头文件中提供了定义的全局函数, 笔者建议使用 `inline`. 对于没有在 `.h` 中声明, 仅仅用于当前编译单元 (`.cpp`) 的一些内部辅助函数, 笔者建议使用 `static`. 当然, 同样的概念不仅仅适用于函数, 同样也适用于变量 (需要 C++17). 以下是一些样例代码.

```cpp
// test.h
#pragma once

namespace dark {

void func();
inline void short_func() {};

// declare and define constexpr value in header
// highly recommended in C++, maybe best practice
inline constexpr int kZero = 0;

struct Test {
    // this static just means not a member variable,
    // but shared by the class `Test`
    inline static constexpr int kon = 1;
};

} // namespace dark
```

```cpp
// test.cpp
#include "test.h"
namespace dark {

static void internal_helper() {}

// do not add static or inline here
void func() {
    short_func();
    internal_helper();
}

} // namespace dark
```

```cpp
// main.cpp
#include "test.h"
int main() {
    dark::func();
    dark::short_func();
}
```

感兴趣的读者可以再自行去了解一下匿名 `namespace` 的概念, 详情请参考 [cppreference](https://en.cppreference.com/w/cpp/language/namespace). 简单来说, 它让其中的所有符号都变成 `static` 的, 非常适合放在 `.cpp` 文件中.

```cpp
namespace {
void no_need_to_mark_static() {}
}
```

### using

[using](https://en.cppreference.com/w/cpp/keyword/using)

一些笔者想到的常用功能:

1. `using enum`: C++20 引入的, 可以把 `enum` 的作用域引入当前作用域, 非常适用于 `switch` 内部.
2. `using namespace xxx`: 不推荐, 除非是 `using std::literals::chrono_literals` 来引入 `s` 这类用户定义字面量.
3. `using ns::xxx`: 引入 namespace `ns` 中的 `xxx`, 适用于一个封闭作用域内部 (比如函数).
4. `using A = B`: 非常常见, 请全面禁用 `typedef`, 我们不应该兼容 `C++11` 之前的代码.

### 结构化绑定

[binding](https://en.cppreference.com/w/cpp/language/structured_binding), [ADL](https://en.cppreference.com/w/cpp/language/adl)

在 C++17 中, 引入了结构化绑定: 对于一个聚合类 (即没有基类, 没有用户声明的构造函数) 或者按照 [特殊规则](https://en.cppreference.com/w/cpp/language/structured_binding) 重载了对应的函数的类, 我们可以类似 python 中 `tuple` 解包的形式写代码. 比如 `std::pair`, `std::tuple`, `std::array` 以及原生的数组都是支持的. 语法如下, 需要注意的是必须用 `auto`:

```cpp
std::tuple<int, float, std::string> t;
auto &[x, y, z] = t;
struct MyStruct {
    int a;
    int b;
} tmp;
auto [a, b] = tmp; // must use auto
```

如果你对于自定义的非聚合类也想使用结构化绑定, 那么你需要提供一个 `get` 函数, 并且特化 `std::tuple_size` 和 `std::tuple_element` 两个模板. 例如:

```cpp
struct A {
    int x;
    double y;
    int other;
    // this violate the definition of aggregate class
    A(int x, double y) : x(x), y(y), other(0) {}
};
template <>
struct std::tuple_size<A> : std::integral_constant<std::size_t, 2> {
    // now, std::tuple_size<A>::value is 2
};

template <std::size_t I>
auto get(A &a) -> decltype(auto) {
    if constexpr (I == 0) {
        return a.x;
    } else if constexpr (I == 1) {
        return a.y;
    }
}

template <std::size_t I>
auto get(const A &a) -> decltype(auto) {
    if constexpr (I == 0) {
        return a.x;
    } else if constexpr (I == 1) {
        return a.y;
    }
}

template <std::size_t I>
struct std::tuple_element<I, A> {
    using type = decltype(get<I>(std::declval<A>()));
};

auto main() -> int {
    A a{1, 1.0};
    auto &[x, y] = a;
    std::cout << x << " " << y << std::endl;
}
```

这里你可能会好奇了: 编译器为什么知道调用的是哪个 `get` 函数? 为什么不是 `std::get`? 这里就涉及 C++ 中 Argument Dependent Lookup (ADL) 的知识了. 考虑到 ADL 对于一般读者还是过于抽象和晦涩, 这里笔者只给出一个 [cppref 链接](https://en.cppreference.com/w/cpp/language/adl), 笔者也是在 23 年暑假花了整整一个暑假才嚼明白, 并且在漫长的实践中才真正理解.

### if/switch

[if](https://en.cppreference.com/w/cpp/language/if), [switch](https://en.cppreference.com/w/cpp/language/switch)

在 C++17 中, 引入了 `if` 初始化语句. 简单来说, 你可以在判断一个条件的同时把结果放在 `if` 内部 (注意, 条件判断不一定返回的是 `bool`, 只要可以 `static_cast<bool>` 即可), 或者为 `if` 额外的添加初始化语句, 从而避免变量名泄露, 把变量的生命周期限制在 `if` 语句内部. 这对 `switch` 也是类似的.

```cpp
auto foo() -> std::shared_ptr<int>;

// before
std::shared_ptr<int> ptr = foo();
if (ptr) {
    // do something A
} else {
    // do something B
}
if (ptr != nullptr) {
    // yet another way
}
```

```cpp
// after
if (std::shared_ptr<int> ptr = foo()) {
    // do something A
} else {
    // do something B
    // note that ptr is still reachable in this branch.
}
// ptr dies here.

if (int x = 0, y = 2; std::shared_ptr<int> ptr = foo()) {
    // yet another way
}

if (std::shared_ptr<int> x = foo(); x && *x > 0) {
    // this is also ok
} else if (std::shared_ptr<int> y = foo(); x && y && *x == *y) {
    // also works for else if
}
```

这个设计最大的好处是, 可以干净的, 只为 `if` 所在的作用域声明变量, 避免变量名泄露, 严格控制变量的生命周期. 笔者认为这是现代语言必须的一个特点, 即闭包化, 每个模块尽量解耦, 严格限制模块之间潜在的耦合, 进而写出更高质量的代码.

### constexpr/consteval/constinit

在 C++20 中, 推出了两个有趣的关键词: `consteval`和 `constinit`. 前者要求函数必须在编译期求值, 后者要求变量必须在编译期初始化.

### constexpr

[constexpr](https://en.cppreference.com/w/cpp/keyword/constexpr)

`constexpr` 是 C++11 引入的, 当修饰变量的时候, 要求变量在编译期初始化, 当修饰函数的时候, 表示函数允许在编译期求值. 这些你应该都在课上已经了解了. 在逐渐发展的过程中, `constexpr` 也在变得更强. 在 C++20 中, `constexpr` 修饰的函数甚至允许动态分配内存, 仅仅要求在编译期确定大小以及释放 (虽然这是编译器对 `std::allocator` 开洞...).

```cpp
constexpr auto find_the_kth_prime(int k) -> int {
    auto vec      = std::vector<int>{2, 3, 5, 7, 11};
    auto is_prime = [&vec](int x) -> bool {
        for (const auto &p : vec)
            if (x % p == 0)
                return false;
        return true;
    };
    int num = vec.back() + 2;
    while (vec.size() < static_cast<std::size_t>(k)) {
        if (is_prime(num))
            vec.push_back(num);
        num += 2;
    }
    // vec is dynamically allocated and freed in compile time
    // so it's ok to use as a temporary variable
    return vec[k - 1];
}
// ok
constexpr auto val = find_the_kth_prime(100);
```

事实上, 传统很多使用模板元编程的奇技淫巧基本都可以用 `constexpr` 来替换了, 这种写法可读性更高, 也更加直观.

### consteval

[consteval](https://en.cppreference.com/w/cpp/keyword/consteval)

`consteval` 的核心在于: 函数必须在编译期求值. 例如:

```cpp
consteval auto must_be_0(int x) -> int {
    if (x != 0)
        throw;
    return x;
}
```

在上述函数中, 如果你的入参不是 `0`, 那么编译器会报错 (因为 throw 无法在编译期执行), 代码无法通过编译. 这有两点:

1. 强制某些函数的计算在编译期完成. 虽然 C++ 编译器有着非常强大的优化能力, 但是对于极其复杂的函数, 并不保证能够优化出来, 即使被标上了 `constexpr`, 只要不是赋值给 `constexpr` 的变量, 编译器也不会强制在编译期求值 (虽然大多数情况下会). 这时候, `consteval` 就派上用场了.
2. 给编译期间的错误提供了更多的可能. 例如上述代码, 如果你传入了一个非 0 的值, 编译器会报错. 我们因此可以实现类似功能: 只要当函数的输入类型/参数满足特定条件的时候, 才能通过编译 (参考 `std::format`).

需要注意的是, `consteval` 和重载决议无关. 编译器在选择了正确的函数之后, 如果该函数不满足 `consteval` 的要求, 编译器会报错. 你可能会联想到 SFINAE, 但是遗憾的是, SFINAE 只会影响重载的选择, 而 `consteval` 是在重载选择之后才会起作用的, 因此两者并无关系.

### constinit

[constinit](https://en.cppreference.com/w/cpp/keyword/constinit)

有的人可能认为 `constinit` 就是 `constexpr`, 但是这是错误的. `constinit` 是要求变量在编译期初始化, 变量本身可以是非 `const` 的, 而 `constexpr` 则暗含了 1. 变量是 `const` 的, 2. 变量在编译期初始化. 例如:

```cpp
constinit int x = 0;
int main() {
    x = 1; // ok
}
```

上述代码是合法的, 因为 `x` 是 `constinit`, 而不是 `const`. 但是如果你把 `constinit` 换成 `constexpr`, 那么编译器会报错. 好奇的你可能想问: 那这个关键词有什么用呢? 事实上, 它是为了解决一个问题而生的: 静态变量的初始化顺序问题.

在 C++ 中, 静态变量(你可以理解为全局变量)的初始化顺序是不确定的, 如果你有两个静态变量, 他们之间有依赖关系, 那么你可能会遇到问题. 例如:

```cpp
constexpr auto f() -> int { return 0; }
int y = f();
int x = y;
```

这个初始化顺序是可能不确定的 (虽然实测几乎没出过错, 可能主要还是在跨编译单元的时候, 容易出问题), 有可能 `x` 先初始化, 也有可能 `y` 先初始化. 这时候, 你可以使用 `constinit` 来解决这个问题:

```cpp
consteval auto f() -> int { return 0; }
constinit int y = f();
int x = y;
```

同时, `constinit` 也可以支持 `extern` 的变量, 这是 `constexpr` 做不到的.

## memory-safe

Modern C++ 一个突出的特点是, 内存安全. 其中, RAII 给我们的实现提供了巨大的便利. 而 `memory safe` 的实现, 其靠的就是 smart pointer. 在大多数的情况下, 我们都应该用智能指针替代裸指针.

- `unique_ptr` 只能有一个拥有者的指针.
- `shared_ptr` 可以被多处拥有的指针, 需要注意防止循环引用.

说实话, 这些智能指针其实最大的便利不是访问的安全性, 用户依然可以随便就写出访问空指针的代码, 它们最大的好处还是, 实现了内存资源的自动回收. 这其实本质就是 RAII 思想的运用, 构造处获取资源, 析构处回收资源, 而移动语义则一般表示资源的转交 (比如 `unique_ptr`, 在移动构造/赋值之后, 被移动的对象会被重置为空指针). 这些先进的理念也被后来很多更加先进的编程语言所采纳, 比如 `rust`.

### unique_ptr

[Item 18](https://cntransgroup.github.io/EffectiveModernCppChinese/4.SmartPointers/item18.html), [unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr), [make_unique, make_unique_for_overwrite](https://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique)

主要截取自 item 18. 对于独占 (尤其是不可复制) 的资源, 我们会用 `unique_ptr` 来管理内存.

一般来说, 如果使用 `unique_ptr` 我们会更加倾向于用一个工厂函数来构建这个 `unique_ptr` 对象, 例如标准库提供的 `std::make_unique`. 当然, 如果你想支持一些更加复杂的功能, 特别是自定义删除器的时候, 很多时候需要自己手写, 可以用到 C++14 的 auto 返回类型 + lambda 函数来实现一些优雅的功能. 下面例子节选自 item 18.

```cpp
template <typename... Ts>
auto makeInvestment(Ts &&...params) { // need C++14 
    auto delInvmt = [](Investment *pInvestment) {
        // pInvestment pointer will not be nullptr.
        makeLogEntry(pInvestment);
        delete pInvestment;
    };
    std::unique_ptr<Investment, decltype(delInvmt)> pInv(nullptr, delInvmt);
    if (...) {
        pInv.reset(new Stock(std::forward<Ts>(params)...));
    } else if (...) {
        pInv.reset(new Bond(std::forward<Ts>(params)...));
    } else if (...) {
        pInv.reset(new RealEstate(std::forward<Ts>(params)...));
    }
    return pInv;
}
```

当然, 如果你不是特别关心效率, 但是非常关心泛化性, 你也可以用 `std::function<void(T *)>` 来作为删除器, 支持任意的仿函数作为删除器, 缺点是会增加存储的空间. 同时, `unique_ptr` 也可以很方便的转化为 `shared_ptr`, 提供了不少的便利性.

特别地, `unique_ptr` 允许指针转化为派生类或者基类的 `unique_ptr`, 不过这很容易带来内存泄露的问题, 析构的时候可能会找到错误的析构函数. 在笔者的日常使用中, `unique_ptr` 经常会搭配抽象基类 (`virtual class`), 虚类的 `virtual` 析构函数可以让 `unique_ptr` 在 destroy 指针指向的对象的时候找到正确析构, 避免资源泄露.

最后, 以 item 18 的 notes 总结一下:

1. `std::unique_ptr` 是轻量级、快速的、只可移动 `(move-only)` 的管理专有所有权语义资源的智能指针
2. 默认情况，资源销毁通过 `delete` 实现, 但是支持自定义删除器. 有状态的删除器和函数指针会增加 `std::unique_ptr` 对象的大小
3. 将 `std::unique_ptr` 转化为 `std::shared_ptr` 非常简单
4. `std::unique_ptr` 在大部分的使用情况, 性能和裸指针无异 (一句话, 我相信编译器优化).

> Remark: 一般来说, 笔者不鼓励使用传入一个 `new` 出来的指针构造 `unique_ptr`, 除非有特殊的删除器, 否则笔者推荐全部用 `std::make_unique` 来构造.

### shared_ptr

[Item 19](https://cntransgroup.github.io/EffectiveModernCppChinese/4.SmartPointers/item19.html), [shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr), [enable_shared_from_this](https://en.cppreference.com/w/cpp/memory/enable_shared_from_this), [make_shared, make_shared_for_overwrite](https://en.cppreference.com/w/cpp/memory/shared_ptr/make_shared)

比起 `unique_ptr`, `shared_ptr` 会更加灵活, 其通过了引用计数来控制了对象的生命周期, 允许高效的拷贝, 当然缺点就是可能成环导致资源泄露. 笔者并不经常使用 `shared_ptr`, 这里就不过多介绍了. 为了避免循环引用, 需要把可能的循环中的一部分设置为 `weak_ptr`, 具体用法请参考 cppreference.

`shared_ptr` 更加灵活的一点是, 它支持传入一个自定义的删除器作为构造参数的一部分, 而不需要像 `unique_ptr` 那样在模板里面显式指出来. 正因如此, 你可以放心的把派生类的 `shared_ptr` 转化为基类的 `shared_ptr`, 不用担心基类不是 `virtual` 可能会带来资源泄露, 这是因为 `shared_ptr` 在一开始已经把对应指针类型的删除函数给 "记下来了". 这自然是有开销的, 但是也能带来不小的便利.

特别地, 使用 `shared_ptr` 的时候, 一个常见的问题是把一个指针, 由两个 `shared_ptr` 来接管 (包括 `unique_ptr` 也会有这种问题), 或者错误的由 `this` 指针来构造一个 `shared_ptr` (这并不会正确的构造一个指向同一个引用计数块的 `shared_ptr`). 因此, 笔者认为无论如何, 在没有自定义删除器的情况下, 请尽一切可能使用 `std::make_shared` 来构造一个 `shared_ptr`. 同时, 对于前面提到的从 `this` 构造 `shared_ptr` 的例子, 正确的做法是继承一个 `std::enable_shared_from_this<T>`, 然后调用基类的 `shared_from_this` 函数. 下面的例子改变自 [cppreference](https://en.cppreference.com/w/cpp/memory/enable_shared_from_this):

```cpp
#include <iostream>
#include <memory>

class Good : public std::enable_shared_from_this<Good> {
public:
    std::shared_ptr<Good> getptr() {
        return shared_from_this();
    }
};

class Best : public std::enable_shared_from_this<Best> {
private:
    struct Private {};
    Best(Private) {}

public:
    // Everyone else has to use this factory function
    // Hence all Best objects will be contained in shared_ptr
    static std::shared_ptr<Best> create() {
        return std::make_shared<Best>(Private());
    }

    std::shared_ptr<Best> getptr() {
        return shared_from_this();
    }
};

struct Bad {
    std::shared_ptr<Bad> getptr() {
        return std::shared_ptr<Bad>(this);
    }
    ~Bad() {
        std::cout << "Bad::~Bad() called\n";
    }
};
```

如果以上这些说法都没法说服你用 `std::make_shared`, 那么笔者可以再告诉你一个有趣的小细节. `shared_ptr` 由两部分组成: 对象指针和控制块指针 (常见 `gcc` 和 `clang` 的实现, `sizeof(shared_ptr<T>)` 都是 `16` 即两个指针). 控制块比较特殊, 管理了对象的析构函数, 引用计数等一系列东西. 而如果你用 `std::make_shared`, 那么在一般的实现中 (比如 `gcc`), 它的控制块和对象会共用一大块存储, 而不需要申请两次空间. 试着运行一下以下的代码吧.

```cpp
#include <cstdio>
#include <memory>

// no inline, required by [replacement.functions]/3
void *operator new(std::size_t sz) {
    std::printf("1) new(size_t), size = %zu\n", sz);
    if (sz == 0)
        ++sz; // avoid std::malloc(0) which may return nullptr on success

    if (void *ptr = std::malloc(sz))
        return ptr;

    throw std::bad_alloc{}; // required by [new.delete.single]/3
}

// no inline, required by [replacement.functions]/3
void *operator new[](std::size_t sz) {
    std::printf("2) new[](size_t), size = %zu\n", sz);
    if (sz == 0)
        ++sz; // avoid std::malloc(0) which may return nullptr on success

    if (void *ptr = std::malloc(sz))
        return ptr;

    throw std::bad_alloc{}; // required by [new.delete.single]/3
}

void operator delete(void *ptr) noexcept {
    std::puts("3) delete(void*)");
    std::free(ptr);
}

void operator delete(void *ptr, std::size_t size) noexcept {
    std::printf("4) delete(void*, size_t), size = %zu\n", size);
    std::free(ptr);
}

void operator delete[](void *ptr) noexcept {
    std::puts("5) delete[](void* ptr)");
    std::free(ptr);
}

void operator delete[](void *ptr, std::size_t size) noexcept {
    std::printf("6) delete[](void*, size_t), size = %zu\n", size);
    std::free(ptr);
}

struct object {
    int a[10];
};

auto main() -> int {
    auto tmp = std::make_shared<object>();
    std::puts("7) make_shared");
    auto tmp2 = std::shared_ptr<object>(new object{});
    std::puts("8) shared_ptr");
}
```

总结一下, `shared_ptr` 是一个非常强大而方便的管理指针数据的工具, 它维护了指针的析构器和引用计数, 它的引用计数甚至是线程安全的. 自然, 这会带来一些不必要的开销, 但是在很多时候, 灵活性带来的好处远胜于一些微不足道的开销带来的弊端.

一个非常非常非常常见的 use case 是 `pimpl`. 具体来说, 我们在对象中维护了一个 `std::shared_ptr<Impl>`, 但是 `Impl` 只有声明 (即 `struct Impl`), 而实现处放在了 `.cpp` 文件中而不是在 `.h`. 这是因为头文件在编译的时候, `#include` 的内容会被替换到文件里面, 而一个类的实现可能会包含很多其他的依赖 (比如标准库里面的 `vector, unordered_set`, 或者是第三方库的一些代码). 如果我们把类的实现 (注意, 是类的实现, 而不仅仅是类的成员的实现) 放在了 `.h` 里面, 那么一旦你的类的结构发生了任何改变 (比如添加了一个函数, 或者删除了一个成员), 编译的时候, 所有依赖这个 `.h` 的 `.cpp` 文件都需要重新编译. 在一个庞大的项目里面, 这会极大地拖垮编译速度, 可能会慢到无法接受, 1 ~ 2 个小时都是有可能的. 这时候, 把类的结构完全分离到 `.cpp` 中 (包括内部成员和所有成员函数), 只对外暴露一些必要的接口或者说 API, 不仅可以极大地提升编译速度, 还能强制把接口和实现分离, 提高了代码的可读性.

这个问题的解决方案有一个是前向声明一个不完整类型 `struct Impl;`, 而在传参的时候尽量用引用类型比如 `const &` (虽然指针也行, 但是 Modern C++ 不提倡指针) (这里传值需要看到类型的完整定义). 同时, 对于一个类型的对象, 总需要有一个持有者吧. 这时候 `shared_ptr` 就可以作为这个持有者. 下面是著名开源项目 [xgrammar](https://github.com/mlc-ai/xgrammar) 里面的一些代码:

```cpp
/*!
 *  Copyright (c) 2024 by Contributors
 * \file xgrammar/object.h
 * \brief Utilities for creating objects.
 */

#ifndef XGRAMMAR_OBJECT_H_
#define XGRAMMAR_OBJECT_H_

#include <memory>   // IWYU pragma: keep
#include <utility>  // IWYU pragma: keep

namespace xgrammar {

/*!
 * \brief A tag type for empty constructor.
 *
 * Since XGRAMMAR_DEFINE_PIMPL_METHODS already occupies the default constructor to
 * construct a null object, this tag is used to define an empty constructor for
 * the object.
 */
struct EmptyConstructorTag {};

#define XGRAMMAR_DEFINE_PIMPL_METHODS(TypeName)                                \
 public:                                                                       \
  class Impl;                                                                  \
  /* The default constructor constructs a null object. Note operating on a */  \
  /* null object will fail. */                                                 \
  explicit TypeName() : pimpl_(nullptr) {}                                     \
  /* Construct object with a shared pointer to impl. The object just stores */ \
  /* a pointer. */                                                             \
  explicit TypeName(std::shared_ptr<Impl> pimpl) : pimpl_(std::move(pimpl)) {} \
  TypeName(const TypeName& other) = default;                                   \
  TypeName(TypeName&& other) noexcept = default;                               \
  TypeName& operator=(const TypeName& other) = default;                        \
  TypeName& operator=(TypeName&& other) noexcept = default;                    \
  /* Access the impl pointer. Useful in implementation. */                     \
  Impl* operator->() { return pimpl_.get(); }                                  \
  const Impl* operator->() const { return pimpl_.get(); }                      \
                                                                               \
 private:                                                                      \
  std::shared_ptr<Impl> pimpl_

}  // namespace xgrammar

#endif  // XGRAMMAR_OBJECT_H_
```

```cpp
/*!
 *  Copyright (c) 2024 by Contributors
 * \file xgrammar/grammar.h
 * \brief The header for the definition and construction of BNF grammar.
 */

#ifndef XGRAMMAR_GRAMMAR_H_
#define XGRAMMAR_GRAMMAR_H_

#include <xgrammar/object.h>

#include <optional>
#include <string>
#include <vector>

namespace xgrammar {

struct StructuralTagItem;

class Grammar {
 public:
  std::string ToString() const;
  static Grammar FromEBNF(
      const std::string& ebnf_string, const std::string& root_rule_name = "root"
  );
  static Grammar FromJSONSchema(
      const std::string& schema,
      bool any_whitespace = true,
      std::optional<int> indent = std::nullopt,
      std::optional<std::pair<std::string, std::string>> separators = std::nullopt,
      bool strict_mode = true,
      bool print_converted_ebnf = false
  );
  static Grammar FromRegex(const std::string& regex, bool print_converted_ebnf = false);
  static Grammar FromStructuralTag(
      const std::vector<StructuralTagItem>& tags, const std::vector<std::string>& triggers
  );
  static Grammar BuiltinJSONGrammar();
  static Grammar Union(const std::vector<Grammar>& grammars);
  static Grammar Concat(const std::vector<Grammar>& grammars);
  friend std::ostream& operator<<(std::ostream& os, const Grammar& grammar);

  XGRAMMAR_DEFINE_PIMPL_METHODS(Grammar);
};

}  // namespace xgrammar

#endif  // XGRAMMAR_GRAMMAR_H_
```

### smart pointer 的遗憾

smart pointer 在笔者看来, 已经极大地解决了 `memory safe` 的问题. 但它并没有解决一个信任链的问题. 那就是, 这个指针到底会不会是一个空指针. 无论是 `shared_ptr` 还是 `unique_ptr`, 都有一个 "指向空" 的默认状态, 这就导致用户永远可能担心: 这个指针是不是空啊?

诚然, 这样似乎显得有些做作, 但这在一个超级大的多人协作的项目中, 是非常重要的. 你需要知道一个返回值的精确语义. 在大部分情况下, 多加一个 `if (ptr != nullptr)` 这样的判断并不会有太大的开销, 但是还是有很多性能非常重要的场景, 我们想要保证一个对象, 它维护了一个指向非空的指针, 同时维护了对象的所有权. 前者我们一般会用引用来直接代替 (引用的语义基本就是, 一个指向对象而非空的指针), 后者我们会用智能指针来管理. 那么两者兼顾呢? 似乎并没有一个工具能实现这一点. 事实上, C++ 中完成这个几乎注定是不可能的: 考虑一个这样的智能非空指针被移动之后的状态, 它不再持有所有权, 那么它的指针指向什么呢? 指向原来的对象将会带来垂悬引用, 这是极其危险的. 事实上, 除非编译器提供支持, 在编译期间做出静态检查, 否则我们永远无法跳出 "这个智能指针可能是空" 的难题, 要么通过高质量的代码和注释来清楚的告诉开发者 "这里一定不是空", 要么就是强迫开发者使用前判断这个指针是否为空 (当然, 你也可以自己包一层智能指针, 所有涉及解引用的操作前插入检查, 如果为空则抛出异常, 相当于防御式编程). 这时候, rust 生命周期那套相关的东西, 以及强制的边界检查, 或许可能可以帮你避免 C++ 这边 "指针是不是空" 的心智负担, 而那些现代语言提供的语法糖 (比如结构化匹配 `match`), 也能有助于读者写出更有可读性、更易于维护的代码.

总之, 笔者强烈建议所有写 Modern C++ 的读者认真的去了解、体验一下 rust, 这一定会在你的编程生涯留下浓墨重彩的一笔.

### non-owning views

[span](https://en.cppreference.com/w/cpp/container/span), [string_view](https://en.cppreference.com/w/cpp/string/basic_string_view), [rust](https://www.rust-lang.org/)

其实就是 `std::span` 和 `std::string_view`. `std::span` 表示对于一个内存上连续区域的视图, 类似一个裸指针 + 区间长度, 而 `std::string_view` 则几乎就是 `std::span<const char>`. 需要注意 non-owning 不代表元素不能修改, 只是表明这个区间的元素不是由持有 `span` 或者 `string_view` 的人来析构, 保证在持有 `span` 和 `string_view` 时区间尚未被析构而已. 要彻底搞明白生命周期, 笔者还是建议读者亲自实践一下 `rust`.

笔者强烈建议尽可能用 `std::span` 替换所有的 `const std::vector<T> &`, 用 `std::string_view` 替换一切的 `const std::string &` (除非要求 null-terminated string). 这不仅是写法更加 modern 代码语义更精确, 它有时还能稍微提升一点代码性能, 并且比起裸指针, 提供了更好的封装.

> Remark: 我想要 `std::optional <T&>`, 请参考后文 [optional 一章](#optional--variant)

## type-safe

Modern C++ 一个突出的特点是, 我们要保证类型安全, 避免危险的 `reinterpret_cast` 防止错误的内存访问. 而标准库也提供了不少容器来帮助我们实现这一点.

需要注意的是, 这些实现并不一定是最高效的, 相信读者自然能想出更加 memory efficient 的实现, 但是在大部分不是那么关心性能/memory 的场景, 尤其是短小的、几乎一定会被 "内联优化" 的函数里面, 以下这些标准库组件能给用户带来极大的便利.

### function

[function](https://en.cppreference.com/w/cpp/utility/functional/function), [move_only_function](https://en.cppreference.com/w/cpp/utility/functional/move_only_function/move_only_function)

`std::function` 传入一个函数签名作为模板参数, 其是裸函数指针的一个替代品, 但是更加灵活. 对于任何一个实现了 `operator()` 并且参数满足函数签名的一个对象, 我们称之为仿函数 (functor), 这是重载运算符给我们带来的便利. 如果这个对象满足可以被复制 (e.g. 函数指针, 常见的 lambda 函数等等), 那么 `std::function` 就可以对应的构造.

```cpp
std::function<void(int)> f; // a function that takes in as an int as argument, return void
f = [](int x) { return x; }; // ok, discard return value
f = [](float) {}; // ok, cast int to float when f is called
struct my_functor {
    auto operator()(int) -> void {}
};
f = my_functor{}; // ok, functor can be invoked with an integer
```

这自然不是免费的午餐, 代价是它类似函数指针, 会引入间接跳转的开销, 而且会拷贝/移动一份对象, 这中间可能涉及堆上内存的分配 (虽然 `gcc` 和 `clang` 都有做 small object optimization). 同时, 经典的 `std::function` 要求对象满足可以复制的条件, 这也并不是适用于所有对象 (比如持有类似 `std::unique_ptr` 类似的唯一资源的对象), 这是因为 `std::function` 为了保证本身可以复制所做出的牺牲.

幸运的是, 如果我们希望得到一个内部对象只可移动 (即转交所有权) 而不需要可复制的 `std::function`, 在 C++23 中有 `move_only_function` 可供选择.

事实上, `std::function` 内部需要持有一份对象, 这本身其实暗含了一种所有权, 也因此不可避免地需要构造/拷贝一份. 那么你可能会好奇了, 如果我们想有一种类似 `std::string_view` 或 `std::span` 那种视图一样不含所有权的结构, 应该怎么解决呢? 在 `C` 语言中, 常见的一种解决是传入一个内容指针 `context`, 以及一个回调的函数指针 `func`:

```c
void f(void *context, void(*func)(void *)) {
    func(context);
}
```

在 C++ 中, 我们自然也可以自己实现一个类似的 `function_view`, 只需要在涉及右值的时候处理好生命周期即可 (我们不应该保存右值的视图, 因为 `rvalue` 可能是 `prvalue`, 在表达式结束后生命周期就结束被析构了). 这玩意网上的参考实现也很多, 这里就不多介绍了.

### optional & variant

[optional](https://en.cppreference.com/w/cpp/utility/optional), [variant](https://en.cppreference.com/w/cpp/utility/variant), [get](https://en.cppreference.com/w/cpp/utility/variant/get), [get_if](https://en.cppreference.com/w/cpp/utility/variant/get_if), [visit](https://en.cppreference.com/w/cpp/utility/variant/visit), [holds_alternative](https://en.cppreference.com/w/cpp/utility/variant/holds_alternative)

> Remark: 需要 C++17

- `optional` 表示一个值可能是不存在, 也可能是存在的. 常见的场景是查找一个元素是否存在, 如果存在则返回这个元素, 否则返回一个特殊的状态, 表示不存在.
- `variant` 则表示存储的 **可以且必须** 是某几种值中的一个, 可以认为是加强版的 `optional`.

可以看出, 这两个东西的存在就是为了取代 C 里面 `union` 的存在 (如果你还不知道, 可以自己先去了解一下). `union` 最大的问题, 是 RAII 资源管理相关的. 假如 `union` 里面的成员有析构函数, 那么在析构的时候, 应该调用哪个成员的析构函数呢? 处理不当, 非常容易造成资源泄露. 这时候, 我们就可以用到 `variant` 来管理了. 特别地, 如果只有 "有" 和 "没有" 两种状态, 那么我们可以用 `optional`, 它提供了更准确的语义.

当然, 虽然笔者一直提倡使用标准库, 但标准库也不是十全十美的. 比如 `optional` 里面, 标准禁止了其直接存引用类型例如 `std::optional<int>` (至少截至 C++23 如此). 仔细思考一下引用的语义是什么: 引用一个对象, 语义上等价于保证非空的指针解引用. 因此, `optional` 引用可以只存一个指针, 如果为空则表示 "没有引用", 否则表示 "合法的引用", 这完全是合情合理的. 它不会引入额外的开销 (甚至还能减少存储空间), 能提供更好的封装 (比起裸指针), 只可惜尚未进入标准库, 不过已经有 [提案](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2988r9.pdf).

对于 `optional`, 笔者推荐结合其成员函数 `.and_then`, `.or_else`, `.transform` 之类使用, 以获得 monad 的效果. 当然, 你也可以用 `if (auto opt = func())` 来分别实现 `optional` 非空和空的逻辑. `optional` 的解引用并没有做边界检查 (非空与 type safety 无关), 如果想要做检查, 请使用 `.value` 函数来获取内部得引用.

对于 `variant`, 笔者推荐使用 `std::visit` 来遍历类型. `std::hold_alternatives` 一般只适用于 `variant` 里面类型不多, 或者只需要特判是不是某一两种特殊的类型 case. 通过 `std::get` 来访问 `variant` 是 type-safe 的, 不用担心访问到错误类型.

在构造 `optional` 的时候, 可以用 `std::nullopt` 表示空, 或者直接花括号 `{}` 默认构造为空, 或者用一个对应存储的类型. 如果你想要给已有的一个 `optional` 更新它的值, 除了可以用 `=`, 也可以用 `emplace` 原地构造.

`variant` 类似, 但是默认构造会调用 `variant` 里面第一次类型的构造函数 (不一定存在, 此时 `variant` 不可默认构造). 一般来说, 如果希望 `variant` 也存在某种类似的 "空" 的状态, 我们会用 `std::monostate`. 赋值和 `emplace` 类似 `optional`.

```cpp
std::variant<std::monostate, int, float, std::string> v {}; // default monostate
v = 1; // ok, construct an int
v.emplace<std::string>(100, 'a'); // good, construct directly
```

需要注意, `optional` 和 `variant` 不涉及堆内存分配, 所有数据都存在内部.

### any

[any](https://en.cppreference.com/w/cpp/utility/any), [any_cast](https://en.cppreference.com/w/cpp/utility/any/any_cast), [make_any](https://en.cppreference.com/w/cpp/utility/any/make_any)

> Remark: 需要 C++17

当你完全不确定可能的类型, 并且希望进一步增加未来的可拓展性, 完全 "擦除" 类型的时候, 你可以用 `void *`. 它直接把类型完全抹去了, 但对应的, 在调用处, 你为了获取其确切类型, 只能用 `std::any_cast` 一个一个去判断.

`any_cast` 当传入的是 `std::any` 的指针的时候, 会返回一个指针, 如果为空表示 `any` 存储的不是这个类型的, 否则为指向对象的指针. 当传入的是 `std::any` 的左值或右值引用时, 如果不是这个类型则会抛出异常, 否则返回存储类型的值. `any_cast` 对于传入引用的情况, 会自动地选择返回时候是进行移动构造还是复制构造.

```cpp
std::any x;
if (auto *y = std::any_cast<int>(&x)) {
    // y is an int * in this case
} else {
    // x doesn't store an int
    if (x.has_value()) {
        // x is not default, or nullptr, or .reset() called.
    } else {
        x.reset(); // reset to a state of empty
    }
}

// this is a common error!
// 1.0 is implicitly cast to std::any && in this case
// I think this is deficiency of std library......
std::any_cast<int>(1.0);
```

在赋值一个 `any` 的时候, 除了常见的 `=` 之外, 你也可以类似 `optional` 和 `variant`, 使用 `emplace` 来原地构造, 减少潜在的移动和复制. 当然, 直接构造也可以用 `std::make_any`.

由于不确定对象的大小, `std::any` 的构造往往涉及堆内存的分配, 不过编译器一般都有 small object optimization.

### format

> Remark: 需要 C++20

由于时间限制, 简单的介绍可以参考: {% post_link 'cpp20' %}. 进阶请自行 cppref.

## 类型和模板的魔法

模板是 C++ 的核心特性. 模板本身就是图灵完备的, 它的功能非常强大. 当和 C 语言的宏结合在一起的时候, 他几乎能创造一切的其他语言. 当然, 这稍微有点夸张了, 但是模板的力量是非常强大的. 结合 C++17 的折叠表达式, 以及 C++11 的 lambda 函数, 你可以写出非常优雅的代码.

### 从 format 到模板推导

[CTAD](https://en.cppreference.com/w/cpp/language/class_template_argument_deduction), [format](https://en.cppreference.com/w/cpp/utility/format/format)

模板推导是非常令人头疼的一部分. 举例:

```cpp
template <typename T>
auto add(std::vector<T> &v, T x) -> void {
    for (auto &i : v)
        i += x;
}

auto f() -> std::vector<int> {
    auto x = std::vector{1, 2, 3}; // need C++ 17 deduction guide,
                                   // the compiler can deduce x as std::vector<int>
    add(x, 1.0); // error, int and double are incompatible
    return x;
}
```

你可能预期的是, `T` 能够自己转化为 `int` (带来的是 `1.0` 被 cast 为 `1`), 但是事实上, 这是不可能的. 遗憾的是, 这里的 `vector<T>` 和 `T` 共同参与了类型的推导, 因此 `T` 的类型不相同, 无法通过编译.

一个简单粗暴的解决方案是: 第二个参数也使用模板. 但这不是我们今天的主题. 事实上, 第二个类型可能也是依赖推导的模板类型, 比如 `list<T>`, 但是实现了类型转化函数或者有其他的特殊要求等等. 针对我们现在的场景, 我们希望类型推导完全由 `vector<T>` 来决定. 这时候, 我们可以用到 `std::type_identity_t`.

```cpp
template <typename T>
auto add(std::vector<T> &v, std::type_identity_t<T> x) -> void {
    for (auto &i : v)
        i += x;
}
```

他的原理是: `std::type_identity_t` 是一个模板别名, 实际是 `type_identity<T>::type`. 而这里作为类的成员类型, 并不会参与推导, 因此 `T` 的类型完全由 `vector<T>` 决定. 这样, 我们就可以正确的推导出 `T` 的类型了. 这部分实际非常复杂, 具体请参考 [cppreference CTAD](https://en.cppreference.com/w/cpp/language/class_template_argument_deduction). 这个在实践中的确被用到了, 可以参考 `std::format` 的实现.

在 `std::format` 中, `format_string` 是 `consteval` 的, 并且其含有实际 format 的类型作为模板参数, 这是为了编译期做出类型检查. 如果暴力的写, 它可能长这样:

```cpp
template <typename ...Args>
auto format(std::format_string<Args...> str, const Args &...args) -> std::string;
```

这里, 我们需要避免 `format_string` 参与模板类型推导, 因为 `Args` 完全是由入参决定的. 这时候, 注意观察 `format_string` 的定义:

```cpp
template<typename... Args>
using format_string = basic_format_string<char, type_identity_t<Args>...>;
```

这意味着, 在 `using` 的内层, 它用到了 `type_identity_t` 来避免了推导, 笔者可以在这里把 `using` 直接理解为 `#define`, 即直接替换为 `basic_format_string<char, type_identity_t<Args>...>`.

### 模板递归

[fold expression](https://en.cppreference.com/w/cpp/language/fold)

一般来说, 模板递归需要用到特化, 这样的代码非常啰嗦.

```cpp
auto f() -> void {
    // end of recursion
}
template <typename _Tp, typename... _Args>
auto f(_Tp &&t, _Args &&...args) -> void {
    g(t); // do something
    f(args...); // recursive call
}
```

幸运的是, 在 C++17 中, 我们有了 `if constexpr`, 这在一定程度上能减轻我们的负担:

```cpp
// helper class
struct end_of_recursion {};

template <typename _Tp = end_of_recursion, typename... _Args>
auto f(const _Tp &t = {}, const _Args &...args) -> void {
    if constexpr (std::is_same_v<_Tp, end_of_recursion>) {
        // end of recursion
    } else {
        g(t);
        f(args...);
    }
}
```

当然, 不要忘记了我们还有 lambda 函数和折叠表达式:

```cpp
template <typename _Tp, typename... _Args>
auto f(const _Tp &t, const _Args &...args) -> void {
    auto fn = [](auto &&t) {
        g(t);
    };
    (fn(t), ...);
}
```

### 模板 + constexpr

模板还可以和强大的 `constexpr` 协同工作. 通过 `if constexpr`, 我们可以允许在输入模板参数不同的时候返回完全不一样的类型. 结合 `decltype`, 我们甚至可以更方便的写出根据某些常量来推导类型, 从而写出比 `std::conditional_t` 更加直观的代码.

```cpp
template <int N>
constexpr auto is_odd(int n) -> bool {
    return n % 2 == 1; 
}

template <int N>
auto f() {
    if constexpr (N == 0) {
        return std::string {};
    } else if constexpr (is_odd(N)) {
        return std::vector<char> {};
    } else {
        return std::array<char, N>{};
    }
}

template <int N>
using f_type = decltype(f<N>());

// use std::conditional_t
template <int N>
using f_type2 = std::conditional_t<N == 0, std::string, std::conditional_t<is_odd(N), std::vector<char>, std::array<char, N>>>;
```

### 模板 + concept

[SFINAE](https://en.cppreference.com/w/cpp/language/sfinae), [concept](https://en.cppreference.com/w/cpp/language/constraints)

`SFINAE` 是一个老功能了. 他的全称是: `Substitution Failure Is Not An Error`. 他的作用是: 当模板参数推导失败时, 不会报错, 而是会继续尝试其他的模板. 例如:

```cpp
struct A {
    using type = int;
};

struct B {
    using fallback = int;
};

template <typename T>
auto f(T) -> T::type {
    return 0;
}

template <typename T>
auto f(T) -> T::fallback {
    return 0;
}

int main() {
    f(A{}); // ok, the first f is called
    f(B{}); // ok, the second f is called
}
```

在这里, 如果模板类型 `T` 没有 `type` 成员, 那么第一个 `f` 会被忽略, 而继续尝试第二个 `f`. 这就是 SFINAE 的作用. 其可以用于很多场景, 例如: 检查类型是否有某个成员, 检查类型是否满足某个特定的条件等等. 常见的搭配有 `std::enable_if_t`, `std::void_t` 等等. 如果 SFINAE 匹配到多个成功的模板, 会选择特化程度最厉害的, 这个说法一听就不是很严谨, 具体细节还是请参考 [cppreference](https://en.cppreference.com/w/cpp/language/sfinae).

然而, 在大部分情况下这样的代码可读性极差. 例如:

```cpp
// ensure T is an integral type, otherwise try other templates.
template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
auto f(T) -> void {
    // do something
}
```

幸运的是, C++20 的 `concept` 能够解决大部分这类问题, 其依然遵循的是 `SFINAE` 的原则, 但是使用更加直观的 `requires` 语句来明确指定模板的约束. `concept` 部分可以参考 [这篇文章](https://darksharpness.github.io/cpp20), 但是更推荐 [cppreference](https://en.cppreference.com/w/cpp/language/constraints).

对于上面那个例子, 可以简写为:

```cpp
template <std::integral T>
auto f(T) -> void {
    // do something
}
// another way
template <typename T> requires std::integral<T>
auto f2(T) -> void {
    // do something
}
```

无论如何, 可读性都比无 `concept` 的 `SFINAE` 强太多了. 关于 `concept` 的四种写法, 除了 [cppreference](https://en.cppreference.com/w/cpp/language/concepts), 也可以参考 {% post_link 'cpp20' %}.
