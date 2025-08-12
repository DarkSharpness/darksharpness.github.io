---
title: nano-sglang
date: 2025-07-28 23:44:08
updated:
tags: [system, LLM-serving]
categories: [system, MLsys]
keywords: [sglang, LLM, MLsys, inference]
cover: https://www.galneryus.jp/images/STAR_jyake.jpg
mathjax: true
description: sglang, 但是极简版.
---

封面出自笔者非常喜欢的一个乐队的新专 [The stars will light the way](https://www.galneryus.jp/music/albums?item=the_stars_will_light_the_way), ~~放一张飞龙在这里寓意着你也可以写一个性能非常强甚至超过 SGLang 和 vLLM 的框架~~

在 7 月初, 笔者在自己的项目中需要用到一个高性能的 LLM serving engine. 但是现有的开源 SOTA 比如 sglang, TensorRT-LLM, vllm 什么的, 过于重量级了, 笔者改起来无从下手. 于是决定了自己从头写一个 LLM 框架, 在这过程中也顺便能加深对于 serving engine 不同组件的理解. 于是就有了现在这个项目.

笔者最早是在六月最后两三天的时候开始写的, 中途因为去 OSDI 开会基本是中断了一周 (倒时差差点干死了笔者). 而且一开始这个框架也不是作为 serving engine. 总之实际花在上面的时间大约只有三周, 但是三周时间足以支持很多高性能 serving engine 所需要的核心 feature 了. 在写这个所谓 nano sglang 的过程中, 笔者自然也是参考了不少 sglang 的 codebase, 不过更多时候笔者还是自己 design, 作为一个做 system 的人, 自己 design 一个 system 再去不断 refine 才是最爽的.

废话不多说, 直接进入正题. 本文还会顺带介绍一些 LLM serving 中非常著名的 paper, 其中的 idea 现在看来并不复杂, 实现起来也没有很多细节.

## Model Structure

> Remark: 这部分笔者花了一周, 当然还有一些杂七杂八的 profiling 占据了不少时间片

模型这部分是最 trivial 的, 理论上知道了模型的架构都非常好写, 但实操的时候还是有不小的细节要注意, 特别是 TP (tensor-parallism) 相关的.

从最外层的角度来看, 一个 LLM (Llama 这种) 需要这些 layer:

1. Embedding layer (Embed)
2. Attention layer (Attn)
3. Feed forward network (FFN)

### How to batch

在 LLM serving 中, 一个比较重要的问题是, 我们怎么进行 batching, 这决定了后面我们搭建的 model 的结构, 以及 scheduler 的预处理部分.

首先, 在 LLM online serving 的场景中, 我们会有很多的请求 (Req) 到来, 对于每个 Req, 它的输入是一些文字, 我们会首先把这些 text tokenize 变成 tokens, 类似一个 list of int.

在没有 batching 的时候, 我们只需要把输入 tokens 喂给模型, 每次把新生成的 token append 到之前的 tokens 后面, 然后把新的 tokens 一起喂给模型.

我们可以简单的把 LLM 看作一个 function $f$, 那么实际上我们输入就是 $x$, 输出 $y=f(x)$. 其中 $x$ 的维度是 $[\text{seq-len}]$, 每个元素的取值范围是 $0 \sim \text{vocab-size}$, $y$ 的维度是 $[1]$, 每个元素的取值范围同 $x$. 下一轮的输入就是把 $y$ 拼接到 $x$ 的后面, 维度是 $\text{seq-len} + 1$.

在有 batching 的时候, 传统 DL model 的做法是新增一个维度, 在我们的例子里就是输入 $x$ 的维度变成 $[2, \text{seq-len}]$. 但是在 LLM serving 中, 不同请求的输入的长度是不一样的. 这时候, 一个比较暴力的做法就是 padding, 即我们取输入长度为 $\max(\text{seq-len})$, 但这样几乎不可避免会带来一些计算上的浪费.

这时候, 一个比较优雅的 solution 是: 我们把输入摊平. 假设输入有 $x_1, \cdots, x_n$, 他们的长度分别是 $l_1, \cdots, l_n$, 那么我们把它拼起来, 直接合成一个大 $X$, 它的长度是 $\sum_{i=1}^{n} l_i$. 这样, 我们就没有一点计算是多余的.

这个方法看起来很简单, 实际上会有一些工程上的 trick. 幸运的是, LLM 常见的 kernel, 大部分很容易就能支持这个操作. 比如矩阵乘法操作 $A \times B$, 本质上是对于 $A$ 的行向量和 $B$ 的每个列向量做内积. 这时候, 你把不同请求的矩阵 $A_i$, 沿着列的维度拼接起来得到形如 $[A_1^T, \cdots, A_n^T]^T$ 的 $A$, 那么 $A \times B$. 此时等价于对于每个 $A_i$ 分别做乘法.

唯一比较 tricky 的部分是 attention kernel. 它是一个取决于每个请求 tokens 数量的 kernel, 并不像 linear kernel, 没有良好的线性性质. 不过幸运的是, 伟大的 flash attention 和 flashinfer 都提供了类似 `flash_attn_with_varlen` 的接口, 支持直接用拼接起来的 $q, k, v$ 计算.

因此, 我们的 batching 思路就非常简单: 首先选出一些请求, 然后把这些请求的 tokens 拼接起来, 直接作为模型的输入, 最后得到输出后再把结果拆开来. (当然实际上我们有 KV Cache, 因此我们只需要把每个请求不在 KVCache 中的那部分后缀 tokens, 对于 decode 每个请求的这部分 tokens 长度是 1).

### Embedding layer

首先分析单机的 Embedding layer. Embedding layer 的作用是, 把 tokenize 后的 ID (可以理解为 list of int) 转换为 embedding vector. 在单机上, 这其实就是一个根据 token ID 的值, 从一个 embedding vector list 中索引并且复制到输出的 tensor 里面.

```python
import torch.nn.functional as F
# shape info:
# x: [n]
# weight [vocab_size, d]
# y: [n, d]
y = F.embedding(x, weight)
return y
```

在 TP 的时候, 我们的做法是把不同的 index 的 vector 分配到不同的机器上. 比如 TP=2 的时候, 我们会把前一半的 embedding vector 放在第一个 GPU 上, 后一半的 embedding vector 放在第二个 GPU 上. 假如我们要的 token id 比 $\frac{\text{vocab-size}}{2}$ 要小, 那么我们会从第一个 GPU 上取, 否则我们会从第二个 GPU 上取. 需要注意的是, 我们并没有切开 weight 的 $d$ 维度, 这也意味着每一个 vector 要么不在机器上, 要么就是完整的在一台机器上, 不会存在某个 token id 对应的 vector 横跨两台机器. 一个参考实现如下:

```python
mask = (x >= vocab_start_idx) & (x < vocab_end_idx)
x = mask * (x - vocab_start_idx)
y = F.embedding(x, weight)
y = mask.unsqueeze(1) * y
all_reduce(y)
return y
```

在这段代码中, 每个 TP rank 存的是 $[\text{vocab-start-idx}, \text{vocab-end-idx})$ 这一段的 vector, 因此我们会 mask 掉其他的 embedding 操作, 对于不在当前 rank 的 indexing, 我们会将值 mask 为 0. 最后, 我们会对输出做 all reduce. 因此最后我们要的值一定存在且只存在于其中某个 rank 上, 所以最后的加和就是正确的 embedding 的值.

### FFN

FFN 本质就是两个 linear 操作加上一个激活函数, 激活函数大部分也都是线性的, 所以这里仅讨论 linear 部分的处理, 即矩阵乘法. 在使用了摊平 batch 的方法后, 输入部分就是一个 $[n, d]$ 的二维矩阵. 对于非 TP 的情况, 我们直接用 torch 自带的 linear 函数即可.

```bash
import torch.nn.functional as F
# shape info:
# x: [n, d]
# up_proj [D, d]
# down_proj [d, D]
# y: [n, D]
# z: [n, d]
y = F.linear(x, up_proj)
z = F.linear(y, down_proj)
return y
```

需要注意的是, `F.linear` 传入的 weight 是一个事先就已经转置好的矩阵, 这样矩乘效率会更高一点.

在 TP 的时候, 我们每一层的输入输出在每个 TP rank 上都是一样且完整的 $[n, d]$. 在过 FFN 的时候, 我们会把 intermediate dimension $D$ 拆开 (一般来说 $D > d$, 所以叫做 `up_projection` 和 `down_projection`). 对于正常的 TP, 我们都会要求 GPU 总数量 $N$ 可以整除 $D$. 此时, 每个 $TP_rank$ 会持有一部分 `up_proj` $[\frac{D}{N}, d]$ 以及 `down_proj` $[d, \frac{D}{N}]$.

在经过 `up_proj` 之后, 每个 rank 只会持有一部分权重 $y_i$, 维度 $[n, \frac{D}{N}]$, 需要将所有 $GPU$ 的 $y$ 拼起来才能得到完整的 $y$. 但是, 为了减少通信次数, 我们可以将不完整的权重先过完 `down_proj`. 此时, 所有 rank 上得到的 $z_i$ 维度都是 $[n, d]$, 而根据分块矩阵乘法的知识可知, 此时完整的 $z$ 应该是所有的 $z_i$ 的和, 因此我们需要经过 all-reduce 操作. 这里的 all-reduce 是常见 collective operation 的一种, 详情可见 [NCCL 对于 collective operation 的介绍](https://docs.nvidia.com/deeplearning/nccl/user-guide/docs/usage/collectives.html).

下面是从 PyTorch 教程里面偷来的 Megatron 关于 TP 的 MLP (也就是 FFN) 的分析, 图比较直观, 读者可以自行用 $N = 2$ 的例子来验证最后输出的值 $Z$ 确实应该是 $\sum Z_i$.

![TP from megatron](https://docs.pytorch.org/tutorials/_images/megatron_lm.png)

## Backend engine

## Overlap Scheduler

## Frontend design
