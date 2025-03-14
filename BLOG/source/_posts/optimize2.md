---
title: 玄学优化和语言知识 2.0
date: 2023-11-11 11:11:11
updated: 2023-12-22 12:35:26
tags: [C++,基础知识,优化]
categories: [C++,优化]
cover: https://s3.bmp.ovh/imgs/2023/12/22/1a16727b353555b1.jpg
keywords: [基础知识,优化]
description: 编译器写的。
mathjax: true
---

写了编译器以后，对于 C++ 的理解又进步了不少，看 Compilor Explorer 的汇编也轻松了许多，也对于优化有来些更加深刻的认识。

下面将会讨论一些有趣的问题，会比较零散。

# 移动语义与右值

## 为什么要有这玩意

原因大概是这样的: 对于复杂的对象，其可能本身不大( `sizeof` 的大小)，但是其本身管理了一些指针，那些指针指向了大量的数据，体积可能是数百倍之于本身。最常见的就是 STL 中的各种容器，比如 `std::set`,`std::vector`,`std::list` 甚至是 `std::string` 等等。

以 `std::vector` 为例，其本质上是保存了指向数据区的三个指针来进行维护。在我们拷贝一份 `std::vector` 的时候，我们可能会提出这样的问题: 如何高效地拷贝 `std::vector` 中的数据。一般情况下，我们肯定想的是直接拷贝，用 `std::copy` 把数据复制一份。

复制的开销当然是巨大的，这时候，聪明的你可能会发现，我们其实不一定要拷贝数据，我们可以只保存指向那些数据的指针即可。是的，这是一种合理的解决方案，在原来的 `std::vector` 失效之前，我们的确可以用指针来访问原来的数据。当然，C++ STL 提供了一个不错的包装: 迭代器 (其实大多就是指针的包装...)。不同容器的迭代器提供了一种通用的容器视图，用 `begin()` 和 `end()` 两个迭代器表示一个区间。

当然，迭代器的方案并不是严格意义上的拷贝。那些迭代器只是对于实际容器区间的一个视图，其与实际的数据的生命周期无关。换句话说，原有数据的“掌控权”还是在老的 `std::vector` 里面，当这个 `std::vector` 发生改变(比如扩容，析构)时，这个迭代器视图可能会失效。因此，真正的拷贝肯定不能这么写。

但是，对于那些生命周期步入尾声的容器 `std::vector`，其该操作之后数据即将被析构，不会再被用到。在这样的情况下，我们可以考虑“接管”这个 `std::vector`，即让其在析构的时候不去释放数据，而是把数据的掌控权交给新的 `std::vector`。

```C++

std::vector <int> func();
void work(std::vector <int> &);

void test() {
    // 这里的 func() 返回的是一个临时对象
    // 在这个赋值表达式结束后，这个临时对象在析构前不会再被用到
    // 因此，我们可以考虑接管这个临时对象的数据 
    std::vector <int> tmp = func();
    work(tmp);    
}

```

而这，便是移动操作。而那些可以被移动的对象，便是右值，其可以被右值引用绑定。而 C++ 则进一步扩大的程序员自由发挥的空间。除了那些显然生命周期即将结束的临时变量可以被右值引用绑定，我们也可以通过 `static_cast` 把一个左值引用转化为右值引用，通过类型的不同来调用不同的函数。这样，我们可以统一对于那些在这个函数之后管理的数据不会再被用到的类型进行针对性的优化。

例如本例 `std::vector` ，可以直接把新的 `std::vector` 的指针指向原来的 `std::vector` 的数据，再把原来的 `std::vector` 的指针设置为空，使得这个数据完全被新的 `std::vector` 接管。

## 一些性质

移动操作其实随处可见。例如，正常的一个函数调用会返回一个临时变量，而它显然在这个操作之后，就不会再被用到，因此，其显然是一个右值，可以被右值引用绑定。又比如说，类型转化后(无论是隐式类型转化还是显式的)，其返回的也是一个临时变量，也是一个右值。

这些天然右值显然可以绑定到右值引用。当然，前面也讲到，我们可以通过 `static_cast` 把一个左值引用转化为右值引用。当然，`static_cast` 还是太长了点，所以标准库提供了 `std::move` 作为一个语法糖，其实现几乎就是 `static_cast`。

那右值的意义何在？为什么要强转为右值引用？因为其决定了重载协议！我们可以根据是左值还是右值来决定是否取走的引用对象的数据，从而实现移动语义。

```C++
std::string str = "abc";

// 如果 str 确认不会再被用到了，那么我们可以把 str 的数据移动到 a 中
// 在 move 以后，str 就不再拥有数据了
// 按照 cpprefence 的说法，数据处于一种合法但是未定义的状态，至少可以保证析构不会出错
std::string a = std::move(str);
```

对于那些天然的右值，比如函数返回值(包括类型转化函数)，我们自然是不需要 `std::move` 来要求移动语义。

```C++
void work(std::string);
void test() {
    std::string a = "114514";
    std::string b = "1919810";
    // 这里的 a + b 返回的是一个临时变量，其 work 之后就不会再被用到，是天然的右值
    // 因此，其可以被右值引用绑定，不需要 std::move()
    // 注意到 work 的参数是传值，所以 work 的参数是采用右值版本的构造函数构造的
    work(a + b);
}

```

对于那些有名字的变量例如，其都属于左值，我们需要 `std::move()` 来要强制求移动语义，调用的函数才会选择绑定右值引用的版本，否则，其只会选择绑定左值引用的重载。

```C++
std::string a;
std::string b;
std::string &c = b; // 正常的左值引用变量，一般来说只能绑定左值

// 右值引用变量，可以绑定那些天然的右值，将其生命周期延长至与变量一致
// 其当然可以绑定那些强转的右值引用，但是那些右值引用的生命周期不会延长，这么做也几乎没有意义
std::string &&d = "ab";


std::string x = std::move(c); // 右值引用，调用移动构造，复杂度是 O(1) 的
std::string y = c; // 左值引用，调用拷贝构造，复杂度是 O(n) 的，n 是字符串长度

x = std::move(c); // 右值引用，调用移动赋值，复杂度是 O(1) 的

y = d; // 即使是右值引用变量本身，其有名字，也是左值，调用拷贝赋值，复杂度是 O(n) 的
y = std::move(d); // 右值引用，调用移动赋值，复杂度是 O(1) 的

```

当然，这里有一个特例，当一个变量值作为函数的返回值的时候，我们 **不需要甚至不应该** 用 `std::move` 来把其强制转化为右值，编译器会自动帮我们做这件事情。笔者猜测，这是因为函数中的值(变量)生命周期与函数一致，因此，其可以被看作是一个天然的右值。但是重点不在于此！如果你加了一个 `std::move` ，其有时候可能会阻止编译器进行 RVO 返回值优化 (具体是 NRVO，具名返回值优化，其优化力度强于移动语义)。

```C++
std::string Hello() {
    string name = "Hello";
    // 这里不应该用 std::move
    // 首先编译器会自动帮我们做这件事情(如果没有进行 NRVO 的话)
    // 其次这可能会阻碍 NRVO 优化
    return name;
}

std::string Hello(std::string &ref) {
    // 这里需要用 std::move, 是因为 ref 不是值
    // 其在函数结束后生命周期不一定要结束
    // 因此，如果需要移出数据，需要用 std::move
    return std::move(ref);
}

```

![NRVO 要求返回的是变量值，move 之后无法满足该要求](https://s2.loli.net/2023/11/11/nScuUXxQ8aLl6vz.png)

## 右值引用变量到底延长了啥?

前文提及了右值引用变量，这东西其实表现就和一个普通的左值引用变量几乎完全一致，为了移动语义我们还需要用 `std::move` 来选择重载决议，唯一的特点就在于延长生命周期这一点......

其本质上并不是延长生命周期，而是赋予了临时变量值了一个名字...... 当其初始化等号右边是一个临时变量值 (也就是前面所说的天然右值，包括调用函数的返回值，类型转化等等) 的时候，其的确可以延长其寿命 (因为临时值的生命周期是确定的，如果没被延长立刻就死了)。但是当初始化等号右边是一个引用时，引用本身与引用的对象的生命周期并不挂钩，引用的对象可能在十万八千里外! 因此，编译器显然无法保证延长那个实际变量的周期，所以并不会有任何作为。

换句话说，只有当右值引用绑定的对象的生命周期明显确定(编译器可知，其实就是那些临时变量)，其才能延长对象的寿命。

```C++
// 请把 std::move 看作 static_cast <int &&>
int x = 0;
int &&tmp1 = 0; // OK! 生命期延长至与 tmp1 一致
int &&tmp1 = std::move(x); // OK! 没有生命周期被延长
int &&tmp2 = std::move(1); // 未定义行为 Undefined behavior，这是垂悬引用
```

## 移动后的对象

在实际编程中，对于那些被移动的对象，其基本上只会等到自然生命周期然后被析构，我们理论上不会再对其做任何事情。但是，移动只不过说其内部的资源可以被移走，引用的对象依旧处于一个可以被析构的，合法但不确定的状态。

当我们清楚具体的实现的时候，我们依然可以操作那些被移走的变量。例如对于常见 STL 容器 `std::vector` ，在 x86_64, gcc 13.2 的标准库实现中，被移走的后 `std::vector` 为空。事实上，常见实现都是移动后设空，至少笔者遇到的 `std::vector` 和 `std::list` 和 `std::set` 什么的都是这样的。

当然，以上不是标准规定的内容，其取决于库的实现。就笔者所知，在 C++ 标准中，智能指针 和 std::thread 有特殊指定，被移动后的状态为空。如果你实在不放心，可以调用 clear() 函数后再使用 (理论上如果移动后为空，这干的是重复的事情，在 O2 下几乎肯定会被优化掉，所以不放心的话那就加上吧)。

简而言之，清楚实现的情况下，怎么玩都行（

## 小总结

移动语义和右值是 C++ 搞出来的小 trick。本质上，C++ 添加了右值引用，其类似左值引用，但是其只能绑定右值，和左值引用属于不同的类型。因此，通过其提供的类型信息来选择不同的重载，从而实现了移动语义。

换句话说，其不存在所谓的修改生命周期，只不过是资源管理权的移交罢了，相关变量析构的生命周期也不会变 (唯一的特例是前面讲到的对于天然的右值，使用右值引用延长生命周期)。

其好处是在某些时候减少拷贝的开销，充分利用起来那些在析构前不会再被用到的资源，从而提高程序的效率。

特别地，当对应的函数没有重载对应的右值版本 (按值传递潜在的含有右值构造函数)，或者当前类型没有可以移走的数据(比如基本类型 `int` ，移动和拷贝开销一致)，那么 `std::move` 并没有任何意义。同时，对于作为函数返回值的变量值，我们不应该使用 `std::move`

当然，以上的说法其实并不严谨，真正的右值还分为 xvalue ，prvalue 等等，想了解的可以自行 cppref，这里就不过多介绍了。

# 传值还是传引用?

在编写 C++ 程序的时候，一个非常头疼的问题是到底应该传值作为函数参数，还是传一个引用，如下所示。

```C++
void pass_value(std::string);
void pass_reference(const std::string &)
```

下面将会以一个不严谨的视角来解释如何解决这个问题。

## 不严谨的直观

首先，引用的本质是什么? 在写了编译器后，我可以自信的说，在绝大多时候，引用“表现”的就和一个指针几乎完全一致，只不过它本身(即类似指针指向的地址)不能被修改。换句话说，其可以简单视作 const pointer 的语法糖。

```C++
void ref(int &);
void ptr(int * const); // 注意，const 修饰的是 * 而不是 int
```

换句话说，引用类似一种指针的约定。该约定要求该指针指向的对象非空，且该指针绑定的对象(即指针的值，对象的地址)不可切换，这显然很有助于编译器做特定的优化。更重要的是，其被赋予了其他更加强大的功效，结合 C++ 语言特性。

例如，在 C/C++ 中，引用不仅能够绑定左值，对于 `T` 类型，`const T &` 还能绑定右值。而在 C++ 中只有左值可以取地址，`const T &` 这种绑定右值的特性是 `const T * const` 所不具有的。

```C++
int func() { return 0; }

int x = 0;
int &y = x; // 正常情况

// int &z = 0;      报错，因为 0 不是左值
// int &w = func(); 报错，函数返回值不是左值

const int &z = 0;
const int &w = func();

// const int *p = &0; 报错，不能取地址
// const int *q = &(func()); 报错，函数返回值不是左值
```

类似右值引用，const 引用绑定临时变量后，其生命周期会被延长至与引用一致。但是，对于那些强转绑定的右值引用，则不会延长。

```C++
int x = 0;
const int &tmp1 = 0; // OK! 生命期延长至与 tmp1 一致
const int &tmp1 = std::move(x); // OK! 没有生命周期被延长
const int &tmp2 = std::move(0); // 未定义行为 Undefined behavior，这是垂悬引用
```

## 开销?

既然和指针类似，那么其开销也应该和指针类似，而指针本身在 64 位系统上就占据 8 个字节。显然，比起恐怖、复杂度未知的拷贝，这个开销是可以接受的。这也是为什么很多人都推荐初学者使用引用而不是指针。

但是，如果拷贝的开销是已知的，那么我们就权衡一下两者的开销比。对于那些短类型例如 `int` 和 `long long` 之类，我们完全没必要传引用。拷贝的开销此时和引用一样，还可以避免引用(指针)间接取值的潜在开销 (这需要一次额外的不确定的地址访问，对比之下局部变量可能被优化为寄存器存储，或者是一个确定的栈上地址)。

因此，笔者的建议是，对于那些拷贝不超过 16 Byte 的类型，也就是不超过`sizeof(std::size_t) * 2` 大小的类型，我们应该传值而不是传引用，除了基本类型，常见的有 `std::complex <double>`,`std::string_view`,`std::pair <int, int>` ，甚至是 `std::unique_ptr` 等等 (这是因为 `unique_ptr` 只支持移动不支持拷贝，移动构造只需要拷贝一个指针的大小)。当然，这个 16 Byte 的界限并不是绝对的，只是一个经验值，可以根据实际情况进行调整。

```C++
void good_func(const std::string &,int);
void good_iff_you_know_wtf_u_r_doing(std::string,const int &);
```

# new 和 delete 到底干了什么

大家都知道 `C` 语言中 `malloc` 和 `free` 分别申请内存/释放内存，而 `C++` 中，我们一般使用 `new` 和 `delete` 来申请/释放内存。但是，这两者到底干了什么呢?

## new

由于 C++ 的特性，在使用 `new` 的时候不仅分配了内存，还会调用构造函数来构造对象。

在分配内存的时候，其实 `new` 差不多就是调用了 `malloc` 来实现的，但是当空间不够的时候，`malloc` 会返回空指针，而 `new` 会抛出异常。如果你不希望抛出异常，可以使用 `nothrow` 版本的 `new` ，其会返回空指针。例子来自 [cppreference](https://en.cppreference.com/w/cpp/memory/new/nothrow) 。

```C++
#include <iostream>
#include <new>
 
int main() {
    try {
        while (true) {
            new int[100000000ul];   // throwing overload
        }
    } catch (const std::bad_alloc& e) {
        std::cout << e.what() << '\n';
    }
 
    while (true) {
        int* p = new(std::nothrow) int[100000000ul]; // non-throwing overload
        if (p == nullptr) {
            std::cout << "Allocation returned nullptr\n";
            break;
        }
    }
}
```

特别地，对于那些基本的内置类型，比如 `int` 和 `char *` 这些，其没有所谓构造函数因此，包括类似地的聚合类，比如 `struct { int x,y; }; ` 。此时，单纯的 `new` 不会调用构造函数，而是直接分配内存。这种情况下，如果你想要让里面的数据默认初始化(为 0)，你需要用花括号或括号显式初始化。

事实上，该初始化过程非常类似 **局部变量的初始化** 。对于内置类型，在构造函数未指值的时候 (比如没有构造函数的 `int`，或者构造函数没为这个 `int` 成员变量初始化)，其值是不确定的。一般来说，对于聚合类/内置类型，会使用 `{}` 初始化来保证是 `0` 。

```C++
int *p = new int; // 分配内存，但是不会调用构造函数
int *q = new int(); // 分配内存，同时调用构造函数，初始化为 0
int *r = new int{}; // 同上
std::cout << *p << std::endl; // 未定义行为，p 指向的内存未初始化
std::cout << *q << std::endl; // 正常输出 0
std::cout << *r << std::endl; // 正常输出 0

struct A { void *p; long long x; };
A *a = new A;   // 分配内存，什么都不干
A *b = new A{}; // 分配内存，初始化为 0

struct B { int x; B() = default; }; // 没有初始化哦

B *c = new B;   // 分配内存，什么都不干
B *d = new B(); // 分配内存，调用构造函数，初始化为 0
```

那为什么不要求基本类型默认初始化为 `0` 而要求显式写出 `{}` 或 `()` 才能呢？这是因为潜在的开销！`C++` 的核心理念是抽象无开销 (Zero overhead) 。那么，初始化为 `0` 还是额外的多做了点事情的对吧，这点开销很有可能是能够避免的，特别是对于大数组的初始化。

## 其他的 new

除了常用的 `new` 加类型来申请内存，还有一些其他的 `new` ，比如 `new[]` 和 `new (std::nothrow)` 等等。当然，这些主题逻辑上是差不多的，都是先分配内存，后尝试调用默认构造函数。真正有意思的是 `operator new` 和 `placement new` 。简单来说，常见的 `new` = `operator new` + `placement new` 。

`operator new` 大致声明如下:

```C++
void * operator new(size_t size);
```

看到 `void *`，想必很多熟悉 C 语言的高手就明白了。这东西几乎就是一个 C 语言的 `malloc` ，只不过额外多了一个防止内存不够的异常抛出。当然，这个 `operator new` 是可以被重载的，我们可以自己实现一个 `operator new` 来实现自己的内存分配策略。网上教程也很多，这里就不展开了，多看 cppreference 就好。

`placement new` 其实根本都称不上 `new` ，其作用是在给定的指针指向的空间上调用构造函数。举例如下:

```C++
void *buf = operator new(sizeof(std::string));      // raw memory
std::string *p = ::new (buf) std::string("Hello");  // placement new

std::cout << *p << std::endl;
```

事实上，给定的指针指向的空间不一定要是 `new` 得到的，甚至可以是栈上的空间。比如 `char` 数组构成的栈上缓冲区之类。

## delete

与之相对的，`delete` 也不仅仅是 `free` 这么简单，其显然还会额外地调用析构函数，这是 `C++` 面向对象的特点。当然，对于基本类型，或者简单类型 (比如 `struct {int x,int y}`) ，其只有 `trivial destructor` ，也就是什么都不干的析构函数，此时 `delete` 也不会额外多做什么。

```C++
int *p = new int;
delete p; // 析构函数什么都不干，直接 free

std::string *q = new std::string {};
delete q; // 析构函数会调用 std::string 的析构函数，然后 free
```

特别地，如果 `delete` 或 `free` 的指针是空指针，那么其什么都不会做!!! 换句话说，这是安全的，你不应该在这之前判断指针是否为空。这是一个常见的习惯，和 `new` 之后判断是否为空指针一样 (因为前面说了，`new` 会抛出异常，只有 `nothrow` 版本的 `new` 才会返回空指针)，都是多余的。

如果你是用 `placement new` 在栈上的空间放置的内存。那么我们肯定不能调用 `delete` 来释放内存，因为栈上内存不能 `free` 。对应的，我们必须对指针显式地调用析构函数。

```C++
using std::string;
char buf[sizeof(std::string)];
string *p = ::new (buf) string("Hello");
p->~string(); // 显式调用析构函数
```

这时候，存在一个小小的 bug ，那就是对于基本类型，其没有析构函数，显然我们不能显式调用析构函数，比如 `~int()` 是错误的。但是当其作为模板参数，在模板实例化的时候被替换，那么此时是合法的 (当然，啥都不会干)。

```C++
// 模板类同理，这里就举一个模板函数的例子
// 即使传入参数为 int，其也可以过编译
template <typename _Tp>
void foo(_Tp *p) { p->~_Tp(); }
```

## 省流

简单来说，`new` 和 `delete` 其实就是对于 `malloc` 和 `free` 的包装。由于 C++ 独特的构造函数和析构函数，对于一个对象，其在申请内存之后必须先执行构造函数，而在归还内存前必须执行析构函数。经过 `new` 和 `delete` 和的包装，我们就不用手动对于申请的内存调用构造函数 (即 `placement new`，如果有的话)，以及在归还内存前调用析构函数 (如果有的话)。

稍微总结一下，`new` 的行为大致是这样的:

1. 首先分配内存空间。如果不够就抛出异常，如果使用 `nothrow` 版本的 `new` 则返回空指针。
2. 尝试调用构造函数。如果是没有构造函数的类型，会调用默认的构造函数。对于 (默认) 构造函数没有覆盖的内置类型，需要用 `{}` 或 `()` 显式初始化为 `0` ，否则其值不确定。

而 `delete` 的行为大致是这样的:
1. 首先调用析构函数。如果是空指针或 `trivial destructor`，什么都不干。
2. 归还内存空间。如果是空指针，什么都不干。

这其实也就是 C++ 的内存模型，核心在于构造函数和析构函数在合适的时候调用。

对于你不清楚的类型实现，请务必保证在一块空间上，只调用一个类型的构造函数。在使用的时候，请务必该对象执行过构造函数。在归还内存前或离开作用域的时候，请务必保证该对象执行过析构函数。只有构造过的对象才是 "存活的" ，而析构前的对象必须是 "存活的" ，析构后对象必须是 "死亡的" ，当然空间也就可以继续使用或者归还了。

当然，对于大家熟知的简单类型比如 `int`，自然没那么多讲究。只要清楚原理，理论上怎么玩都可以 (毕竟指定编译器版本和操作系统，给定对象的实现，行为显然是定义的)。
