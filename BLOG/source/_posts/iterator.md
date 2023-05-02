---
title: 关于 STL iterator 的那些事
date: 2023-03-08 18:11:37
updated: 2023-03-17 11:15:20
tags: [基础知识,STL]
categories: [C++,STL]
keywords: [STL,iterator]
cover: https://s2.loli.net/2023/03/08/v39f5zd4pEeSlQc.jpg
mathjax: false
description: 关于 iterator 的一些想法
---

希望您在阅读本文前对于迭代器、容器等概念已经略有了解，如果您不了解好像也没啥大关系。


# 为什么要有迭代器

> 读懂自己，迭代自己 —— [yyu](https://zhuanlan.zhihu.com/p/82644069)

## 迭代器的起源

笔者最初接触 iterator 是在学习 std::map 的时候。为了从小到大遍历访问 map 里面的元素，我们需要用到 iterator。

iterator，迭代器，顾名思义就是一个可以支持迭代的工具类，其最基础的功能就是通过自我迭代来遍历访问一个容器中的元素，即是对于 **遍历访问容器的内容的操作** 进行了封装。

这样的封装是非常有意义的，对于不同的容器，例如链表、数组、树，遍历访问的方式可能大不相同。如果没有迭代器，那么对于一些需要遍历容器的函数，例如 average() 获取容器中元素的平均值，我们需要针对每个容器数据存储的特点，设计其独有的 average() 函数。这样子不利于代码复用，而且更加容易在编写的过程中出错，甚至还容易过度暴露容器内部的细节，影响了封装性。

事实上，我们重新回到遍历访问容器中每一个元素这一个过程，其本质上可以拆成如下两个原子过程:

1. 对当前的元素(对象)执行操作
2. 将操作的对象修改为下一个元素

由此，我们可以对所有容器添加一些公有的接口，其会返回一个容器独特的迭代器，而这些迭代器都至少具有如下两个功能:

1. 访问当前位置
2. 移动到下一个位置

同时，遍历访问也是有终点的，所以我们还必须要能判断什么时候应该停止迭代，即何时到达了终点的位置，于是便有了迭代器的第三个功能: 

3. 判断两个迭代器是不是同一位置。

基于以上的这些原则，便有了 iterator 这个产物。以上三个功能，在 iterator 上分别对应的是

1. 类似指针,用 \* 或 -> 访问
2. 支持 ++ (有时包括 --) 运算符
3. 可以对同类的 iterator 进行 == 或 != 运算符比较

借助 iterator ，我们可以方便的遍历容器，且无需考虑内部的细节，也可以写出适用于各种有 iterator 的函数，而不必对每个容器重载一遍这个函数。

## 线性容器的迭代器之思

如果 iterator 真的简单如此，那么为什么 vector,array 这类线性容器还要有 iterator 呢？用指针替代不就行了吗？指针的确能完美覆盖以上三个功能，而且性能不会也不可能坏于 iterator ，但 STL 依然为这些线性容器提供了 iterator ，这背后其实有更加深层的原因。

首先，笔者最早体会到的一点，也是比较次要的一点，就是指针本身不是一个类(class or struct)，它是原生数据类型。而 C++ 原生数据类型的右值是不支持 ++ 和 -- 运算符的。例如，假如一个函数 ```int *begin() ``` 返回一个int 类型指针，那么你是不能直接进行 ++begin() 的操作，因为返回的begin() 是一个右值。而对于迭代器，只要重载了 ++ 运算符，即使是返回值右值 begin() 也可以直接进行 ++ 操作。

其次，在使用迭代器的时候，我们可能会想要知道迭代器的类型，例如对于随机访问迭代器，其向前迭代 n 次可以用 += n 来一步完成，而对于链表的迭代器，其向前迭代 n 次则只能一次次地迭代来实现。如果只用指针来作为迭代器，那么我们只能获得迭代器所指向的对象类型这一个信息，并不足以用来判断迭代器的类型。而线性容器 iterator 类则不一样。尽管为了性能，其本质上只存了一个指针，并且前进/后退等操作完全等价于一个指针，但其可以借助 traits 来判断类型，只会在编译期进行类型检测，不会有运行时开销，这也将会在后面讲到。

由此可见，iterator 并不仅仅是对于指针操作的一些包装，其还利用了 traits 和 模板技术，允许在编译期检测 iterator 的信息，便于用户能够针对性的写出不同的函数。

当然，指针的思想也深深地影响了迭代器的设计，目前STL的迭代器大部分在访问元素的操作上都极其类似指针，并且线性容器的迭代器几乎拥有指针所具有的所有性质(例如支持 \* ->)。

## 小总结

迭代器本质上是对于指针操作的一些包装，基于容器内部的细节实现，允许用户通过简单的迭代来遍历容器，正是 yyu 所云的 “读懂自己” 进而 “迭代自己”。同时，比起原生指针，尽管操作方式几乎一致(都可以迭代自增，判等以及访问元素)，但是其借助了 traits 的力量可以在编译器表明自己的迭代器类型，不产生运行开销的同时，便于用户进一步地根据迭代器的类型来实现不同的函数，以不同的方式迭代，可谓是更深一层的 “读懂自己，迭代自己” 。

# 迭代器进阶

## STL 迭代器基础

STL 为几乎所有常用的容器都提供了迭代器，例如 array,vector,deque,map,multimap,unordered_map,set等等。特别地(题外话，可跳)，对于 priority_queue , queue 和 stack 这三个容器适配器，其并没有对应的迭代器，因为适配器本质是限制部分的功能，将一种容器转化为另外一种容器，其只保留了原容器部分必须的功能(例如 stack 只能在 top 处操作)，而原容器并不保证允许迭代器访问。例如 std::stack 的底层容器只需要 size(),empty(),push_back(),pop_back(),back(),emplace_back() 这些接口即可，虽然标准库里面 std::stack 选择的底层容器是 std::deque 支持迭代器访问，但是仅包含这些接口的底层并不是都能支持迭代器。

对于这些迭代器，结合前面讲到的三条性质。

1. 访问当前位置
2. 移动到下一个位置
3. 判断两个迭代器是不是同一位置

其必然可以支持以下的几种操作:

``` C++
struct test { int x; }

iterator <test> i,j; // 伪代码,仅表示 i,j 是某迭代器

*i , i->x;      // 1. 访问当前位置元素,操作类似指针
i++ , ++i;      // 2. 迭代自增,有的迭代器支持自减
i == j, i != j; // 3. 判断是否相等,有的支持比较( < 等)

```

对于常见 STL 容器，我们一般可以通过成员函数 begin() 或 end() 来获得其头部迭代器和尾部迭代器，分别指向 第一个元素的位置 和 最后一个元素后面的位置(这也是为什么不能访问 end() 指向的元素) 。其类型为 容器名字::iterator ，例如对于 ```vector <int>``` ，其迭代器类型为 ```std::vector <int>::iterator``` 。对于 const 的容器，其 begin() 和 end() 返回的是const元素迭代器，即容器名字::const_iterator，其本身可以修改，迭代(不同于 const iterator)，但是其指向的元素不能被修改(类似 const 数据类 的 非const 指针，例如 const int \*)。

这时，肯定有小可爱要问了: 那么对于非 const 的容器，我们怎么获得 const_iterator 来安全访问呢？这非常简单，只需要用 cbegin() 和 cend() 成员函数来访问即可。

对于部分容器，其也支持反向迭代器，即可以从最后一个元素到第一个元素反向迭代。显然地，这样容器的正向迭代器必然支持反向迭代操作。

下面将再次结合三条性质进行分析

### 1. 访问当前位置元素的性质

对于序列容器，例如 vector,deque,list，一般来说其返回迭代器访问元素的方法完全等同于该类型的指针。下面以 list 为例。

``` C++
struct test { int x; }
std::list <test> l = {test{1},test{2}};
std::list <test> iterator i = l.begin();
// 一般C++11 以后会用 auto
auto j = l.cbegin(); // const_iterator

*i; // 返回一个 test 类型可读写引用(test &)
*j; // 返回一个 test 类型只读引用(const test &)
i->x; // 相当于 (*i).x , 类似指针,类型为 int &
j->x; // 同上,但类型为 const int &

```

对于 set,multiset,unordered_set 和 unordered_multiset ，尽管其也有迭代器，但由于修改迭代器所指的元素可能会破坏内部的性质(红黑树 / hash)，因此其 iterator 和 const_iterator 本质上都是 const_iterator 都是只读不可写的。

对于 map 和 unordered_map 等，其也类似 set ，对于键值 key 不能随便修改，但是允许修改 key 对应的 value。因此，其迭代器解引用的类型是 ```std::pair <const key_type,value_type>``` ，对于 iterator 变量 i 我们可以通过 i->second 来修改当前位置 key-value pair 的 value 值。而 const_iterator 则什么都不能修改，是 read-only。

### 2. 移动到下一个位置的性质

该性质取决于迭代器的性质。

对于随机访问迭代器 (Random Access iterator)，顾名思义可以随机访问，因此其不仅可以支持最基础的 ++ ，还允许进行 -- 甚至是 + n 和 - n 以及迭代器之间相减这类操作，其本质原因是底层存储连续且有序 ，例如元素本身连续存储的 vector,array 等，或者指向元素的指针连续存储的 deque 等。

对于双向迭代器 (Bidirectional iterator)，其显然可以反向迭代进行 -- 操作，但是其略弱于随机访问迭代器，不能 + n 或者 - n。代表性的有 map,list 的迭代器

对于前向迭代器 (Forward iterator)，其只有最基础的前向迭代 ++ 操作。代表性的有 forward_list,unordered_map 的迭代器。对于此类容器，其显然也不会有反向迭代器。

### 3. 判断所指是否相等的性质

其同样取决于迭代器的性质。

对于随机访问迭代器，其往往也会支持 < > 等比较操作，本质上是比较两个元素的相对下标之差，类似指针的比较。

而对于其他的迭代器，一般都不会支持除了 != 和 = 之外的比较运算符。



## iterator_traits

~~咕咕咕...(待更新，欢迎催更)没人催也更新~~

std::iterator_traits 本质上是一种模板类，用来提供一个统一的接口，便于标准库基于迭代器的类型来实现不同的算法。而作为用户，只需提供一个含有一些特定的 typedef 的 iterator 类，或者是手动模板偏特化。

其定义大致如下:
```C++
template <class Iter>
struct iterator_traits;
```

对于提供的 Iter 类，其必须要有以下五个成员类
1. difference_type
1. value_type
4. pointer
5. reference
1. iterator_category

其中，difference_type 是两个 iterator 做差后的类型(如果可以做差的话)。value_type 是解引用后得到的 value 类型。pointer 是指向 value 的指针类型。reference 是 value 类的引用类型。而 iterator_category 是 iterator 的类型。

这里有必要单独讲一讲 iterator_category 。其具体分为 input_iterator_tag , input_iterator_tag , forward_iterator_tag , bidirectional_iterator_tag , random_access_iterator_tag 。其含义比较直观，分别对应的是只读，只写，只能向前迭代，可以双向迭代，和可随机访问迭代。不难看出后面三个是一个比一个更强的，都至少需要前面一级作为基础，因此后三者应该呈继承关系。事实上，STL 里面也是这么实现的，下面是标准库的一些片段。(注释没改，来自 gcc 12.2.0 的原始片段)

```C++
  ///  Marking input iterators.
  struct input_iterator_tag { };

  ///  Marking output iterators.
  struct output_iterator_tag { };

  /// Forward iterators support a superset of input iterator operations.
  struct forward_iterator_tag : public input_iterator_tag { };

  /// Bidirectional iterators support a superset of forward iterator
  /// operations.
  struct bidirectional_iterator_tag : public forward_iterator_tag { };

  /// Random-access iterators support a superset of bidirectional
  /// iterator operations.
  struct random_access_iterator_tag : public bidirectional_iterator_tag { };
```

基于以上的知识，我们也可以为我们自己写的容器的 iterator 加上一些成员类型，从而使得其可以被 iterator_traits 识别。例如对于一个存储 int 的 array 类的 iterator ，其定义应该大致如下:

```C++
class iterator {
  public: // 注意，成员类需要 public! 否则无法识别
    // 为了C++ 11 之前的兼容性，可以用typedef 代替 using

    using difference_type   = std::ptrdiff_t;
    using value_type        = int;
    using pointer           = int *;
    using reference         = int &;
    using iterator_category = std::random_access_iterator_tag ;

  private:
    // 后面是实现部分,省略
    // ...
};

```

可以看出，比起原本的实现，只需额外 typedef 或 using 声明几个模板类，即可满足迭代器被 iterator_traits 识别，从而可以用于标准库里面的某些函数。

这时候，聪明的您可能又要问了，那么原生指针不就没法支持了吗。您说得对，但是标准库对于原生的指针有模板特化，依然可以被 iterator_traits 所识别。

总结下来，iterator_traits 是对于原生指针和自定义的 iterator 的一层包装，用来提供一个统一的接口，便于 STL 实现针对性的算法。

而作为用户，也可以利用这一特性，实现针对性的算法。举例: 让迭代器前进 n 步。对于随机访问迭代器，只需 += 即可，但其他的迭代器需要一步一步的向前走。这时，可以借助 iterator_traits。

```C++
/* 如果是其他 input_iterator, 可以转换为基类 */
template <class Iter>
void advance_n(Iter &i,size_t __n,std::input_iterator_tag) {
    while(__n--) ++i;
}

/* 随机访问迭代器要特殊重载 */ 
template <class Iter>
void advance_n(Iter &i,size_t __n,std::random_access_iterator_tag) {
    i += __n;
}

template <class Iter>
void advance_n(Iter &i,size_t __n) {
    return advance_n(i,__n,typename iterator_traits <Iter>::iterator_category ());
    // 空类型 iterator_category 无开销
}

```

如果您对这个感兴趣，可以去看看 cppreference 上关于 [iterator_traits](https://en.cppreference.com/w/cpp/iterator/iterator_traits) 的解释。

# 迭代器的特殊实现

听了 DarkSharpness 讲了这么多，你难道不想要给自己写的容器实现一个迭代器吗。先别急，先看看我们要实现哪些:

- iterator
- const_iterator
- reverse_iterator (bonus)
- const_reverse_iterator (bonus)

先不考虑 bonus，你静下心来想想，便会发现 iterator 和 const_iterator 的行为几乎一模一样，唯一的区别在于，后者指向的对象是不可修改。仔细想想，如此重复的代码该如何提高复用 ? 你可以先思考，也可以直接看下去。

一开始，DarkSharpness 的想法是用 const_cast 等手段，先实现一个 iterator_base ，通过让两种 iterator 继承 iterator_base，来实现代码复用。这样子的弊端是，存在一个不太安全的 const_cast，以及还有额外实现 const_iterator 和 iterator 两个类型之间的比较函数，转换函数，这样子还是很不美观的。

这时候，神一般的 Wankupi 降临了。他使用了模板的思路，优化了 iterator_base ，通过传递模板参数 bool is_const 来判断是不是 const_iterator ，并且用到 C++ 11 以后的模板推导工具 std::conditional_t (不会自行百度qwq) 来实现当模板参数 is_const 为 false ，指针为 T \* ，反之为 const T \* ，其中 T 为解引用后的类型。

不过 Wankupi 的实现还是用来 iterator_base 和 iterator_common 两种类，这实在是太麻烦了。事实上，我们可以重新思考并抽象 iterator 的三个核心操作。

- 访问当前位置
- 移动到下一个位置
- 判断是否在同一位置

访问当前位置的操作，已经通过前面的 std::conditional_t 来解决了 const 与否的问题。

移动到下一个位置，其实本质上就是一个前进/后退函数。例如对于原生指针作为迭代器，其 advance 函数大致如下:

```C++
using pointer = int *;
void advance(pointer &__p,bool dir) {
    if(dir) ++p;
    else    --p;
    /* dir = 1 前进 || dir = 0 后退 */
}
```

当然，也可以借助模板以及 C++ 11 以后的一些工具 (std::true_type 之类) ，将 dir 传入模板参数，让 dir 不同的时候进入不同的重载，进而生成更优的代码，效率上完全等价于真实指针，这里暂时不多提了，感兴趣可以看看 DarkSharpness 是如何实现随机访问迭代器的 advance 函数 [点我点我点我](https://github.com/DarkSharpness/DarkSharpness/blob/main/Template/Dark/Container/iterator.h)。

判断是否在同一位置，其实本质上是判断指针指向的变量地址是不是同一个，此时不用成员函数实现反而会更优雅而简洁，只需对 iterator 类提供一个对外访问内部指针的接口即可。

事实上，一旦把前进过程用一个函数 advance 抽象化了以后，你会发现，四种 iterator (包括反向的两种) 的操作竟然是出奇的一致 :

- 访问当前位置 (const / non-const)
- 移动到下一个位置 (reverse / normal)
- 判断是否在同一位置 (同类型比较)

参考之前 const 模板化的过程，我们不难联想到 : 我们可不可以把 reverse 与否也扔到模板里面? 这个主意真的是太妙了。在把移动过程抽象为一个函数 advance 之后，这样的实现变得可行，其可以使得代码复用四次，复用率高了不是一点点。

参考代码如下:

```C++


/* Advance 函数需要自己实现，取决于容器内部结构 */
template <bool dir> /* 0 后退 || 1 前进 */
void advance(pointer &ptr);


/* 
    is_const 是否是 const 版本的迭代器
    dir = 0 反向迭代器 || 1 正向迭代器
*/
template <class T,class is_const,bool dir>
class iterator {
  public:
    using U = std::conditional_t <is_const,const T,T>;
    using pointer = U *;
    pointer ptr;

    /* 这里以 ++ 和 -- 为例,其他都差不多 */

    iterator &operator ++ (void) 
    { advance <dir>  (ptr); return *this; }

    iterator &operator -- (void) 
    { advance <!dir> (ptr); return *this; }

    /* 同向迭代器,允许non-const -> const,赋值同理 */
    iterator(const iterator <T,false,dir> &rhs) :
        ptr(rhs.ptr) {}


    /* 用于 iterator 比较或其他操作 */
    pointer base() const { return ptr; }

};


/* 同向迭代器可以比较，const 情况无所谓 */
template <class T,bool k1,bool k2,bool dir>
bool operator == (const iterator <T,k1,dir> &lhs,
                  const iterator <T,k2,dir> &rhs) {
    return lhs.base() == rhs.base();
}

```

就笔者个人而言，这样的写法真的是简洁而清晰，即使是对于原生指针的包装，在运行效率上也几乎完全等价于未包装的情况(模板)，没有任何开销。事实上，这些代码编译器大概率会直接 inline 掉，此时就真的完美等价于原生指针了。

# END

感谢 [yyu](https://apex.sjtu.edu.cn/members/yyu) 的~~友情~~客串。

感谢 [Wankupi](https://www.wankupi.top) 提供的关于 iterator 和 const_iterator 模板化的思想。

感谢您认真看到了最后！
