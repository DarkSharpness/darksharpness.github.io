---
title: KMP算法 (已失效，请勿参考)
date: 1900-01-01 00:59:52
updated: 2022-09-13 17:02:10
tags: [KMP,字符串Hash,字符串,算法]
categories: [算法,字符串,KMP]
cover: https://s2.loli.net/2023/01/28/iUjK4Ef7sGDzZCl.png
keywords: [KMP,字符串Hash]
description: 一个简单的普及难度字符串算法:KMP
mathjax: true
---

这篇文章写的很烂，请不要作为参考! 仅留档用!

## 问题引入

前置知识与记号约定：[字符串基本知识与约定](/2022/09/13/String/index/)

### 题目([Luogu 3375)](https://www.luogu.com.cn/problem/P3375)

![Luogu3375](https://s2.loli.net/2023/01/28/iUjK4Ef7sGDzZCl.png)

### 问题分析

给定两个字符串s1,s2。记$s1[i$ ~ $j]$为s1的第i到j个字符构成的子串(例如:$s1$="abcab"  $s1[0$ ~ $2]$ = "abc")
设s1长度为$n$，s2长度为$m$，满足$n>=m$

~~我们简单分析下数据来猜测复杂度，~~数据总量 $n,m$ 达到了$1e6$的级别，说明该算法必须是O($nlogn$)且常数较小 或者 O($n)  的复杂度。

考虑最朴素的暴力做法:枚举s1中每一个位置 i 作为起始位置，然后将对应s1子串$s1[i $ ~ $i+m-1] $与 s2 逐字符比较。
此时 i 枚举的范围为$[0,n-m]$，单次比较时间复杂度为O($m$)，所以整体时间复杂度为O($n*m$)，这样明显不能通过数据。

## *Hash算法巧解简化版本

为了便于理解，我们先简化题目，先不考虑"border"。

简化版本 : 求出和 s2 相等的 s1 的子串 的起始位置。
极简主义 : 求出 s1 中 s2 出现的位置。

既然暴力无法解决，我们应当考虑一些优化。通过字符串Hash优化，我们可以把单次比较的时间复杂度降低到O(1) (虽然有概率错误)

字符串Hash是把字符串看作一个 B 进制的数字，再对一个数取模，将一个较大的值域范围 映射 到一个小的值域范围里面。

### 具体实现

设字符串str长度为n，记$f(str)$ = $\sum_{t=0}^{n-1} str[t] * B^{n-1-t} $ ，表示str的B进制数大小 。

这样，我们成功将字符串通过B进制与一个数对应，且该数仅对应唯一一个字符串str。同理，对于str的子串$str[i  $ ~ $j]$，其本身对应的B进制数的大小为$\sum_{t=i}^{j} str[t] * B^{n-t}$。这样，对于两个长度相等的字符串，判断其相等可以转化为判断其 $f$ 函数是否相等。

可以看出，对于str的长度为i的前缀子串$str[0$ ~ $i]$，$f(str[0 $ ~ $i])$可以由递推求出，而$f(str[i $ ~ $j])$ = $f(str[0 $ ~ $j])$ - $f(str[0 $ ~ ${i-1}])*B^{j-i+1}$ 求出，我们只需要O(1)的预处理就可以快速得到子串$str[i$ ~ $j]$的 $f$ 函数。但其值往往过大超出了long long范围不易表示和计算，因此我们通过取模，我们可以把较大的值域映射到一个更小的值域内，再比较判断相等。将f对M取余，得到$Hash(str) = f(str) mod M$。

然而值域缩小带来的是可能出现两个不同的字符串Hash函数相同，即Hash碰撞。在均匀分布的情况下(一般取B和M互质)，Hash碰撞的概率为$1/M$。为了减少Hash碰撞的概率，我们可以多次取余来验证字符串是否相等，进而减少Hash碰撞概率。

下面是演示代码(仅供演示，若想要更好的板子请在站内搜索关键词 Hash 找到相关文章):

```C++
const int N = 1e6 + 10;//视题目而定
long long Hash[N];//Hash[i]为str[0~i-1]的Hash值
long long base[N];//base[i]为B的i次幂对M取mod
//预处理出str的Hash值
void prework(const string& str,
             long long B,
             long long M) {
    Hash[0] = (str[0]-'A') % M; //本题中只有大写字母,所以这么处理
    base[0] = 1;
    //一般来说 str[0] <= B < M 不用 %M 
    for(long long i = 1 ; i < str.length(); ++i) {
        base[i] = (base[i-1] * B) % M;
        Hash[i] = (Hash[i-1] * B + str[i-1]) % M; 
    }
    return ;
}
//求出str[l~r]的Hash值
long long getHash(const string& str,
                  int l,int r,
                  unsigned long long M){
    static long long tmp;//该变量只会被声明一次
                         //类似全局变量 
    tmp = Hash[r]-Hash[l-1]*base[r-l+1];
    return tmp >= 0 ?
           tmp % M : (tmp % M) + M;
    //相当于if结构，?前表达式成立则返回冒号左，否则返回右
    /*
        补充知识: C++ 负数取mod结果 为负
        例如 5 % 3 = 2
            -5 % 3 = -2;
        所以为防止出问题，如此处理
    */
}
```

### 亿些细节

一般来说,B取略大于字符值域范围质数。本题中字符为大写字母，值域为26。本题中不妨取 $B = 29$。一般情况下字符(无限制)值域为128，此时可以取 $B = 131$。

而常见的mod数会选取int范围内的较大质数，常见的有 $1e9+7,1e9+9,998244353$。特别地，我们可以利用unsigned long long的自然溢出来避免低效的取mod运算(其慢于自然溢出)，只需把上面代码中的Hash和base改为unsigned long long类型，并且不再对M取mod即可。

~~为了避免常见大质数被毒瘤出题人卡Hash或者运气不好哈希碰撞~~，这里推荐使用多次Hash组合加以判断，或者使用其他的大质数(例如1090000361,1060000037)。

### 局限性

Hash算法的确可以优异地求解本题的简化版本，只有 O($n+m$) 的时间复杂度。

然而，我们回到题目本身，其还要求求解s2每个前缀的最长"border"。根据题意，"border"是指对于一个长度为l的字符串$str$，其前缀子串$str[0$ ~ $j] $ $(0 <= j < l-1)$，该子串满足$str[0$ ~ $j]$ = $str[i-j$ ~ $i]$而题目要求的是求出每个前缀子串最长的 "border" 的长度。此时，字符串Hash就无能为力了。

## KMP算法

为了解决这一问题，K(kan)M(mao)P(pian)算法诞生了。其由Knuth，Morris，Pratt三人发明，可以有效地解决上述问题。

### 核心思想——维护nxt数组

KMP的精髓在于其next数组。对于s2，我们记录一个next数组$nxt[i]$ $(-1 <=nxt[i]<= i-1 )$，$nxt[i]$ 表示最大的满足$s2[0$ ~ $nxt[i]]$ = $s2[i-nxt[i]$ ~ $i]$，即上面所说的前缀子串$s2[0$ ~ $i-1] $的最长的"border"的长度。***例如*** : 字符串"ABCABCDA"，其$nxt[4] = 1$(因为"ABCAB"中前缀"AB"=后缀"AB"，且没有更长的满足的了)，同理$nxt[7] = 0$，$nxt[6] = -1$(因为"ABCABCD"不存在前缀子串等于后缀子串，所以$nxt$值为$-1$)。

#### **我们应该如何维护nxt[i]呢？**

如果暴力维护的话，对于s2每一个子串 $s2[0$ ~ $i ] $ $ (0<=i<=m-1)$，需要枚举前缀子串的位置 $ j$ $(0 <=j<= i-1)$，比较 s2 的每个子串$s2[0$ ~ $j]$和$s2[i-j$ ~ $i]$是否相等。即使运用了字符串Hash能在O($1$)的时间内进行比较，处理出整个$nxt$数组还是O($m^2$)的时间复杂度。

我们~~仔细观察(百度百科)~~，发现nxt[i]其实可以递推求解：首先我们可以发现，若$s2[0$ ~ $j]$ = $s2[i-j$ ~ $i]$，那么必然有$s2[0$ ~ $j-1]$ = $s2[i-1-j+1$ ~ $i-1]$。可以看出，从$i-1$ 到$ i$，若$nxt[i] >0$，则 $str[0$ ~ $i]$ 的最长"border"子串必然是由 $s2[0$ ~ $i-1]$ 的一个"border"加上str[i]得到。***例如***：字符串str = "CABCCABCA"，其$nxt[7] = 2$，而对于$i = 8$ , 枚举$str[0$ ~ $7]$的每个"border"："CABC" "C"，发现不存在前缀子串"CABC"+"A"，但是存在 "C"+"A"="CA"，所以$nxt[8] = 1$。因此，我们只需验证$s2[0$ ~ $i-1]$的每个前缀子串"border" 后一位是否等于$s2[i]$，即可求出$nxt[i]$。

**我们如何枚举$s2[0$ ~ $i]$的所有"border"呢？**

先说结论：$s2[0$ ~ $i]$所有"border"的长度由长到短依次是 : $nxt[i] , nxt[nxt[i]] , nxt[nxt[nxt[i]]] ...... ,-1$。

首先$s2[0$ ~ $nxt[i]]$定义为i的最长border很好理解，***下证 :*** 若$nxt[i]!=-1$，则不存在$nxt[nxt[i]] < j < nxt[i] $  使得 $s2[0$ ~ $j]$ 为 $s2[0$ ~ $i]$ 的一个"border"。

***证明***：由题意，此时，$ s2[0$ ~ $nxt[i]]$ = $s2[i-nxt[i]$ ~ $i] $ ，且 $s2[0$ ~ $j]$ = $s2[i-j$ ~ $i]$，所以$s2[0$ ~ $j]$ =  $s2[i-j$ ~ $i]$ = $s2[i-j-(i-nxt[i])$ ~ $i-(i-nxt[i])]$ = $s2[nxt[i]-j$ ~ $nxt[i]]$。此时由定义，$nxt[nxt[i]] = j$ ，与$ j > nxt[nxt[i]]$ 矛盾。

#### 代码实现

```C++
const int N = 1e6 + 10;
int nxt[N];
char s1[N];
char s2[N];

//O(m)预处理nxt数组
void prework() {
    int j = -1;
    nxt[0] = -1;
    int len = strlen(s2);
  
    for(int i = 1; i < len; ++i) {
        while(j!=-1 && s2[i] != s2[j+1]) j = nxt[j];
        //这一步对应的是由长到短枚举s2[0~i-1]的border
        //第i次循环开始前,j=nxt[i-1]均成立
        //若匹配不成功,则j = nxt[j]尝试匹配一个最长的border
  
        if(s2[i] == s2[j+1]) ++j;
        //如果匹配成功而跳出循环,此时s2[0~i]最长border就是j+1
        //反之,说明这样的border不存在,因此j=-1不加
  
        nxt[i] = j;//此时nxt = j
    }
}
```

### 复杂度证明

看了代码,你可能觉得for循环中嵌套了一个while,这不是要 $O(m^2)$ 吗?
~~你先别急~~。我们考虑while中操作的对象$j$，其初始值为-1，在每次while循环中至少减少1（显然$nxt[j] < j$），最多减少到-1。而在一次for循环中，$j$最多增加1$($++$j$)。因此，在整个过程中，$j$在while循环中减小操作的次数不会多于++$j$操作的次数，而++$j$至多$m-1$次，所以执行while循环中的总次数小于$2*(m-1)$，因此这个预处理prework的时间复杂度为O($m$)级别。

### 求解出现位置

该操作类似求解nxt的过程，只需将prework中的if和while的判断改为 $s1[i] != s2[j+1]$即可。同时，若此时$j$达到了$s2$的长度$m$，那么此时$i$为$s2$在$s1$中出现的一个子串的末尾位置，可以通过计算输出其头部位置，并且令 $j = nxt[j]$ 继续循环匹配寻找下一个位置。

类似地，我们可以证明该操作时间复杂度为O($n$)，所以该算法整体的时间复杂度为O($n+m$)，十分高效。

## 附：AC代码

```C++
#include<bits/stdc++.h>
using namespace std;
typedef long long ll;
const int N = 1e6 + 10;
int nxt[N];
char s1[N];
char s2[N];
int len1,len2;

//O(m)预处理nxt数组
void prework() {
    int j = -1;
    nxt[0] = -1;
    for(int i = 1; i < len2; ++i) {
        while(j!=-1 && s2[i] != s2[j+1]) j = nxt[j];
        //这一步对应的是由长到短枚举s2[0~i-1]的border
        //第i次循环开始前,j=nxt[i-1]均成立
        //若匹配不成功,则j = nxt[j]尝试匹配一个最长的border
  
        if(s2[i] == s2[j+1]) ++j;
        //如果匹配成功而跳出循环,此时s2[0~i]最长border就是j+1
        //反之,说明这样的border不存在,因此j=-1不加
  
        nxt[i] = j;//此时nxt = j
    }
}
//O(n)处理出位置
void work() {
    int j = -1;
    for(int i = 0; i < len1; ++i) {
        while(j!=-1 && s1[i] != s2[j+1]) j = nxt[j];
        //这一步对应的是由长到短枚举s1[0~i-1]的border
        //第i次循环开始前,j=nxt[i-1]均成立
        //若匹配不成功,则j = nxt[j]尝试匹配一个最长的border
  
        if(s1[i] == s2[j+1]) ++j;
        //如果匹配成功而跳出循环,此时s1[0~i]最长border就是j+1
        //反之,说明这样的border不存在,因此j=-1不加

        if(j == len2-1) {//说明已经匹配成功了一个子串
            printf("%d\n",i-len2+2);
            j = nxt[j]; //跳到nxt继续下一次的匹配
        }
    }
}
void printnxt() {
    for(int i = 0; i < len2 ;++i) {
        printf("%d ",nxt[i]+1);
    }
}


int main() {
    scanf("%s",s1);
    scanf("%s",s2);
    //注意,strlen是O(n)时间
    //所以循环中必须要用 i < len1 而非 i < strlen(s1)
    len1 = strlen(s1);
    len2 = strlen(s2);
    prework();
    work();
    printnxt();
    return 0;
}
```
