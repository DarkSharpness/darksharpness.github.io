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

### C++ if/switch

### constexpr/consteval/constinit

## memory-safe

Modern C++ 一个突出的特点是, 内存安全. 其中, RAII 给我们的实现提供了巨大的便利. 而 `memory safe` 的实现, 其靠的就是 smart pointer. 在大多数的情况下, 我们都应该用智能指针替代裸指针.

- `unique_ptr` 只能有一个拥有者的指针.
- `shared_ptr` 可以被多处拥有的指针, 需要注意防止循环引用.

说实话, 这些智能指针其实最大的便利不是访问的安全性, 用户依然可以随便就写出访问空指针的代码, 它们最大的好处还是, 实现了内存资源的自动回收. 这其实本质就是 RAII 思想的运用, 构造处获取资源, 析构处回收资源, 而移动语义则一般表示资源的转交 (比如 `unique_ptr`, 在移动构造/赋值之后, 被移动的对象会被重置为空指针). 这些先进的理念也被后来很多的编程语言, 比如 `rust` 所采纳.

### unique_ptr

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

## type-safe

Modern C++ 一个突出的特点是, 我们要保证类型安全, 避免危险的 `reinterpret_cast` 防止错误的内存访问. 而标准库也提供了不少容器来帮助我们实现这一点.

需要注意的是, 这些实现并不一定是最高效的, 相信读者自然能想出更加 memory efficient 的实现, 但是在大部分不是那么关心性能/memory 的场景, 尤其是短小的、几乎一定会被 "内联优化" 的函数里面, 以下这些标准库组件能给用户带来极大的便利.

### function

### optional & variant

> Remark: 需要 C++17

- `optional` 表示一个值可能是不存在, 也可能是存在的. 常见的场景是查找一个元素是否存在, 如果存在则返回这个元素, 否则返回一个特殊的状态, 表示不存在.
- `variant` 则表示存储的 **可以且必须** 是某几种值中的一个, 可以认为是加强版的 `optional`.

可以看出, 这两个东西的存在就是为了取代 C 里面 `union` 的存在 (如果你还不知道, 可以自己先去了解一下). `union` 最大的问题, 是 RAII 资源管理相关的. 假如 `union` 里面的成员有析构函数, 那么在析构的时候, 应该调用哪个成员的析构函数呢? 处理不当, 非常容易造成资源泄露. 这时候, 我们就可以用到 `variant` 来管理了. 特别地, 如果只有 "有" 和 "没有" 两种状态, 那么我们可以用 `optional`, 它提供了更准确的语义.

当然, 虽然笔者一直提倡使用标准库, 但标准库也不是十全十美的. 比如 `optional` 里面, 标准禁止了其直接存引用类型例如 `std::optional<int>` (至少截至 C++23 如此). 仔细思考一下引用的语义是什么: 引用一个对象, 语义上等价于保证非空的指针解引用. 因此, `optional` 引用可以只存一个指针, 如果为空则表示 "没有引用", 否则表示 "合法的引用", 这完全是合情合理的. 它不会引入额外的开销 (甚至还能减少存储空间), 能提供更好的封装 (比起裸指针), 只可惜尚未进入标准库, 不过已经有 [提案](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2988r9.pdf).

### any

> Remark: 需要 C++17

### 现代的 range 库

> Remark: 需要 C++20

### 直观的 format 库

> Remark: 需要 C++20

## 类型和模板的魔法

### 避免类型推导: format 的秘密

### trait 和类型体操

### 更好的 SFINAE 和 concept
