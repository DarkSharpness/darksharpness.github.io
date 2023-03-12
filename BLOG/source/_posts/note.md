---
title: C++ 部分特性梳理
date: 2023-01-30 15:09:04
updated: 2023-03-12 17:31:24
tags: [C++,基础知识]
categories: [C++,基础知识]
keywords: [C++,基础知识]
cover: https://s2.loli.net/2023/01/30/NTQevOjLCIDX4z3.jpg
mathjax:
description: 一些坑过 DarkSharpness 的C++特性
---
~~咕咕咕，大概寒假结束前写完。欢迎催更。~~

~~寒假早结束了才发现还有一堆没写完~~

# 基础整数类型

C++ 默认的整数类型有很多，例如 short,int,long,long long，对应的都有 unsigned 的版本和 signed 版本，这是大家都所熟知的。值得特别注意的有以下几点:

1. signed/unsigned 默认对应(等价)的是 int/unsigned int
2. char 不一定等于 signed char ! 尽管其他大部分整数类型不写 signed/unsigned 前缀默认的是 signed，但是 char 是一个特例。char 和 signed char 不一定是同一个类型，char 的具体实现取决于编译器。事实上，为了区分字符类型或最小寻址单位的 char 和 整数类型的 signed/unsigned char，这三者往往是互不相同的内置类型。(感兴趣的可以去查查 std::byte，其进一步细化了 char 的功能，std::byte 仅是用作最小寻址单元，不能四则运算，只能最简单的赋值位运算之类的)
3. 即使两个内置的整数类型含二进制位数相同，其也属于不同的类。曾经愚蠢的 DarkSharpness 以为在 Win64 下 long 和 long long 都是 64bit，所以他们是同一个类型，但是他错了。即使其二进制位数相同，计算结果完全相同，甚至可以无开销地静态转化，其依然是两个不同的类型。

![如图所示](https://s2.loli.net/2023/03/12/K7RnAjB5eFX8lwN.png)

# 指针与内存

指针是一个好东西。然而 DarkSharpness 过去被这玩意坑惨了。

指针本质上保存的是一块内存区域的地址，其具体是一个数字。在 delete 的时候，仅仅是把指向的这块内存区域给清空了，并不一定修改指针本身的值。因此，重复 delete 一个指针会出问题(注意，delete nullptr 貌似是不会出事的)。但坑人的不是这个，坑人的在于某些类的析构函数是不保证 delete 后把指针置为空的。因此，这会导致用户手动调用析构函数释放内存带来潜在的风险。以前 DarkSharpness 不懂(ICPC 大作业)，愚蠢地手动调用 map 的析构函数，然后就 RE(Runtime Error)了。事实上，清空一个 map 容器比较安全的方法如下:

```C++
std::map <int,int> recorder;
// Some function here. 
// ...

recorder = std::map <int,int>();
```

在用临时变量进行右值赋值的时候，map 会先清空自身占用的内存空间，然后再直接接管该临时变量的内存空间(为空)。对于其他 STL 容器原理大致相同。


这个故事告诉我们，在不清楚内部实现的情况下，永远不要多次调用析构函数，其只保证对一个合法对象调用一次是不会有问题的。

# 优化? 忧化!

O2 优化是一个好东西，但是其可能导致一些 unexpected error。

例如以下这段代码: (from [hsfzLZH1](https://github.com/hsfzLZH1))

```C++
const int maxn = 1e6 + 3;
struct que{
    int l,r,*s;
    que() {l=1;r=0;s=new int[maxn];memset(s,0,sizeof s);}
    ~que() { delete []s; }
    bool empty(){return l>r;}
    int front(){return s[l];}
    int back(){return s[r];}
    int popfront() {l++;}
    int popback() {r--;}
}q1,q2;
```

乍一看，这个队列没啥问题，除了 popfront() 和 popback() 没有返回参数。然而，如果你用这样实现的 queue 去提交Luogu 上的单调队列模板题，并且开 O2 优化。恭喜你，你可能会收获一个五颜六色的结果 (RE WA TLE MLE 随机组合)。

![如果是 ACMOJ，你会 RE ](https://s2.loli.net/2023/03/12/tIQf1nlkxgBabLr.png)

原因很简单，对于一个有返回参数的函数，由于 O2 优化对于 Undefined Behaviour 会有非常逆天而激进的优化(比较常见的是把 UB 当作不可达分支，认为程序员会保证不会产生导致 UB 的 input)，你将难以预测他会做些什么。本例中，就会出现 RE 等错误。

再分享一个梗图。

![WTF is that!](https://s2.loli.net/2023/03/12/BZldImL63AQnsFc.png)

死循环也是一个类似的 UB，在 clang 的 O1 下被激进地优化掉了。但是由于 int main() 没有返回参数，所以 clang 又自然地把返回语句优化没了，所以在运行完 main() 后，其没有结束程序，而是继续 fall down 到了 unreachable().

![看眼汇编就知道咯，main 被优化空了](https://s2.loli.net/2023/03/12/izS8gbRTyMIteKO.png)

所以，请尽量避免 undefined behaviour 的出现! 常见的 UB 可以参考 [cppreference](https://en.cppreference.com/w/cpp/language/ub)。如果你是 gcc 用户，并且想要尽可能在编译器检测出更多的 UB，请添加 -Wall 指令。

省流: 拒绝 UB ，从你他做起。

# END?

感谢观看，以后有想起什么的话还会再更新的。Special thanks: [hsfzLZH1](https://github.com/hsfzLZH1) , and You !
