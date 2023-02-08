---
title: STL_vector简析
date: 2023-01-23 13:30:10
updated: 2023-01-25 16:43:55
tags: [基础知识,STL]
categories: [C++,STL]
keywords: [STL,vector]
cover: https://s2.loli.net/2023/01/28/HAG6kBYsTN4EKrJ.jpg
description: 略深入地分析 std::vector
mathjax : true
---
&emsp;本文将更加细致的分析 std::vector 的具体原理以及实现。关于vector 的基础功能，请点击[这里](https://darksharpness.github.io/2022/10/23/%E6%B5%85%E8%B0%88C++STL/STL/#std-vector)。需要注意的是，vector &lt;bool&gt; 是一个特化的模板，其实现不同于其他的vector类，本文将不会讨论这一个特殊的实现，感兴趣可自行查询相关信息: [cppreference](https://en.cppreference.com/w/cpp/container/vector_bool)

&emsp;本文重点分析 vector 的底层实现以及其运用到的 C++ 11 以后的特性等等，会讲到一部分但不是全部vector的功能，对于 OIer 可能帮助不大，这里先提醒一下。

# std::vector 的功能

## 模板和声明

&emsp;首先，我们可以在 `#include<vector>` 之后，通过 `std::vector <T> vec` 来声明一个存储了 T 类型变量的动态数组，初始为空。其中，用户还可以提供一个 allocator type 作为模板的第二个参数，但是一般情况下不会用到，本文也不会分析这一个参数。

## 初始化

&emsp;vector 不仅可以初始化为空，也可以用同类型的vector初始化。除此之外，vector还支持初始化列表(要求C++11)，也可以在构造的时候直接设置初始数组的大小以及元素的值。如下所示:

```C++
#include <vector>
using std::vector;

vector <int> vec0;     // 空vector
vector <int> vec1(10); // 初始数组长度为10的vector，这10个元素将分别调用默认无参数构造函数(对于int就是初始化为0)
vector <int> vec2(10,-1); // 初始化长度为10的vector，这10个元素初始化的值为-1
vector <int> vec3 = {1,3,4,5}; // 初始化为一个元素为1,3,4,5的数组

// vector <double> vec4 = vec0; 不可以! 类型不同
vector <int> vec4 = vec0; // 同类型vector 也可用于初始化

```

## 常用功能

&emsp;vector 的功能有很多。最基础的就是通过成员函数push_back() 或 emplace_back() 往 vector 尾部添加一个元素，push_back() 仅支持一个元素作为参数，而emplace_back() 可以传入多个参数，把这些参数作为新添加的元素的构造函数的参数。需要注意的是，我们应当区分 emplace_back() 和 push_back()的本质，两者虽然都是往尾部添加一个元素，但是底层机制不太一样，笔者将会在后文着重讨论。当然，有push_back()也自然会有pop_back()函数。

&emsp;类似原生数组，vector 也可以通过下标访问元素，进行读/写操作。而对于下标越界的情况，直接通过下标访问并不会进行任何检查，带来一些未定义行为。如果你的确需要含有越界检查的访问，请使用at()成员函数，其对于下标越界的情况会抛出错误。vector也可以用back()和front()成员函数访问最后一个/第一个元素，并且是一个读/写访问。当然，你也可以通过data()直接的得到vector的第一个元素的地址。由于vector内部数据是连续储存，这相当于就是指向vector内部数组的指针，但是该指针在vector进行改变容量的操作后会失效。

## 迭代器

&emsp;vector也支持迭代器，既有成员函数版本的begin(),end(),cbegin(),cend()，也有非成员函数版的。也有反向迭代器 (即在正向迭代器前面加一个r,例如rbegin(),crend())。同时，其内存连续存储，所以其也是随机访问迭代器。

## 容积相关

&emsp;vector还有许多关于容量的成员函数。首先是empty()用于判断vector()是否为空，size()返回 vector 内部元素的个数。除了以上两个常用的函数，其实还有一个capacity()，其返回的是，vector 已经申请的空间中可以存放元素的个数。这点可能比较绕，笔者将举例子阐释。例如 `class T` 在内存中占有字节数 $32$ Byte，vector 内部假设已经有了 $3$ 个 T 类型的元素，但是已经了申请 $128=32 \cdot 4$ Byte 的空间，那么其size()毫无疑问就是3，而capacity() 就是4。显然的，一个 vector 内部申请的可以存放元素的个数不少于其已经存放元素的个数。当然，用户不用担心这点，vector 内部会自动动态扩容的。

## 元素相关

&emsp;vector 还有很多的管理内部元素的成员函数。clear()可以清空所有的元素 ；insert() , emplace() 可以在指定位置(用迭代器表示) 插入一个元素 ；erase() 则类似的可以在指定位置删除元素  ；assign()可以重新初始化 vector 内部的元素，统一赋值 ； resize() 可以改变 vector 的 size() ；reserve() 可以为 vector 预留一定的空间 ；；shrink_to_fit() 可以把capacity()降低到和size()一样 ；swap() 可以交换两个同类型 vector 内部的元素，并且只要常数的时间复杂度。附代码:

```C++

vector <int> vec = {1,2,3};
/* vec : {1,2,3} */

vec.clear(); // 清空vector
/* vec : {} */

auto iter = vec.end(); // 指向尾部的 iterator
vec.insert(iter,1); // 在iter 前面插入一个元素
/* vec : {1} , size : 1 , capacity : 1 */

iter = vec.begin();   // 注意，vector 元素个数改变后可能会导致迭代器失效! 请及时更新迭代器。
vec.emplace(iter,3); // 在iter 前面，用一系列参数作为构造函数构造一个新的元素，类似 emplace_back() 
// (insert 和 emplace 具体差别请参考 push_back与emplace_back)
/* vec : {3,1} , size : 2 , capacity : 2 */

vec.erase(vec.end()); // 删除迭代器指向的的元素
/* vec : {3} , size : 1 , capacity : 2 */

vec.resize(3); // 将vec size()设置为3，如果新的size()大于原来的的size() , 新的元素将会采取默认构造函数
/* vec : {3,0,0} , size : 3 , capacity : 3 */

vec.resize(4,1) // 类似，但是新的元素用传入的第二个参数拷贝构造
/* vec : {3,0,0,1} , size : 4 , capacity : 4 */

vec.resize(2);
/* vec : {3,0} , size : 2 , capacity : 4 */

vec.shrink_to_fit(); // 把多余的 capacity 扔掉
/* vec : {3,0} , size : 2 , capacity : 2 */

vec.reserve(3); // 预留一定空间，若小于当前capacity()则什么都不做
/* vec : {3,0} , size : 2 , capacity : 3 */

vec.assign(4,1); // 强行把vector的size()变为4，并且每个元素值等于第二个参数(如果没有就调用默认构造函数)
/* vec : {1,1,1,1} , size : 4 , capacity : 4 */

vector <int> tmp = {2,3,3};
tmp.swap(vec); // 常数时间交换两个 vector 元素
/* vec : {2,3,3} , size : 3 , capacity : 3 */
/* tmp : {1,1,1,1} , size : 4 , capacity : 4 */

```

&emsp;最后，vector 还有一些常见的运算符操作，例如 = 赋值以及 == , &lt; , &gt; 等基本逻辑运算符。

# std::vector 的底层原理

&emsp;vector 是一个动态的数组，拥有自动扩容的能力(push_back操作)，而且具有均摊 $O(1)$ 的添加元素时间复杂度，常常用于各类程序中。

&emsp;从底层的角度来看，vector本质上是储存了三个指针，分别指向内存区域的头部、内存区域尾部 以及 元素存储的尾部。当然，也可以存储头部指针、size()、capacity() 代替。$

&emsp;之所以 vector 能够支持 $O(1)$ 的添加元素时间复杂度 ，是因为其采用了 "倍增" 的扩容方式。当 size() == capacity() 的时候 ，再push_back()必须要扩容，因为申请的内存空间不足了。而此时，如果只申请 size() + 1 个元素的空间，那么，下次push_back()又要申请一次。这么实现，push_back() $n$ 次就达到了 $O(n^2)$ 的时间复杂度了。而基于 "倍增" 的思想，我们每次push_back需要扩容的时候，扩大capacity()为size()的两倍(例外: 当size() == $0$ 的时候，capacity()就变为 $1$ )。这样子，假设push_back() $n$次，$ 2^{m-1} \le n \lt 2^m$，那么最多扩容 $m + 1$ 次，扩容的总时间复杂度为:

$$
O(\sum_{i=0}^{m}{2^i}) = O(2^{m+1}) = O(n)
$$

&emsp;这样一来，$n$ 次扩容就只要均摊 $O(n)$ 的时间复杂度，而且最多只会用两倍的空间，这在时空复杂度上都是十分高效的。以上便是 std::vector 最根本的原理。

# std::vector 的实现细节

## 构造函数赋值和swap()

&emsp;由于是指针管理内部数据，因此交换两个容器就只需要交换3个指针即可，所以是常数时间复杂度。同时，在 C++ 11 以后，移动构造函数、移动赋值函数也只需要移动三个指针即可，因此也是常数时间复杂度，其十分高效。而拷贝构造、拷贝赋值等操作则是一个一个的拷贝元素，时间复杂度为线性。

## size() 和 capacity()

&emsp;前面有讲过，size() 返回的是内部存放的元素的个数，而capacity() 则是已经预留空间的元素数，也已经举例讲过。更详细的说，我们可以把 vector 内部看作维护了三个指针 head,tail,end。其中 head 指向申请的内存块的首地址，end 指向了申请的内存块的尾地址(0-base，所以其实是在最后一个可用元素后面一个的位置)，tail 则是指向当前存放最后一个元素后面一个的位置。此时，size() 就等于 tail - head，而 capacity() 就等于 end - head，满足 head $\lt$ tail $\le$ end 。需要注意的是，[head,tail) 之间的元素都是执行过一次构造函数的，但是 [tail,end) 之间的元素的都是未初始化的，没有执行过构造函数。对于一些复杂类，这样的初始构造函数是十分重要的。

## push_back() 和 emplace_back()

&emsp;前面提到的 push_back() 和 emplace_back()，不同便是在于 emplace_back() 极大地利用了 C++ 11 的特性。push_back() 只能接受一个 T 类型参数，可以是左值或者右值。对于左值，新的元素将通过拷贝构造初始化；对于右值，将会执行移动构造函数来初始化新的元素。而emplace_back()可以接受多个参数，其可以通过完美转发，直接用这些参数作为新元素的构造函数，来构造尾部的新元素。

&emsp;两者在用 T 类型、单个参数初始化的时候表现几乎一致，但是在非 T 类型或者多个参数初始化的时候，emplace_back() 效率显著高于 push_back()。具体来说，如果往尾部添加一个左值 T 对象，那么两者完全一致，都是执行一次拷贝构造。同理，传入一个右值 T 类型的对象，那么两者也同样，都是执行一次移动构造。但是，当传入一系列参数作为新元素的构造参数时，push_back()需要显式的调用 T() 在外部构造，然后vector 内部还会移动构造一次。而emplace_back()则直接在内部构造，只有一次构造，可以省去一次移动构造。

下面将举例分析。

```C++

struct temp {
    unsigned long long x;
    double y;
    char ch[1024];
    temp() = default;
    temp(const temp &) = default;
    temp(temp &&)      = default;
    temp(const char *str,double __y) {
        x = strlen(str);
        strcpy(ch,str);
        y = __y;
    }
}
vector <temp> vec;

vec.push_back(temp("DarkSharpness",1.99));
vec.emplace_back("DarkSharpness",1.99);


```

&emsp;在以上这段代码中，push_back 非 T (本例中为 temp) 类的参数需要先构造出一个 T 类对象 ，而内部则用这个临时对象来移动构造出一个元素。而emplace_back 则直接在内部用这些参数执行构造函数。而如果 T 类对象的移动构造并不是非常高效，甚至可能等同拷贝构造(例如用固定长度的字符数组表示的字符串，正如本例子中的temp，那么此时，emplace_back便可以省去不必要的移动构造过程，大大提升效率。

&emsp;值得注意的是，emplace_back和push_back有强异常保证。这意味着当扩容过程中发生了异常，那么原来的 vector 不会有任何的改变。这看起来没什么，但是这可能会极大的影响到程序的效率。因为在扩容时，需要将老的数据移到新的内存。一般来说，老数据都是没用的，应该调用的是移动构造而不是拷贝构造。但是，如果移动构造函数没有 noexcept 标识符，那么如果在移动某个元素 x 到 y 的过程中发生了异常，x的部分元素已经被移动到了 y ，我们不知道当前移动了多少，不知道如何把这部分数据移回 x ，也不确定移动回去会不会再发生异常。因此，原来 x 元素损坏了，数据丢失。为了满足强异常保证，此时只能使用拷贝构造。注意到了这点，我们对于明显不会抛出异常的移动构造函数应该加上 noexcept，否则可能影响程序效率，特别是对于拷贝开销大而移动开销小的类。详见[cppreference](https://en.cppreference.com/w/cpp/container/vector/emplace_back)。

&emsp;在 C++ 11 之后，笔者认为应当用 emplace_back()替代push_back()以追求更高的效率。如果要减少模板的实例化(emplace_back是模板)并且对象易于移动(例如 std::string)，或者
追求旧版本的兼容性，那么再用push_back()。

## emplace() 和 insert()

&emsp;两者都是在指定的 iterator 前面插入一个元素，具体区别类似 emplace_back() 与 push_back()。当元素在尾部的时候，其同样有一定的强异常保证，即在 vector 尾部插入元素的异常表现完全等同 emplace_back() 与 push_back()。复杂度也是线性的，而且有多种变种，具体请参考cppreference: [insert](https://en.cppreference.com/w/cpp/container/vector/insert)，[emplace](https://en.cppreference.com/w/cpp/container/vector/emplace)。

## reserve() 和 resize()

&emsp;这两个函数比较特殊，其均有改变 vector 的的大小的作用，只不过 reserve() 会预留空间，但是resize() 在预留空间(如果是扩容)的同时，还会对超过原来size()部分新建的元素执行构造函数。

&emsp;很多时候，当用户能明确得知 vector 大小的时候，的确需要 resize() 或 reserve()。然而，并不是所有时候都需要用resize() 或 reserve() 。因为两者会精确改变 vector 的 capacity() ，这很可能使得 vector 高效 push_back() 的特性失效。举例:

```C++
vector <int> vec = {1,2,3};
vec.push_back(1);
vec.push_back(3);
/* 只需要扩容一次，capacity = 6 */

vector <int> tmp = {1,2,3};
tmp.resize(tmp.size() + 1);
tmp.resize(tmp.size() + 1);
/* 需要扩容两次，capacity = 5 */
```

&emsp;就笔者个人经验而言，一般情况下会在 vector 一开始的时候 reserve() 到最大可能的size() 然后 push_back() 不超过这个size() ； 或者 resize() 到指定大小，然后对小于size()的下标的元素，进行读/写操作，类似对一个定长 array 操作。总之，一般要避免连续的 reserve() 和 resize()。

## shrink_to_fit()

&emsp;这是一个比较玄学的函数，用来使 capacity() 缩小到 size()，减小空间占用。除非你明确释放内存(比如之后的vector的 size() 不会再增长只会减小，并且想要节约空间)，否则请不要使用这个函数，其会使得 vector 的优化(即多预留空间)带来的高效率 push_back() 失效。

## clear()

&emsp;这个函数用来清空 vector 内部的元素，它会使得 size() 减小到0，但是 capacity() 不变，这是为了保证高效的 push_back() 操作。参考之前的实现，有一小部分用户 (比如曾经的我) 可能错以为 clear() 就是移动一个指针事情(tail = head) ，以为是常数复杂度。这样的观点是错误的。事实上，vector的clear()操作其实是线性复杂度，正比于内部元素个数。这是因为内部的元素可能是非平凡类，这种时候内部元素在销毁的时候必须执行析构函数(例如 std::map，析构函数需要释放内部的内存，不然会内存泄漏)。当然，这时候又有小可爱(比如我)想要问: 那么对于简单类，比如 int,double，其不需要析构函数来释放空间，那不是会降低效率吗。然而，cpp的编译器(至少gcc)要求了，对于空的析构(比如 ~int())，其会被优化掉，甚至连循环都会被优化掉。其会被优化为空，所以不用考虑这些细节对于效率的影响。

![写在gcc标准库注释里面的](https://s2.loli.net/2023/01/28/Z9kN6oHOedLfMBz.png)

![析构单个pointer](https://s2.loli.net/2023/01/28/E2M41zGa3fyjQHL.png)

# 参考 vector 实现

&emsp;笔者在寒假也写了一个类似 std::vector 的实现，其大致如下，具体可见 [github仓库](https://github.com/DarkSharpness/DarkSharpness/blob/main/Template/Dark/Container/dynamic_array.h)。注意，没有任何可信的保证，程序可能存在很多 bug! 而且没有任何异常处理，push_back() emplace_back() 没有任何异常保证。总之，仅供学习、参考，没有任何保证! 以下是 2023/01/25 16:42 UTC+8 的参考版本，附加了一点点注释。

```C++
#ifndef _DARK_DYNAMIC_ARRAY_H_
#define _DARK_DYNAMIC_ARRAY_H_

#include "../include/basic.h" // 只用到了一个 Log2 函数，可以忽略
#include "../iterator"        // 一个iterator 库

#include <memory>             // std::allocator
#include <initializer_list>   // 初始化列表

namespace dark {

// dark::dynamic_array 就是 std::vector

/**
 * @brief A dynamic %array that can expand itself automatically.
 * In other words, user don't need to consider the allocation and 
 * deallocation of space.
 * Note that if the elements within are pointers, the data pointed 
 * to won't be touched. It's user's responsibility to manage them.
 * 
 * @tparam value_t The type of elements stored within the %array.
 */
template <class value_t>
class dynamic_array : private std::allocator <value_t> {
  protected:
    value_t *head; /* Head pointer to first element. */
    value_t *tail; /* Tail pointer to one past the last element. */
    value_t *term; /* Terminal pointer to the end of storage. */

    /* Allocate memory of __n elements. */
    value_t *alloc(size_t __n) { return this->allocate(__n); }
    /* Deallocate of the memory of head. */
    void dealloc() { this->deallocate(head,capacity()); }

    /* Destory __n elements */
    void destroy_n(value_t *pointer,size_t __n) {
        while(__n--) { this->destroy(pointer++); }
    }

    /* End of unfolding. */
    void reserved_push_back() noexcept {}

    /* Push back a sequence of elements with space reserved in advance. */
    template <class U,class ...Args>
    void reserved_push_back(U &&obj,Args &&...objs) {
        this->construct(tail++,std::forward <U> (obj));
        reserved_push_back(std::forward <Args> (objs)...);
    }

  public:

    /* Construct a new empty %array. */
    dynamic_array() : head(nullptr),tail(nullptr),term(nullptr) {}
    /* Destroy all the elements and deallocate the space. */
    ~dynamic_array() noexcept { this->destroy_n(head,size()); dealloc(); }

    /* Construct a new %array from an initializer_list. */
    dynamic_array(std::initializer_list <value_t> __l) 
        : dynamic_array(__l.size()) {
        for(auto &iter : __l) { this->construct(tail++,std::move(iter)); }
    }

    /* Construct a new %array with __n elements' space reserved. */
    dynamic_array(size_t __n) {
        term = (head = tail = alloc(__n)) + __n;
    }

    /**
     * @brief Construct a new %array filled with given length and element.
     * 
     * @param __n The initial length of the %array.
     * @param obj The element to fill the %array.
     */
    dynamic_array(size_t __n,const value_t &obj) 
        : dynamic_array(__n) {
        while(tail != term) { this->construct(tail++,obj); }
    }

    /**
     * @brief Construct a new %array with identical elements with another %array.
     * Note that no vacancy of %array remains, 
     * which means the new %array's size() equals its capacity(). 
     * 
     * @param rhs The %array to copy from.
     * @attention Linear time complexity with respect to the size() of rhs,
     * multiplied by the construction time of one element.
     */
    dynamic_array(const dynamic_array &rhs) 
        : dynamic_array(rhs.size()) {
        for(const auto &iter : rhs) { this->construct(tail++,iter);}
    }

    /**
     * @brief Construct a new %array with identical elements with another %array.
     * It will just take away the pointers from another %array.
     * 
     * @param rhs The %array to move from.
     * @attention Constant time complexity in any case.
     */
    dynamic_array(dynamic_array &&rhs) noexcept {
        head = rhs.head;
        tail = rhs.tail;
        term = rhs.term;
        rhs.head = rhs.tail = rhs.term = nullptr;
    }

    /**
     * @brief Copy assign a new %array with identical elements with another %array.
     * Note that no vacancy of %array remains, 
     * which means the new %array's size() equals its capacity(). 
     * 
     * @param rhs The %array to copy from.
     * @attention Linear time complexity with respect to the size() of rhs,
     * multiplied by the construction time of one element,
     * Note that there might be an additional time cost linear to the 
     * elements destroyed.
     */
    dynamic_array &operator = (const dynamic_array &rhs) {
        if(this != &rhs) { copy_range(rhs.begin(),rhs.size()); }
        return *this;
    }

    /**
     * @brief Construct a new %array with identical elements with another %array.
     * It will just move the pointers from another %array.
     * 
     * @param rhs The %array to move from.
     * @attention Constant time complexity in any case.
     */
    dynamic_array &operator = (dynamic_array &&rhs) noexcept {
        if(this != &rhs) {
            this->~dynamic_array();
            head = rhs.head;
            tail = rhs.tail;
            term = rhs.term;
            rhs.head = rhs.tail = rhs.term = nullptr;
        }
        return *this;
    }

    /* Swap the content of two %array in constant time. */
    dynamic_array &swap(dynamic_array &rhs) noexcept {
        std::swap(head,rhs.head);
        std::swap(tail,rhs.tail);
        std::swap(term,rhs.term);
        return *this;
    }
    /* Swap the content of two %array in constant time. */
    friend void swap(dynamic_array &lhs,dynamic_array &rhs) noexcept {
        std::swap(lhs.head,rhs.head);
        std::swap(lhs.tail,rhs.tail);
        std::swap(lhs.term,rhs.term);
    }

  public:
    /* Count of elements within the %array. */
    size_t size() const noexcept { return tail - head; }
    /**
     * @brief Count of elements the %array can hold 
     * before the next allocation.
     */
    size_t capacity() const noexcept { return term - head; }
    /**
     * @brief Count of vacancy in the back of the %array  
     * before the next allocation.
     */
    size_t vacancy() const noexcept { return term - tail; }
    /* Test whether the %array is empty */
    bool empty()  const noexcept { return head == tail; }

    /* Doing nothing to the %array. */
    void push_back() noexcept {}

    /**
     * @brief Push one element to the back of the %array.
     * 
     * @param obj The object pushed back to initialize the element. 
     * @attention Amortized constant time complexity,
     * multiplied by the construction time of one element.
     */
    template <class U>
    void push_back(U &&obj) {
        if(tail == term) { reserve(size() << 1 | empty()); } 
        this->construct(tail++,std::forward <U> (obj));
    }

    /**
     * @brief Push a sequnce of elements to the back of the %array.
     * 
     * @param objs The objects pushed back to initialize the element.
     * @attention Amortized linear time complexity with respect to count of objs,
     * multiplied by the construction time of one element.
     */
    template <class ...Args>
    void push_back(Args &&...objs) {
        size_t count = sizeof...(objs); // count >= 2 
        if(vacancy() < count) {
            size_t space = capacity() + empty();
            reserve(space << (LOG2((size() + count - 1) / space) + 1));
        }
        reserved_push_back(std::forward <Args> (objs)...);
    }

    /**
     * @brief Construct one element after the back of the %array.
     * 
     * @param obj The object pushed back to initialize the element. 
     * @attention Amortized constant time complexity,
     * multiplied by the construction time of one element.
     */
    template <class ...Args>
    void emplace_back(Args &&...objs) {
        if(tail == term) { reserve(size() << 1 | empty()); }
        this->construct(tail++,std::forward <Args> (objs)...);
    }

    /* Destroy the last element in the back, with no returning. */
    void pop_back() noexcept { this->destroy(--tail); }

    /**
     * @brief Clear all the elements in the %array.
     * Note that the capacity() of the %array won't shrink.
     * 
     * @attention Linear complexity with respect to size(),
     * multiplied by the deconstruction time of one element.
     */
    void clear() noexcept {
        this->destroy_n(head,size());
        tail = head;
    }

    /**
     * @brief Clear the vacancy of the %array.
     * Note that it will disable the optimization of the %array.
     * 
     * @attention Linear complexity with respect to size(),
     * multiplied by the deconstruction time of one element.
     */
    void shrink() {
        if(tail != term) {
            value_t *temp = alloc(size());
            for(size_t i = 0 ; i < size() ; ++i) {
                this->construct(temp + i,std::move(head[i]));
            }

            this->~dynamic_array();
            term = tail = temp + size();
            head = temp;
        }
    }

    /**
     * @brief Resize the %array to __n.
     * The original data with index smaller than __n won't be touched.  
     * If __n is greater than size(), elements will be appended to the back.
     * These new elements will be assigned by default construction function.
     * 
     * @param __n The new size of the %array.
     * @attention Linear complexity with respect to __n ,
     * multiplied by the construction time of one element.
     * Note that there might be an additional time cost linear to the 
     * elements destroyed.
     */
    void resize(size_t __n) {
        if(__n <= size()) {
            size_t count = size() - __n;
            tail -= count;
            this->destroy_n(tail,count);
        } else {
            reserve(__n);
            size_t count = __n - size();
            while(count--) { this->construct(tail++); } 
        }
    }


    /**
     * @brief Resize the %array to __n.
     * The original data with index smaller than __n won't be touched.  
     * If __n is greater than size(), elements will be appended to the back.
     * These new elements will be assigned by val.
     * 
     * @param __n The new size of the %array.
     * @param val The object to assign the new value.
     * @attention Linear complexity with respect to __n ,
     * multiplied by the construction time of one element.
     * Note that there might be an additional time cost linear to the 
     * elements destroyed.
     */
    void resize(size_t __n,const value_t &val) {
        if(__n <= size()) {
            size_t count = size() - __n;
            tail -= count;
            this->destroy_n(tail,count);
        } else {
            reserve(__n);
            size_t count = __n - size();
            while(count--) { this->construct(tail++,val); } 
        }
    }

    /**
     * @brief Reserve space for __n elements.
     * If __n < capacity(), nothing will be done.
     * 
     * @param __n The space reserved for elements.
     * @attention Linear time complexity with respect to __n,
     * only if __n >= capacity(), multiplied by the time of (de-)construction. 
     * Otherwise, constant time complexity.
     */
    void reserve(size_t __n) {
        if(capacity() < __n) {
            value_t *temp = alloc(__n);
            for(size_t i = 0 ; i < size() ; ++i) {
                this->construct(temp + i,std::move(head[i]));
            }

            this->~dynamic_array();
            term = temp + __n;
            tail = temp + size();
            head = temp;
        }
    }

    /**
     * @brief Resize the %array to __n and assign all the elements by 
     * default construction function of value_t. 
     * 
     * @param __n The new size of the %array.
     * @attention Linear complexity with respect to __n ,
     * multiplied by the construction time of one element.
     * Note that there might be an additional time cost linear to the 
     * elements destroyed.
     */
    void assign(size_t __n) {
        const value_t val = value_t();
        if(__n <= size()) {
            size_t count = size() - __n;
            tail -= count;
            this->destroy_n(tail,count);
            for(auto &iter : *this) { iter = val; }
        } else {
            for(auto &iter : *this) { iter = val; }
            reserve(__n);
            size_t count = __n - size();
            while(count--) this->construct(tail++);
        }
    }

    /**
     * @brief Resize the %array to __n and assign all the elements by val. 
     * 
     * @param __n The new size of the %array.
     * @param val The object to assign the value.
     * @attention Linear complexity with respect to __n ,
     * multiplied by the construction time of one element.
     * Note that there might be an additional time cost linear to the 
     * elements destroyed.
     */
    void assign(size_t __n,const value_t &val) {
        if(__n <= size()) {
            size_t count = size() - __n;
            tail -= count;
            this->destroy_n(tail,count);
            for(auto &iter : *this) { iter = val; }
        } else {
            for(auto &iter : *this) { iter = val; }
            reserve(__n);
            size_t count = __n - size();
            while(count--) this->construct(tail++,val);
        }
    }
  
    /**
     * @brief Copy elements from a range [first,last).
     * Note that the Iterator must be random access iterator.
     * Otherwise, you should provide the count of elements.
     * 
     * @tparam Iterator A random access iterator type.
     * @param first Iterator to the first element.
     * @param last  Iterator to one past the last element.
     * @attention Linear time complexity with respect to (last - first)
     * multiplied by the time of moving one element.
     * Note that there might be an additional time cost linear to the 
     * elements destroyed.
     */
    template <class Iterator>
    void copy_range(Iterator first,Iterator last) {
        return copy_range(first,last - first);
    }

    /**
     * @brief Copy elements from a range [first,last).
     * The number of elements in the range should be exactly __n,
     * or unexpected error may happen.
     * 
     * 
     * @param first Iterator to the first element.
     * @param last  Iterator to one past the last element.
     * @param __n   Count of all the elements in the range.
     * @attention Linear time complexity with respect to __n,
     * multiplied by the time of copying one element.
     * Note that there might be an additional time cost linear to the 
     * elements destroyed.
     */
    template <class Iterator>
    void copy_range(Iterator first,size_t __n) {
        if(__n <= capacity()) {
            value_t *temp = head;
            while(__n-- && temp != tail) { *(temp++) = *(first++); }
            ++__n;
            while(__n--) { this->construct(tail++,*(first++)); }
            this->destroy_n(temp,tail - temp);
            tail = temp;
        } else {
            this->~dynamic_array();
            term = (tail = head = alloc(__n)) + __n;
            while(__n--) { this->construct(tail++,*(first++)); } 
        }
    }


    /**
     * @brief Move elements from a range [first,last).
     * Note that the Iterator must be random access iterator.
     * Otherwise, you should provide the count of elements.
     * 
     * @tparam Iterator A random access iterator type.
     * @param first Iterator to the first element.
     * @param last  Iterator to one past the last element.
     * @attention Linear time complexity with respect to (last - first),
     * multiplied by the time of moving one element.
     * Note that there might be an additional time cost linear to the 
     * elements destroyed.
     */
    template <class Iterator>
    void move_range(Iterator first,Iterator last) {
        return move_range(first,last - first);
    }

    /**
     * @brief Move elements from a range [first,last).
     * The number of elements in the range should be exactly __n,
     * or unexpected error may happen.
     * 
     * @param first Iterator to the first element.
     * @param last  Iterator to one past the last element.
     * @param __n   Count of all the elements in the range.
     * @attention Linear time complexity with respect to __n,
     * multiplied by the time of moving one element.
     * Note that there might be an additional time cost linear to the 
     * elements destroyed.
     */
    template <class Iterator>
    void move_range(Iterator first,size_t __n) {
        if(__n <= capacity()) {
            value_t *temp = head;
            while(__n-- && temp != tail) { *(temp++) = std::move(*(first++)); }
            ++__n;
            while(__n--) { this->construct(tail++,std::move(*(first++))); }
            this->destroy_n(temp,tail - temp);
            tail = temp;
        } else {
            this->~dynamic_array();
            term = (tail = head = alloc(__n)) + __n;
            while(__n--) { this->construct(tail++,std::move(*(first++))); } 
        }
    }

  public:
    /* Return the pointer to the first element. */
    value_t *data() { return head; }
    /* Return the pointer to the first element. */
    const value_t *data() const { return head; }
    /* Subscript access to the data in the %array.  */
    value_t &operator [] (size_t __n) { return head[__n]; }
    /* Subscript access to the data in the %array.  */
    const value_t &operator [] (size_t __n) const { return head[__n]; }

    /* Reference to the first element. */
    value_t &front() {return *begin();}
    /* Reference to the  last element. */
    value_t &back()  {return *--end();}
    /* Const reference to the first element. */
    const value_t &front() const {return *cbegin();}
    /* Const reference to the  last element. */
    const value_t &back()  const {return *--cend();}

    using iterator       = RandomAccess::iterator       <value_t>;
    using const_iterator = RandomAccess::const_iterator <value_t>;
    using reverse_iterator = RandomAccess::reverse_iterator <value_t>;
    using const_reverse_iterator = RandomAccess::const_reverse_iterator <value_t>;

    /* Iterator to the first element. */
    iterator begin()  { return head; }
    /* Iterator to one past the last element. */
    iterator end()    { return tail; }

    /* Const_iterator to the first element. */
    const_iterator begin()  const {return head;}
    /* Const_iterator to one past the last element. */
    const_iterator end()    const {return tail;}
    /* Const_iterator to the first element. */
    const_iterator cbegin() const {return head;}
    /* Const_iterator to one past the last element. */
    const_iterator cend()   const {return tail;}


    /* Reverse iterator to the last element. */
    reverse_iterator rbegin() { return tail - 1; }
    /* Reverse iterator to one before the first element. */
    reverse_iterator rend()   { return head - 1; }

    /* Const_reverse_iterator to the last element. */
    const_reverse_iterator rbegin()  const {return tail - 1;}
    /* Const_reverse_iterator to one before the first element. */
    const_reverse_iterator rend()    const {return head - 1;}
    /* Const_reverse_iterator to the last element. */
    const_reverse_iterator crbegin() const {return tail - 1;}
    /* Const_reverse_iterator to one before the first element. */
    const_reverse_iterator crend()   const {return head - 1;}

};


}


#endif
```
