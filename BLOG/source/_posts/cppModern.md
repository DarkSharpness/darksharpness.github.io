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

难绷的被下一届的同学拉去给下下届讲 Modern C++. 想了半天, 最后还是决定讲讲 modern effective C++, 也算是笔者的启蒙读本了... 那也是 23 年春节的回忆, 转眼就过去两年了, 这两年笔者又学了些什么呢...

## 从值类型到类型推导

参考内容, Effective Modern C++ Item 1, 23, 24. 虽然基本都是笔者口胡.

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

比较恶心的是数组实参和函数实参.

在推导值类型或者指针类型的时候, 数组会退化为指针, 函数同理. 在推导引用相关类型的时候, 数组会被推导为特殊的数组引用, 函数同理.

### auto 推导

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

## 语法糖

众所周知, C++ 这个语言充满了语法盐, 写起来一点都不舒服, 没法像 python 那样写的非常简洁. 真的是这样的吗?

### for range loop

在 C++11 中,

### 结构化绑定

在 C++17 中, 引入了结构化绑定: 对于一个聚合类 (即没有基类, 没有用户声明的构造函数) 或者按照 [特殊规则](https://en.cppreference.com/w/cpp/language/structured_binding) 重载了对应的函数的类, 我们可以类似 python 中 `tuple` 解包的形式写代码. 比如 `std::pair`, `std::tuple`, `std::array` 以及原生的数组都是支持的. 语法如下, 需要注意的是必须用 `auto`

```cpp
std::tuple<int, float, std::string> t;
auto &[x, y, z] = t;
struct MyStruct {
    int a;
    int b;
} tmp;
auto [a, b] = tmp; // must use auto
```

### type-safe

optional, variant, any, bit_cast

### 现代的 range 库

### 直观的 format 库

## 类型魔法
