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

## Backend engine

## Overlap Scheduler

## Frontend design
