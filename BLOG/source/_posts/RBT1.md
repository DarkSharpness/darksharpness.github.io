---
title: 红黑树的一些实现 & 剖析
date: 2023-03-14 16:42:16
updated: 2023-03-17 12:39:50
tags: [树形结构,平衡树,STL]
categories: [算法,树形结构,平衡树]
keywords: [树形结构,平衡树,STL]
cover: https://s2.loli.net/2023/03/14/zWCE74irBUnSox3.png
mathjax: true
description: 红黑树的一种高效架构及实现。
---


红黑树是一种非常高效的数据结构。其本质是一颗平衡的二叉搜索树，可以在 $O(logn)$ 的时间内完成一次插入或者删除操作，支持在最坏 $O(logn)$ 的时间内树上进行二分查找的操作。在各种编程语言中，往往也会用红黑树算法来实现最基础的 map 类(或者其他类似的名字，例如 python 字典类)，可以通过一个 key 来查询对应的 value 。

本文不会过分地讨论红黑树平衡相关的算法，而是会具体讨论下红黑树实现的底层架构，算法问题请左转百度自行学习。本文希望能够从一些最简单的想法出发，引出一个复杂而高效的红黑树实现。

# 前置任务

**在开始阅读文章前，请牢记以下这些核心问题与要求!**

**在开始阅读文章前，请牢记以下这些核心问题与要求!**

**在开始阅读文章前，请牢记以下这些核心问题与要求:**

## Questions:

- 如何简化繁多的边界判定以及分支 ?
- 如何减少不必要的重复代码 ?
- 如何尽可能地减小空间占用 ?
- 如何避免模板导致的代码膨胀 ?

## Requirements:

- 最坏 $O(n)$ 的遍历树(用迭代器)，复制另外一颗树
- 最坏 $O(logn)$ 的插入、删除、二分查找操作等等
- 均摊 $O(1)$ 的迭代器自迭代操作(++ 和 -- 双向迭代)
- 最坏 $O(1)$ 取得 begin() 和 end() 迭代器、迭代器解引用

# 回顾

先简单回顾下二叉搜索树。

## 节点

一颗二叉树的节点会保留一些必要信息。一般来说，一个节点必须要有指向左儿子和右儿子的两个指针，还有节点保存的数据，大致如下:

```C++
struct Node {
    Node *left;
    Node *right;
    value_type data;
};
```

对于红黑树，其特别地还需要保留一个节点的颜色(Color)。不难发现，保存红黑树的节点只需要 1bit 即可，但是如果用一个 bool 变量来保存，那么由于 C++ 的对齐，其将会被拓展到 8bit 甚至更多 (一般来说至少会被对齐到 4Byte 或 8Byte)。因此，存在一种节约内存的优化，即把颜色嵌入某个指针的最低或者最高位 (事实上，在 64 位机器上，指针有很多bit 都是无效信息，因为指针只需 40bit 就可以唯一标号一个 1T 内存中每一个位置，一般多余的 bit 内存管理器会用来保存其他的信息，例如内存块的大小) ，当然这涉及程序甚至是系统底层 allocator 的具体实现，一般只有非常底层、贴近操作系统 (例如Linux内核) 的代码中才会考虑。

同时，若要实现迭代器遍历的功能，则必须额外存一个指向父亲节点的指针 parent ，原因也很显然，这里举一个最简单的例子(不是说只有这个情况) : 如果当前迭代器指向叶子节点 (即当前节点左右儿子指向空) ，那么向前或向后迭代该迭代器的时候，其必须要往父节点方向走，这就需要父亲指针 parent 的存在。(你可能想问为啥迭代器不能单独存一些额外信息来避免存 parent 指针，但是容易证明这需要额外保存最长 $O(logn)$ 级别的 parent 链，这样的开销对一个本应 simple 的迭代器是不可接受的)

下面是一个演示实现代码:

```C++
/* 颜色 变量，只需一个 bit 即可 */ 
enum class Color : bool { BLACK,RED };

/* 节点主体 */
struct Node {
    Node *parent;
    Color color;
    Node *left;
    Node *right;
    value_type data;
};

```

## 搜索树

对于一颗搜索树，其显然需要保留一个根节点，或者说是指向根节点的指针。不仅如此，其也需要提供一个 allocator 来管理节点占用空间的申请与释放，还需要一个 Compare 类来负责比较两个 key-value pair 的 key (因为搜索树内部一般是按照 key 的大小顺序构建的)。当然，如果你的容器需要记录节点总数，那么还需要额外存一个 count 变量。

*题外话: 如果你还不知道如何啥是 allocator，说明你可能没怎么写过工程代码，但是别急，你可以理解为用来申请/释放内存空间的一个类，提供类似 new/delete,malloc/free 的功能。

因此，一个典型的基于搜索树类可以大致写成如下的形式:

```C++
struct tree {
    allocator Alloc;
    Compare comp;
    Node *root;
    size_t count;
};

```

## 亿朵乌云

> 动力理论肯定了热和光是运动的两种方式，现在，它的美丽而晴朗的天空却被两朵乌云笼罩了。

> 现在，红黑树那看似美丽而晴朗的应用前景，也被亿朵乌云所笼罩。

现在我们已经基本定义了红黑树的最核心的两个类 (好吧，应该是三个，算上 enum class Color 的话)。然而，问题真的解决了吗 ? 还有些许边界细节没有讨论清楚吧 ?

以下是一些是实现红黑树时常见的问题 (至少 DarkSharpness遇到了部分) :

1. 当树为空的时候，root 指向什么? 空?
2. allocator 和 Compare 往往是空类型，但是由于 C++ 要求至少会占用 1 Byte，对齐后可能就是 8 Byte，空间浪费如何解决?
3. iterator 类里面存什么? 一个指针? 还是两个?
4. 如何保证获得 begin() 和 end() 都是最坏 O(1) 的?
5. end() 迭代器理论上指向的不是这个 map 中的元素。此时如何保证 --end() 能回到 map 中?
6. 红黑树主体如此繁杂，如何尽可能避免模板类带来的代码膨胀? [(see this)](https://blog.csdn.net/zhizhengguan/article/details/113384008)
7. 如何尽量复用代码，让程序主体看起来尽可能的简洁?

**我们记作 7 朵乌云。**

事实上，仔细思考，这里面还是有很多细节的。下面将从具体实现入手，加以分析。

# 深入

为了尽可能地优化，我们需要结合一些 C++ 和 红黑树 的 Feature 进行针对性的优化。

## 空基类优化

**针对前面的第 2 朵乌云**，我们可以利用C++ 中的**空基类优化** ([Empty base optimization](https://en.cppreference.com/w/cpp/language/ebo))。简单来说，虽然 C++ 要求任何类型的 sizeof 至少为 1 来保证两个不同的对象有不同的地址 (想想sizeof()是 0 的时候，数组里面不同的对象的地址情况) 。但是对于空基类 (即没有虚函数和非 static 的变量)，如果衍生类除去基类的部分 sizeof() 不是 0，且满足一些特殊的条件 (详见前面的link)，那么可以把空基类的地址设置的和衍生类第一个 sizeof 不是 0 的变量一样，这样就可以避免空类型占用空间。本例中，由于要记录树节点数量 count，所以将其 allocator 和 Compare 压缩到 count 上。

参考实现如下:

```C++
// Utilizing EBO
struct implement : allocator,Compare {
    size_t count;
};

// 如果 allocator 和 Compare 为空 , 正确
static_assert(sizeof(size_t) == sizeof(implement));
```

**由此，我们借助了 EBO 空类型优化，解决了乌云 2。**

## 节点和树的关系

节点 Node 是树 tree 的重要组成部分，而 tree 又是必须依赖于模板里面的 Compare 和 allocator的，但是，这真的意味着两者不可分离吗? 难道所有的原子操作都是依赖模板参数的? 模板代码膨胀 ~~dssq~~ 不可避免了?

最简单的，我们粗糙地考虑下红黑树的一次成功的插入 insert 操作: 首先通过树，定位到对应的插入的位置，构造并插入一个 Node，然后开始调整树的红黑关系。

**现在给你半分钟。仔细思考以上的步骤中，哪些是必须依赖模板类 tree 的，哪些是可以独立于 tree 而只依赖于 Node 而进行的。在这里，我们假设 tree 和 Node 是分离的。** 你可以带着问题先看下去。

**我们考虑前面的第 1 朵乌云**: root 存什么。事实上，root 是一个很特殊的指针，其指向的是红黑树的根节点。而当 root 所指向的节点改变，例如在 红黑树旋转 rotate 操作后，root 的值也应当随之改变。然而 rotate 操作修改的是相关 Node 的指针，而不能修改 root 这一个属于 tree 的指针。这便会导致如果 rotate 旋转后改变了根节点，那么你将必须要回到 tree 去修改 root，或者在 rotate 参数传入 root 指针引用。这样的代码显然是不好的，将导致 rotate 过多依赖于 tree 。事实上，如果把 root 指针嵌入某个 Node，即 root 其实是某个 Node 的指针，那么我们在 rotate 中就可以直接通过修改 Node 的指针来修改 root，从而达到脱离 tree 只借助 Node 来 rotate。

**一分钟差不多也到了吧(笑)。**从上面这个例子，我们不难看出，在把 root 嵌入 Node 以后，我们可以把 rotate 操作直接独立于 tree，只借助 Node 之间的修改操作 (一般是通过Node *间接修改)，来实现。再回到前面那个问题，在 insert 中，定位到插入位置依赖于比较函数 Compare，而构造并插入 Node 依赖于 allocator，但是调整树的红黑关系，它只需要知道 Node 的颜色，修改 Node 的属性即可了。因此，我们可以认为调整 Node 颜色这一步是独立于 tree 进行的。

但是，别忘记了，我们的 Node 目前还是依赖于树的 value_type 呢，两者独立只是我们的假设，重新看看节点最原始的定义:

```C++
/* 节点主体 */
struct Node {
    Node *parent;
    Color color;
    Node *left;
    Node *right;
    value_type data;
};

```

这就很 annoying 了。但是，仔细思索前面提及的 Node 的操作 : rotate 和 insert 后调整颜色。两者似乎...似乎有什么共性? **两者都不依赖于 value_type !** 是的，当你完成插入以后，他已经是一颗标准的二叉树了，无论怎么 rotate ，怎么调整颜色，都不会影响到 value_type。不仅如此，value_type 也不会影响到后续核心的 insert 后调整颜色。说到这，你是不是有一种感觉，感觉 Node 是一个不够原子不够本质的东西? 好像还能再提取些什么出来，其完全不依赖于 tree 所提供的模板。由此，DarkSharpness 想出了两种不同的思路:

- 将 value_type 换成一个 void * 指针，需要时指针类型强转。
- 将 Node 拆成一个不含 value_type 的基类 Node_base 和继承了 Node_base、依赖于模板 的 Node 类。进行不依赖 tree 操作时，Node 指针安全退化为基类 Node_base 指针。

显然的，后者会略好于前者。首先，前者会多存一个指针的大小，这个开销时完全可以避免的。同时，后者用到了隐性类型转化 (派生类退化为基类) 会更加安全。最后，前者用指针简介访问存储的数据，其实会带来一定的性能开销 (访问数据相当于再经过了一层指针，而指针寻址不是很高效)。

所以，在综合多种考虑后，DarkSharpness 决定采用第二种实现: 将 Node 拆成一个不含 value_type 的基类 Node_base 和继承了 Node_base、依赖于模板 的 Node 类。进行不依赖 tree 操作时，Node 指针安全退化为基类 Node_base 指针。如下所示:

```C++
/* 基类 */
struct Node_base {
    Node_base *parent;
    Color color;
    Node_base *left;
    Node_base *right;
};

/* 派生类 */ 
template <class T>
struct Node : Node_base {
    T data;
};

```

而 rotate 等部分红黑树操作也就可以独立于模板，从而避免代码膨胀了。大致如下:

```C++

// 将当前节点旋转到父节点位置
void rotate(Node_base *ptr) {
    // ...
}

// 插入当前节点后，完成红黑树的修复(调整颜色，改变相对位置rotate)
void insert_fix(Node_base *ptr) {
    // ...
}

```

总结一下，通过综合考虑红黑树的 tree 和 Node 的关系，我们把之前朴素想法的 Node 类拆成了更加本质的 Node_base 和模板派生类 Node ，从而将部分的函数直接独立于模板类 tree 实现。事实上，这部分函数占据的是红黑树的主体部分，这样的实现方式非常有助于减少代码膨胀。

**由此，乌云 6(其实也就是核心问题中的代码膨胀问题) 得以部分被解决。**

## 从迭代器到根结点、root 指针架构

(大段文字警告!⚠ 请耐心阅读，谢谢配合!)

在前面节点和树的关系的讨论中，我们首先指出了 root 指针(注意，不是 root 节点) 应该嵌入一个 Node ，这样可以便于节点和树进一步地分离，例如在 rotate 操作中修改 root 的话只需修改 root 所在 Node 的一个指针即可，不需要知道模板类 tree 内的信息。随后，我们又为了分离 tree 和 Node ，把 Node 拆分为了 Node_base 和 派生模板类 Node。因此，我们可以进一步地细化，指出 root 应该嵌入一个 Node_base 中，作为其一个指针。我们把这个特殊的 Node_base 称作 Header。

需要特别注意的是，root Node 和 root 指针不是一个东西，root Node 是一个具体的 Node，root 指针是一个指针变量，其指向 root Node，应该是 Header 的一个成员变量。

**但是**，你真的认为这样就结束了吗 ? 如何保证根节点能正确地修改(例如 rotate 操作，根结点不用特判吗 ? 怎么判断是根结点呢 ? ) ? root 指针具体如何嵌入 Header ? 是作为 Header 的 left 指针 ? right 指针 ? 难道是 parent 指针 ? 剩下两个不记录 root 的指针记录什么呢 ? 这个 Node_base 的 Color 有用吗? root Node 的 parent 是什么呢 ? 这些问题似乎都没有一个明确的答案。

### iterator 存在的问题

你先别急。我们先来简单分析下 iterator 的迭代过程。当一个 iterator 要自增，其操作大致如下:

- 有右儿子
  - 后继节点是右子树的最左边的叶子节点。
- 没右儿子
  - 往上走，直到当前节点不是的父节点右儿子，此时父节点为后继节点。
  - 上述操作走到了根结点，说明当前节点就是书上最大节点。

这没啥好分析的，非常自然，容易由二叉树的定义导出。代码也很简单，参考如下。

```C++
/* 返回后继节点，相当于 ++ */ 
Node_base *next_Node(Node_base *current) {
    if(current->right) {
        current = current->right;
        while(current->left)
            current = current->left;
        return current;
    } else {
        /* is_root 是一个特殊的在类外判断是否为根的函数 */
        while(!is_root(current)) {
            Node_base *parent = current->parent;
            if(parent->left == current) return parent;
            current = parent;
        }
        return nullptr; // 当前节点无后继
    }
}
/* 返回前驱节点，相当于 --*/ 
Node_base *prev_Node(Node_base *current) {
    if(current->left) {
        current = current->left;
        while(current->right)
            current = current->right;
        return current;
    } else {
        /* is_root 是一个特殊的在类外判断是否为根的函数 */
        while(!is_root(current)) {
            Node_base *parent = current->parent;
            if(parent->right == current) return parent;
            current = parent;
        }
        return nullptr; // 当前节点无前驱
    }
}

```

先不管缺失的 is_root 函数，我们暂时只考虑迭代器 end() 自减的问题。--end() 可以使得迭代器指向 tree 中最大的元素，但是问题在于，end() 应该存储什么信息 ? 如果 end() 是空指针的话，那么仅靠它没有任何方法走向 tree 中最大的元素。

你可能会说，“那么我们不要求迭代器可以反向迭代不就可以了吗”。这不是一个好的办法。事实上，Node_base 的结构天生的就保证了双向迭代的可行性，如果不实现反向迭代，其实某种程度上的功能浪费，没能 exploit potential to its full. Anyway ，笔者在文末也会单独讨论这种情况的特殊实现，但是现在我们暂时认为这是一个不好的解决方案。

### 解决方案

针对 end() 的问题，一种简单的解决方案是在迭代器中多存一个指向 tree 的指针，end() 存空指针，特判 --end() 即可。这样也的确是一种可行的方案，但是毕竟多存了一个不必要的指针，还是有一定空间上的浪费，而且除此之外该指向 tree 的指针毫无用途，只有一个地方有用，真的浪费。

还有一种思路，是让 end() 指向树中的某一个特殊节点，满足其正常 -- 操作后指向最大的节点。然而，容易证明，在树上没有这样的节点，而且会导致迭代器相等的判定出错。

我们或许可以在最大节点下面再挂一个节点(作为右儿子)。**这势必会带来更多的不必要的特判，破坏了代码的简洁性，更重要的是容易导致 tree 和 Node 的分离失效，是应该避免的，请务必记住这点，感兴趣的话自己写写看就知道了**。

但是，等等! 我们只说了树上的节点，还没说不是树上的节点。我们还有一个 Header! 这便是突破口 : **我们可以让 end() 指向 Header** ! 但是，我们依然要保证以下两条特殊性质:

1. 指向最大节点的迭代器 ++ 会到达 end()
2. end() 迭代器 -- 能回到最大节点。这是两个特殊的性质。

后面将具体分析。

### 特殊性质 1

让我们回到 prev_Node 和 next_Node 函数，仔细再分析一下。对于第一个性质，最大节点的迭代器 ++ 即 next_Node 会到达 end()，针对 next_Node 的两个 if 分支，一个解决方案是在最大节点下面挂一个节点 (右儿子) ，但是前面已经说过了，会带来大量不必要的特判，非常麻烦。另外一个解决方案，就只能是最大节点在祖先链条上。而显然的，对于最大值所在节点，++ 的时候能走到根节点并且停下来。因此，一个很自然的想法出现了 : 我们让根节点的 parent 指针指向 Header，这样，我们 ++ 的时候就可以通过不断地往父亲节点走，最终从 root 到达了 Header，即所谓的 end() 指针。这看起来真的太棒了! 它几乎不会引入什么额外的分支。

不过需要注意的是，此时对于最大值迭代器 ++ 的情况，仅仅让根节点的 parent 指针指向 Header 需要一个 is_root 函数。但是当根结点的 parent 指针指向的是 Header 而不是空的时候，我们暂时还不能很高效地、不依赖于 tree 地判断当前节点是不是根。因此，一种可行的架构是，让 Header 的 left 指针指向根结点。此时最大值迭代器 ++ 自然会停在 Header 即 end() ，因此不用再特判。同时，另外一种可行的架构是，让 Header 的 right 指针指向根结点。此时，最大值迭代器 ++ 会一路走到 Header，而我们只需要一个能在 tree 外判断是不是 Header 的函数即可，例如将 Header 的 parent 指针置空，通过检测 该特定指针来判断是不是 Header。当然，还有一种可行的架构是 Header 的 parent 指向根结点，这样子，根结点和 Header 都具有性质: 当前节点的 parent 的 parent 是自己，从而可以在根结点特判解决。

总结一下，我们暂时做的是:

- 把根节点的 parent 指针指向 Header。

我们可选的方案是:

1. 去除 is_root 特判，直接将 Header 的 left 作为 root 指针(指向根结点)。
2. 将 Header 的 right 作为 root 指针(指向根结点)，将 Header 的 parent 指针置空用于判别 (额外占用一个指针)。
3. 将 Header 的 parent 作为 root 指针(指向根结点)，特判根结点和 Header 具有 parent 的 parent 指向自己的性质。

三者开销几乎一致，暂时看来 1 略优于 3 略优于 2。

### 特殊性质 2

OK，问题已经解决了一半，剩下另一个性质是: 保证 --end() 可以到达最大值所在节点。而通过 -- 即 prev_Node ，针对 prev_Node 的两个 if 分支，一个解决方案是将 Header 的 parent 指向最大节点，但是缺点也很明显 : 此时最大节点的右儿必须指向 parent (为了 next_Node 中的特判)，这已经分析过了，势必会带来大量不必要的特判。剩下的解决方案是将 Header 的 left 指针指向根结点到最大节点路上的任意一个节点。一般来说，正常人只会考虑其中两种情况 : 指向根结点和指向最大值节点。

但是，结合前面特殊性质 1 里面提到的可选方案，你会发现将 Header 的 left 指针指向 root 节点几乎是完美的。这相当于 Header 存储了一个永远大于最大值的值，因此可以保证 --end() 可以到达最大值节点，反过来也可以通过 ++ 到达。这样的实现看起来就非常优雅、简洁，唯一的问题是Header 节点在树中的关系不是太和谐，他不是一个对称的存在而是一个永远最大的存在。

当然，我们也不能忽略其他几种相对没那么完美的实现。对于特殊性质 2 里面的可选方案 2 似乎的确会带来很多麻烦事情，但是，可选方案 3 并不是一个无用的存在。在方案 3 中，如果将 Header 的 left 指针指向最大值节点，同样可以 --end() 到达最大值节点，而且可以把这一次操作的复杂度降低到 $O(1)$ 。不仅如此，如果出于对称性，将 Header 的 right 指针指向最小值节点，你会发现整颗树连起来，我们可以让最大值迭代器 ++ 回到 Header (end())，再 ++end()，可以直接走到最小值节点。由此，一颗树被连了起来，无论如何 ++ -- ，迭代器都不会走出 树的范围(包括 Header)。不仅如此，此时树的 begin() 迭代器也可以通过访问 Header 的 right 指针在 $O(1)$ 的时间内完成，还顺便解决了第 4 朵乌云。而且此时 Header 节点是完全对称的，这样的实现比起前面看似“完美”的实现，要更加的对称、优雅，也是笔者采用的实现方式。

不过，这也不是唱衰前面那种“完美”实现，其也可以通过将 Header 的 right 指向最大值节点，以及 Header 的 parent 指向自己 (仔细想想是为什么，如何实现，留作思考题，欢迎在评论区留言) ，来实现和方案 3 一样的功能 : 永不越界的迭代器、常数时间的 begin() 和 end()。只不过，比起方案 3 ，方案 1 的不对称性令人略有不爽。事实上，方案 1 实现的迭代器不需要根结点特判 (is_root 函数)，可能实际时间效率还略高于方案 3 。

同时，其实仔细再想想，方案 2 也不是不行，只需将 方案 1 镜像以后，即 Header 的 left 指针指向最大值节点，right 指针指向最根结点，parent 指针指向自己 (同样的，自行思考为什么指向自己不会出 bug)。不过问题在于，begin() 函数指向最小值节点未维护，因此其是 $O(\log n)$ 的时间复杂度，其实并不优。

仔细考虑后，你才会发现，其实可选的实现方案真的很多。

### 小总结

综上，我们在思考 root 指针和根结点架构的时候，为了满足迭代器的一些性质，做出了针对性的一些优化。最终架构如下:

- root 指针嵌入 Header 中
- 根结点的 parent 指针指向 Header

Header 的可选方案只剩以下两种:

1. Header 的 left 指针指向根节点，right 指针指向最小节点，parent 指针指向自己
3. Header 的 left 指针指向最大节点，right 指针指向最小节点，parent 指向根结点

笔者使用方案 3，不过两者均可以满足:

- 迭代器可双向加减，包括 end()。
- 迭代器 ++ -- 不会越界，整棵树连起来。
- begin() 和 end() 都是 $O(1)$ 时间
- tree 和 Node 依然可以很好地分离。

而利用了以上实现，我们也不难发现，此时迭代器只需存指向节点的指针即可，因为 ++ -- 完全不会有越界问题，parent 指针的存在 以及 Header 和 root 的特殊架构保证了这一点。

**至此，乌云 1,3,4,5 彻底解决。**

## 剩下的实现细节

讲到这里其实基本没啥好讲的了，笔者具体来讲讲实现上的一些细节，顺便解决最后的第 7 朵乌云。

### 从代码复用到 Node 架构

看到这个副标题，你可能会困惑。 WTF ? Node(其实是 Node_base) 架构又要改了，那不是前面都白弄了 ? 你先别急。不是这样的。我们不会改变 Node_base 原有的架构 : 两个儿子指针，一个父亲指针，一个颜色。那还还能改什么呢 ? 继续看下去。

前面已经提到了 ++ 和 -- 的底层函数 : next_Node 和 prev_Node 。你有没有想过，这两个函数是可以压缩成为一个的。的确，这两个操作本质上就是镜像操作。我们再考虑下正常平衡树的旋转操作 rotate 。一般来说，他会被分为两个函数 : zig 和 zag 。然而，~~愚蠢的笔者分不清 zig zag 哪个是左旋，哪个是右旋~~这么对称的两个函数分开实现，真的不会感到重复吗 ? 

不难看出，红黑树上有许多的操作是镜像对称的，例如 next_Node (++) 和 prev_Node (--)，还有 zig 和 zag。这些操作明显有点重复，如何把他压缩成为一个函数呢? 我们以 next_Node 和 prev_Node 为例分析。

```C++
/* 返回后继节点，相当于 ++ */ 
Node_base *next_Node(Node_base *current) {
    if(current->right) {
        current = current->right;
        while(current->left)
            current = current->left;
        return current;
    } else {
        /* is_root 此时视架构而变 */
        while(!is_root(current)) {
            Node_base *parent = current->parent;
            if(parent->left == current) return parent;
            current = parent;
        }
        return current; // 返回参数视架构而定
    }
}
/* 返回前驱节点，相当于 --*/ 
Node_base *prev_Node(Node_base *current) {
    if(current->left) {
        current = current->left;
        while(current->right)
            current = current->right;
        return current;
    } else {
        /* is_root 此时视架构而变 */
        while(!is_root(current)) {
            Node_base *parent = current->parent;
            if(parent->right == current) return parent;
            current = parent;
        }
        return current; // 返回参数视架构而定
    }
}

```

不难发现，两个函数只需将 left 和 right 简单替换即可，parent 指针部分不用改变。这时候，我们可以从 OIer 的代码中获得一定的启发。

![RBT2.png](https://s2.loli.net/2023/03/17/6cmJS3Kaiu2FMxZ.png)

这是 OIwiki 上平衡树 Treap 的板子。注意，left 和 right 指针用了一个数组来代替。这样的设计真的是太妙了! 通过调整下标，我们就可以访问不同地儿子，这巧妙地规避了不对称性。

此时，++ 或 -- 函数可以通过额外传递一个参数来表示，进而将 next 和 prev 压缩为一个函数，记作 advance ，实现如下:

```C++
struct Node_base {
    Node_base *parent;
    Color color;
    Node_base *son[2];
};

/* dir = 0 走向前驱 || dir = 1 走向后继 */
Node_base *advance(Node_base *current,bool dir) {
    if(current->son[dir]) {
        current = current->son[dir];
        while(current->son[!dir]) // 另外一个方向
            current = current->right;
        return current;
    } else {
        while(true) {
            Node_base *parent = current->parent;

            /* Header 方案 1 的写法 */ 
            if(parent->son[dir] == current) continue;
            return parent;

            /* Header 方案 3 的写法 */
            if(parent->parent != current && parent->son[dir] == current) continue;
            return parent;
        }
    }
}


```

当然，由于这个函数本来也不是一个很大的函数，也不包含模板，因此，你也可以将 dir 参数设置为模板参数，通过额外生成一份模板(bool 也就两种)来加速代码，当然这都是小问题了。

不过，对于前面提到的 rotate 函数，其实可以简化到连额外地 bool 参数都不传递，因为 rotate 本质是将一个节点和其父节点交换，而我们借助 parent 指针就很容易知道是左旋还是右旋了。

### 从代码复用到迭代器到 Header 架构

<a href="{% post_path iterator %}#迭代器的特殊实现"> 参考这个 </a> ，我们不难发现，我们只需为 iterator 提供一个 advance 模板函数即可。幸运的是，前面的 next_Node 和 prev_Node 的简化函数 advance 模板化以后，就是我们想要的函数。

由此，我们可以很轻松的借助那篇文章提供的思路，实现一个 map 的迭代器，这里暂时先不给出代码了。

但是，当我们要实现反向迭代器的时候，我们就不得不思考一下 : 对应的，我们有 $O(1)$ 的 rbegin() 和 rend() 函数吗。

对于方案 1，其没保存指向最大值的节点。因此，不可避免地，rbegin() 将会是 $O(\log n)$ 的复杂度。而对于方案 3 ，其由于极高的对称性，依然可以保证 $O(1)$ 的 rbegin() 函数。

因此，如果要实现反向迭代器，并且也保证 $O(1)$ 的 rbegin() 和 rend() 函数，那么留下来的唯一的选择就是 方案 3。具体如下:

- Header 的 left 指针指向最大节点，right 指针指向最小节点，parent 指向根结点
 
# Ending 结束了?

至此，核心问题和需求已经基本解决了。

但是似乎，还有啥没写完...? 我们为什么一定要反向迭代器 ? 为什么我们一定要保证可以反向迭代 ? 难道不能搞个单向的迭代器 ? 三指针的架构真的已经最优了吗 ?

暂时先更新到这里，有啥想问的评论区都可以问 qwq。
