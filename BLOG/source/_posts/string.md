---
title: 字符串基本知识与约定
date: 2022-09-13 18:57:52
updated: 2022-09-13 18:57:10
tags: [基础知识,字符串]
categories: [基础知识]
cover: https://raw.githubusercontent.com/DarkSharpness/Photos/main/Images/String_HelloWorld.png
top_img: https://raw.githubusercontent.com/DarkSharpness/Photos/main/Images/String_HelloWorld.png
keywords: [字符串,基础知识]
decription: 适合入门级和普及选手的字符串介绍
mathjax: false
---
## 字符串基础

在阅读本文前请先了解字符，数组这些概念！

### 一些定义与约定

#### 字符串

字符串(string)是将一些字符按顺序排列而成的一个序列，字符总数称为字符串的长度，在C++中可以用双引号表示字符串。对于一个长度为n的字符串str，我们可以用下标索引str[i]在O(1)的时间内访问其第i+1个字符。对一个长度为n的字符串，我们可以下标索引访问str[i] 的 i 的范围为 0 ~ n-1。

* 字符串str =  "abc" ，其中str[0] = 'a'，str[2]='c'，长度为3

#### 子串

对于一个长度为n的字符串str，我们由其中第 i , i+1 , ... , j-1，j个字符构成的字符串称为str的子串。

***我们约定*** ：str[i ~ j] 和 str[i ... j]为 由str[i]，str[i+1]，... ，str[j-1]，str[j]的构成子串。

* 字符串str =  "abcdef" 其中str[1...3] = "bcd"，str[4 ~ 5] = "ef"

#### **前后缀**

对于一个长度为n的字符串str，其子串str[0 ... i]为前缀（子串），str[j ... n-1]为后缀（子串）。

一般来说，前/后缀不能等于原字符串。

***我们约定***：
Pre(str，i) = str[0 ... i]
Suff(str , i) = str[n-1-i ... n-1]

#### 字典序

类似字典中的排序，我们从小到大地枚举每一个字符，逐位比较(注意是按照ASCII的规则)，缺失的位置认为ASCII值为0（注意'0'的ASCII的值不是0而且空格' '的ASCII值为32）。在第一个不同的位上，对应字符的ASCII值大的字符串字典序更大。

* "aa" < "ab"
* "cc" < "cca"
* "00" < "0A" < "0a"

### 储存方式

一般来说，字符串有以下两种储存形式:char 数组（偶尔char *） 和  std::string(需要头文件string)。前者需要我们预先保留足够的空间（长度不小于字符总数），访问、输入输出较快。而后者会根据输入字符串长度自动申请内存，我们不用考虑其存储问题，且提供了许多的接口（即std::string提供的库函数），方便好用，但输入输出较慢，且所占空间稍多于前者。

#### char数组

我们一般用 char str[N] 的形式申请，其中输入字符总量不能超过n。因为是数组，所以索引需要从0开始。输入输出如下:

```C++
const int N = 114514;//足够大即可
char str[N];
char str1[6]="Hello";//注意!数组大小需要比字符总数多1
int main() {
    scanf("%s",str); //输入
    printf("%s",str);//输出
    return 0;
}
```

为了得到输入的字符串的长度，我们可以使用strlen(str)，需要 cstring 库。**注意！**该操作的时间复杂度为O(n)，所以我们一般用一个变量来记录strlen，避免多次调用strlen(str)带来的大量时间开销。

```C++
#include <cstring>
const int N = 114514;//足够大即可
char str[N];
int main() {
    scanf("%s",str); //输入
    printf("%s",str);//输出
    int len = strlen(str);//O(n)求出str长度
    return 0;
}
```

#### std::string

在输入输出数据量不大的时候，std::string是一个不错的选择，还提供了很多强大的功能，下面将在代码中列举一些常见的操作。

```C++
#include <string>
#include <iostream>//输入输出要用到
using namespace std;
string str;//声明变量
string str2;
char cstr[] = "Hello";//char数组也可以这样开

int main() {
    cin >> str;         //输入
    cin >> str2;        //输入
    cout << str << endl;//输出+换行
    int x,y;
    str[x]              //第x+1个字符
    str.append(str2);   //在str后面加上一个str2
    str.front();        //访问第一个字符
    str.back();         //访问最后一个字符
    str.clear();        //清空字符串
    str.compare(cstr);  //和char数组的字符串比较字典序
    str.size();         //返回字符串长度
    str.length();       //返回字符串长度
    str.substr(x,y);    //返回从str[x]开始,长度为y的子串
    str = str + str2;   //str2 加在str后面
    str += str2;        //str  加在str后面
}
```

以上是std::string中比较常用的一些函数，感兴趣的同学可以查看C++官方文档中关于std::string的[文档](https://cplusplus.com/reference/string/string/)来了解更多相关的接口以及具体实现等细节，这里就不多说了。

## 字符串进阶(个人建议)

聪明的你已经学会了字符串的基础应用，下面我们将学习字符串的一些进阶小知识。由于以下建议局限于答主个人，不保证其高效性，不一定对OI有用，仅供参考。

### 字符串的储存原理及运用

字符串的储存实际是在原有的字符上增加一个'\0'字符，其ASCII值为0，标志着字符串的终结。此外，一般文件的结尾会有EOF(End of File)，其ASCII值为-1。

知道其储存方式后，我们便可稍加运用，用char数组代替std::string避免低效的输入输出。

#### 用getchar()来读入字符串/数字

getchar()是 iostream下面的一个函数。其可以读入单个字符，返回int类型的ASCII值，适用于需要将字符映射到整数的情况(例如不同的字母对应不同的值)。如果需要存储的char 数组中，需要将末尾字符后一个位置的字符的ASCII值改为0即可。读入过程需要注意些细节：当读入的不在给定字符范围内，一般是空格或者换行，则认为当前字符串读入完毕。同时，每行读入到换行 '\n' 同样要停止。

具体运用 ：

1. 快速输入输出(快于scanf/printf,cin/cout;模板如下)

```C++
#include<iostream>
//后面的函数内static变量相当于全局变量,可以稍稍提高效率

//快速读入
int read() {
    static int  tmp ;//记录最终值
    static int  flag;//符号
    static char ch  ;//记录每次读入数字
    flag = 1;  
    tmp  = 0;  
    ch   = getchar();
    while(ch < '0' || ch > '9') {//范围外就跳过
        if(ch == '-') flag = -1; //负数就反号
        ch = getchar();
    }
    while(ch >= '0'&&ch <= '9') {//范围内就一直读入
        tmp = tmp * 10 + ch - '0';
        ch = getchar();
    }
    return tmp*flag;
}
//快速输出
void write(int x) {
    if(!x) {             //0单独处理
        putchar('0');
        return ;
    }
    static char ch[10];  //缓冲区,记录每一位数字
    static int  cnt   ;  //最高位置计数器
    cnt = -1;
    if(x < 0) putchar('-'),x=-x;
    while(x) {
        ch[++cnt] = x % 10 + '0';
        x /= 10;
    }
    while(cnt) putchar(ch[cnt--]);
    putchar(ch[0]);
}


int main() {
    int x;
    x=read();
    write(x);
    return 0;
}
```

#### 用char实现多个字符串读入

很多时候，我们面临着这样一个情景：字符总量巨大，我们明确知道它的上限，有很多组字符串要读入。然而，我们不想用IO(In/Out,指代输入输出)很慢的std::string,甚至有时候字符串组数也未知，用std::vector `<std::string>`则必然会占用大量空间。此时，我们应当考虑使用char*数组代替。
![Luogu5357](https://raw.githubusercontent.com/DarkSharpness/Photos/main/Images/Luogu5357.png)

具体实现可以用一个int类数组记录第i个字符串的起始位置记作int loc[N]，然后用逐个读入字符串，下一个字符串的起始位置为上一个字符串最后一个字符(除了'\0')的位置+2。

(当然，你也可以直接关闭流同步)

```C++
#include<iostream>
#include<cstring>
const int M = 2e5 +2;//字符串总数
const int N = 5e5 +2;//字符总量
int loc[M];
char ch[N + M];//考虑每个串多一个字符'\0'
int tot; // 字符串总数

// 读入一个字符串
void readstr() {
    scanf("%s",ch + loc[++tot]);
    loc[tot + 1] = loc[tot] + strlen(ch + loc[tot]) + 1; // 设定下一个开始的位置
}

int main() {
    int n;
    scanf("%d",&n);//读入总数
    scanf("%s",ch);//第一个字符串读入
    for(int i = 1; i < n; ++i) 
        readstr();
    // 第i个字符串起始位置为:
    // ch + loc[i]
    // 结束位置为(该位置为'\0')
    // ch + loc[i + 1] - 1
    return 0;
}

```

## 想到了再补充吧
