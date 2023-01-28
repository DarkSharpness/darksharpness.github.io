---
title: 浅谈C++STL
date: 2022-10-23 15:32:55
updated: 2022-11-10 13:30:10
tags: [基础知识,算法]
categories: [基础知识,STL]
cover: https://oi-wiki.org/lang/csl/images/container1.png
top_img: https://oi-wiki.org/lang/csl/images/container1.png
keywords: [基础知识,算法,STL]
decription: 入门级 STL 介绍
mathjax: true
---
![OIWIKI_STL](https://oi-wiki.org/lang/csl/images/container1.png)

图片来自 [OIwiki](https://oi-wiki.org/lang/csl/container/)，有兴趣可以看看，讲的比我好多了Orz.
最近有亿点点忙,所以停更了一段时间。这次试着来讲讲 STL (大约是入门level)，你需要知道一些基本数据结构(队列，栈等等)作为前置知识。

## 初步认识STL

What is STL ?

STL 代表的是 Standard Template Library ， 是 C++ 的标准的模板库.

![来自Wikipedia](https://raw.githubusercontent.com/DarkSharpness/Photos/main/Images/Wikipedia_STL.png)

由上述Wikipedia定义可见，STL中包含了大量的可用容器、算法、函数和迭代器。下面将会具体讨论。

## 容器

### 什么是容器?(个人理解)

容器，顾名思义就是一种装载东西的器皿。在C++中就是一种模板类，用来按照一定的方法贮存、取出元素。

正确使用容器，可以非常有效地存储、管理数据。下面将大致介绍一些常见的容器。

### STL容器的共有的函数

以下是大部分STL容器都提供的函数

size()  返回容器内元素的个数
empty() 返回这个容器是否为空
clear()  清空容器中存储的元素
swap()  和括号内的另一个容器交换内部元素
=  将一个容器的元素赋值到另外一个容器的元素
== 判断两个容器保存的元素是否相等
!= 判断两个容器保存的元素是否不相等
begin() 返回初始迭代器(指向的是第一个元素,详见后文)
end()   返回终点迭代器(指向的是最后一个元素后一个的元素,不可访问)
rbegin() 反向初始迭代器(即指向最后一个元素)
rend()   反向终点迭代器(即指向第一个元素前一个的元素,不可访问)

### std::string

具体内容参考 [字符串基础](https://darksharpness.github.io/2022/09/13/String/string/#std-string)。


### std::vector

加入头文件 #include &lt;vector&gt; 之后即可使用。

注意，其在命名空间std内，使用前需要加std::。你也可以直接在头文件中加 using namespace std; 或者 using std::vector.

简单来说，vector 类似于数组，内部数据连续地储存，但是其长度并不确定。其可以动态地改变长度，而且可以比较高效地在尾部添加/删除元素(这方面类似栈(stack))。

#### vector 的常用功能

我们可以用 vector &lt;value_type&gt; 来声明一个vector变量，其中value_type是你要存储的变量的类型，可以是基本数据类型，也可以是一个结构体或者类。

初始情况下，容器 vector为空。我们可以用 **push_back()** 函数来向一个 vector 尾部添加元素。这会使得 vector 的体积增加1。类似的，我们也可以通过 **pop_back()** 来清除最后一个元素，使得 vector 体积减少1。

而我们可以通过 **size()** 来获取这个vector中元素的个数，并且类似数组，我们可以用 [] (0-base) 进行下标访问。

如下演示

```C++
#include <vector>
using std::vector;
vector <int> vec; // 定义

signed main() {
    vec.push_back(114);
    vec.push_back(514);// 尾部插入元素
    vec.pop_back();// 去除最后一个元素
    unsigned siz = vec.size();// 获取 vec 元素个数
    int first_element = vec[0];
    // vec[i] 获取 vec 第i + 1 个元素的值
    // (因为0-base) 
    return 0;
}
```

除此之外，vector 在初始化的过程中可以使用参数列表。

vector 初始化也可以使用数组、迭代器(见后文)通过指定首尾位置来确定，或者指定元素个数和值。

```C++

// 参数列表初始化
vector <int> v = {1,1,4};
vector <int> w {5,1,4};

// 用数组来初始化
// 两个参数分别为首地址和尾地址
// 初始化记录的是[首,尾)的元素
const int a[] = {19,19,810};
vector <int> u(a,a+3);

// 指定大小的初始化
// 此时 vector 里面的元素都为默认值
// 例如 int 就是 0
vector <int> x(10);

// 指定大小与初始值的初始化
// 下例中 y 里面有10 个 1
vector <int> y(10,1);

```

在访问 vector 内部的元素的过程中，除了像数组一样可以用下标进行访问，我们还可以用at()函数来访问指定位置，并且有越界检查(越界会抛出异常)。特别的，我们也可以用 **front()** 和 **back()** 来访问第一个/最后一个元素。

```C++

vector <int> vec = {2,3,3};

vec[0] = 1;          // 最基础的下标访问,类似数组
                     // 注意数组越界问题
int tmp = vec.at(2); // 一般情况下相当于 []
vec.at(0) = 1;       // at()也可作为左值被修改
vec.front() = 4;   
vec.back()  = 5;     // front()/back() 同理 

```

#### vector 的应用场景

首先，vector 具有一个 stack 所需的所有的接口，所以它几乎可以在任何时候替代 stack。

其次，在面对不能确定长度的数组时，vector 就是一个强有力的工具。**在均摊情况下**，vector 向尾部插入一个元素的时间复杂度为 $O(1)$ ，访问指定位置的元素的时间复杂度是 严格 $O(1)$ ，因此我们可以简单用它来处理一些容易爆内存的问题，不用考虑内存分配的细节等等。

e.g : 用邻接表存图。对于一个 n 个节点的图你需要开一个空间为 $O(n ^ 2)$ 的二维数组,当 $n > 10^4$ 时很容易爆内存MLE。这时候，我们可以用 vector 存图。对于每个节点，我们维护一个 vector，存储每个结点连向的其他结点的编号，这样我们就只需要大致与边数 $m$ 相同数量级 $O(m)$ 的内存空间。

e.g : 不定长度的数组。在程序运行的过程中，在很多情况下往往需要动态的开数组。此时，用 vector 就可以很方便的在局部开不固定长度的数组，不用担心内存分配问题。

### std::queue

加入头文件 #include &lt;queue&gt; 后即可使用

注意，其在命名空间std内，使用前需要加std::。你也可以直接在头文件中加 using namespace std; 或者 using std::queue.

简单来说，queue就是一个队列，满足基本性质FIFO。具体是每次可以向队列尾部插入一个元素，并从头部访问/取出最早插入的元素，类似于排队。

![瞎画的](https://raw.githubusercontent.com/DarkSharpness/Photos/main/Images/STL_queue.png)

如上图(画的是有点丑...),当前队伍为 {3,1,4}。你只能访问最前面的3，你也可以在尾部继续插入元素。如果我们取出头部元素3，那么下一个头部元素就是1。

#### queue 的常用功能

我们可以用 queue &lt;value_type&gt; 来声明一个 queue 变量，类似前面讲的vector(事实上，几乎所有stl容器皆是如此)。

queue的内置函数较少，故在此一次性列出，时间复杂度均为 $O(1)$

```C++
#include <queue> // 头文件
using std::queue;
queue <int> q;   // queue不能参数列表初始化

q.front() // 返回queue的第一个元素
q.back()  // 返回queue的最后一个元素
q.push(1) // 往队尾插入元素
q.pop()   // 弹出队首元素
q.size()  // 返回队伍中元素个数
q.empty() // 判断队伍是不是空

```

#### queue 的应用场景

e.g 经典场景:bfs的实现。为了保证是一层层的访问遍历，需要用一个queue记录之后要访问的结点(或节点编号)，由于队列FIFO的性质，只有在遍历完了某一次的节点后才会再遍历下一层。


### std::map

加入头文件 #include &lt;map&gt; 后即可使用

注意，其在命名空间std内，使用前需要加std::。你也可以直接在头文件中加 using namespace std; 或者 using std::queue.

简单来说是一种映射的数据结构，其可以由一种类型作为键值(key)，每个键对应一个值(value)。

#### map 的常用功能
我们可以用 map &lt;key_type,value_type&gt; 来声明一个变量。值得一提的是，对于map中的key_type，你必须提供 < (小于号) 的比较方式，因为 map 内部是维护的是一颗[红黑树](https://baike.baidu.com/item/%E7%BA%A2%E9%BB%91%E6%A0%91/2413209)，元素是从小到大的排列。(所以指针一般来说不能作为key_type，因为指针比较默认是基于指针本身的值(即地址的大小))

当然，你也可以自定义一个比较类 comp_class 来进行自定义的比较(该comp_class需重载()运算符，括号内可以接受两个key_type类来自定义比较)。

```C++
#include <map>
using std::map;

map <int,double> mp;

class comp {
  public:
    bool operator ()(int X,int Y) {
        return X > Y;
    }
};// 自定义比较类

// 用了自定义比较方法的map
map <int,double,comp> mp_custom;

```

map 最基本的操作是 [] 运算符，[] 内是一个key_type类型的值，返回的是一个value_type的值。若括号里这个 key 不存在于 map 里，则返回的值是未初始化的value_type(例如 int 未初始就是0)

## 咕咕咕