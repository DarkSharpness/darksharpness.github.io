---
title: 高精度模板(基于双模数NTT)
date: 2022-11-19 15:59:10
updated: 2022-11-19 15:59:10
tags: [算法,模拟,数学,数论]
categories: [算法,数学]
cover: https://raw.githubusercontent.com/DarkSharpness/Photos/main/Touhou/pixiv_664449380.jpg
top_img: https://raw.githubusercontent.com/DarkSharpness/Photos/main/Touhou/pixiv_664449380.jpg
keywords: [算法,数学,高精度]
decription: 用NTT实现乘法的高精度模板.
mathjax: true
---
由于作业要求，我需要写一个压位的高精度的模板，并且需要通过FFT / NTT对于乘法进行优化。

说实话，我是不太喜欢~~(不会)~~写这种又暴又难调的的模板的，这对我来说还是非常具有挑战的。

如果你还不会 FFT/NTT，不妨看看 [这个(from OIwiki)](https://oi-wiki.org/math/poly/ntt/)

本文将侧重于基于双模 NTT 的乘法是如何实现，对于其他的细节也会提到。

## 基本思路

由于我担心FFT压位会爆double精度(事实证明压到 $10^4$ 做 $10^{200000}$ 的乘法也不会炸精度，而且效率上暴打双模数NTT)，我选择了双模数 NTT + 压6位(每个单元上限 $10^6 - 1$)。

具体如下:
首先，设置了一个class NTT_base类来保护内部的静态数据(constexpr static)。

然后大整数类名为 class int2048 . 它继承了 std::vector &lt;ull&gt; 和 NTT_base 的所有数据。(我们约定using ull = unsigned long long).

### 双模数 NTT 的分析

由于单模 NTT 能够允许的范围为 $ n * (base - 1)^2 < mod $ ，$n$ 为压位后乘法结果的长度 (且必须向上取整到 $2 ^ k$ )。其中，模数 $mod$ 必须要小于 unsigned int 的范围，是因为中间计算的过程中会出现 $ (a \times b) $ % $ mod $的表达式，而$ a \times b $ 最多不能超unsigned long long 。因此，当 $ n = 10^5 $，$ base $ 只能选择为 100 ，这样几乎没有压位的效果 )。

回到 NTT 算法本身，其用于替代 FFT 是为了替代 FFT 中可能出现的 浮点精度丢失 以及 浮点计算慢 等问题。我们通过 NTT 得到某一位的值实际上是: 这个位置通过多项式乘法得到的一个值对mod取模。又因为得到的值在不压位的情况下一般是小于 mod 的，所以正确性得以保证。

抽象一下，就是假设仅仅通过多项式乘法，我们得到某一项乘出来的系数为 $z$，假设模数为 $M$ ，我们通过 NTT 得到的是同余方程 $z \equiv x \ (mod M)\ (x < M)$的解 $x$ ，又因为 $z < M$，所以我们得到的 $x$ 就是所求的 $z$。

如果采用两个模数，则通过如下同余方程组 :

$$
\begin{cases}

z  \equiv x\ (mod M_1)\\
z  \equiv y\ (mod M_2)\\

\end{cases}
$$

选取两个不同的质数 $M_1,M_2$ 作为 NTT 模数 ，只要 $gcd(M_1,M_2) = 1$，由中国剩余定理，我们可以通过两次 NTT 得到 x 和 y 来还原任意一个 $\ z < M_1 \times M_2 $ 。

这样一来，我们可以使得可选的 z 的上限拓展到了 $M_1 \times M_2 - 1$，实测可以达到
所以根据双模 NTT 能够允许的范围为 $ n * (base - 1)^2 < M_1 \times M_2 - 1 $ 即使 $base$ 选为 $10^6$ , $n$ 的上限也能达到 $ 2^{22} $，对大部分问题还是绰绰有余的。

### NTT_base的实现

NTT_base声明了所有NTT相关的函数以及数据，都是静态储存 (相当于全局函数/变量，而且编译期确定)。

同时，为了防止用户自己定义一个NTT变量，我将其构造函数放在了 protected 里面，这样就可以避免用户直接使用 NTT_base 的变量，而只能通过继承来安全的访问NTT_base 的数据。其本质只是对于内部数据/函数的保护，防止被误用，确保安全性。

值得注意的是，对于编译期常数的除法和取余，编译器会展开优化为一系列更快的位运算。因此，我们需要实现两份 NTT 以及 fastPow(复制黏贴，仅仅模数不同)，这样相比传参数来决定 NTT 是采用 $M_1$ 还是 $M_2$，要快将近一倍(如下图,题目来自交大的[ACMOJ 1754](https://acm.sjtu.edu.cn/OnlineJudge/problem?problem_id=1754) )。

![确实快了不少,当然还是打不过FFT](https://raw.githubusercontent.com/DarkSharpness/Photos/main/Images/ACMOJ1754.png)

最后是一个小优化:注意到了 NTT 中每次单位根的大小都是固定的(取决于 NTT 长度 $len$ 中 $len = 2^k$ 中的 $k$)，我们可以预处理这些值，一共就 2(INTT/NTT) * 2(两个模数) * 22(最大的k) 个数，故打表处理。逆元同理。

还有就是当数据量较小的时候，应该避免使用 NTT 而是使用暴力乘法或者分治递归实现，这个笔者暂时还没考虑好应该设置为多少。(如果您有测试结果与实现，欢迎在评论区留言awa)

不考虑 FFT 和 NTT 本身的时间差距，压相同位的情况下，双模 NTT 的理论常数应该是 FFT 的两倍

代码如下:

```C++
/**
 * @brief NTT related data.
 * You can't apply for an NTT_base object. 
 * 
 */
class NTT_base {
  protected:
    NTT_base() = default;
    static inline uint64_t fastPow0(uint64_t base,uint64_t pow);
    static inline uint64_t fastPow1(uint64_t base,uint64_t pow);
    static void NTT0(uint64_t *A,uint32_t len,bool type);
    static void NTT1(uint64_t *A,uint32_t len,bool type);
    static inline void reverse(uint64_t *A,uint32_t *rev,uint32_t len);
    static inline std::vector <uint32_t> getRev(uint32_t len);
    static inline uint64_t getMult(uint64_t A0,uint64_t A1,uint64_t inv);
  

    constexpr static uint64_t mod[2]  = {2281701377,3489660929}; // mod number
    constexpr static uint64_t lenb    = 6;   // base len in decimal
    constexpr static uint64_t base    = 1e6; // base of int2048 = 10 ^ lenb
    constexpr static uint64_t initLen = 2;    // initial length reserved
    constexpr static uint64_t MaxLen  = 1 << 21; // Maximum possible NTT length
    // constexpr static uint64_t rate    = 3;    // compressing rate
    constexpr static uint64_t NTTLen  = 1e6;  // pow(NTTLen,rate) = base
    constexpr static uint64_t BFLen   = 1e9;  // Brute Force length
    // constexpr static uint64_t root[2][2] = {     // root and inv root
    // 3,(mod[0] + 1) / 3,3,(mod[1] + 1) / 3
    // };  // common root
    constexpr static uint64_t unit[lenb] = { // units below base
        1,
        10,
        100,
        1000,
        10000,
        100000
    };
    constexpr static uint64_t root[2][2][22]= {
        2281701376,344250126,483803410,617790083,2023592065,
        216937880,123697435,1639385633,1301610063,865646229,
        1780348903,799681555,977546242,1286750706,1294996786,
        2270548020,451618310,637539285,231852688,1783582410,
        1346120317,1057547068,

        2281701376,1937451251,582229479,1778233327,996068929,
        533126167,1540362740,1845123106,1922965124,1184734049,
        369448383,1732276489,1444283332,92283190,2059450554,
        114788634,1156211696,2033086166,1274602630,1845241368,
        656109765,1987373021,

        3489660928,1841841630,1054308003,1513900834,1424003439,
        3290428437,2792923286,424291397,1938306374,731827882,
        340708175,1005229295,3231347667,962117947,1030533309,
        725028937,3369885469,72934420,758575992,3373763992,
        1882435536,1143890192,

        3489660928,1647819299,1761617041,724177331,38202934,
        2302762011,917171561,3348395406,763600137,382076615,
        417640332,3318291862,2451874772,3398023446,1583677827,
        997063351,2673837393,2327172879,845347823,1749721614,
        2180195085,87513231,
    };
  
    /**
     * @brief Least length for NTT.
     * Note that 2 * threshold * base should be less than 2 ^ 64. 
     * 
     */
    constexpr static uint64_t NTT_threshold = 0;
};



```

## 暂时先摸了