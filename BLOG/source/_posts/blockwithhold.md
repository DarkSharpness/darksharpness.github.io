---
title: Block Withholding Attack
date: 2025-05-26 14:47:33
updated:
tags:
categories:
keywords: [blockchain, block withholding attack]
cover:
mathjax: true
description: Introduction to Block Withholding Attack in PoW Blockchain.
---

## PoW Mining Model

As we all know (?), in PoW (proof of work) blockchain system, miners can mine blocks to gain block reward (bitcoins). Mining blocks requires the miner to prove that they have done a certain amount of work. The most common proof of work is to find a hash that satisfy certain properties. For example, the hash value of a string in integer is no larger than a certain number $N$. Assume the range of the output of hash is $R$, then the probability of a random hash to be valid will be $\frac{N}{R}$.Due to the nature of cryptography secure hash functions, the distribution of output hash is unpredictable given any different strings as input, which means we may assume that the probability of any string to has a valid hash is $\frac{N}{R}$. Note that there can be other means of constrains for a block to be valid, but the core is simple: the probability of a hash to be valid will be $p$, which is typically a small number.

Let's take a look at a well-known real-world example: Bit-Coin. In Bit-Coin, the threshold $N$ is related to **difficulty**. Trivially, the higher the difficulty is, the lower the threshold will be. This is threshold is adjusted dynamically by the system in response to the overall computing power in the Bit-Coin mining system. It's public to all and can't be configured manually. This is not the core of this blog post, so we will skip on this.

## Mining Pool

Now, the problem is clear: as a miner, you want to be make money by mining Bit-Coin. However, if you mine alone (called solo miner), the variance of finding a block is large, which means higher risks.

In Bit-Coin, the difficulty is adjusted to ensure that every new block will be found in around $T = 10$ minutes. Suppose the hash compute power of you is $h$, and the overall compute power in Bit-Coin mining is $H$, then in expectation you will find a valid block in $\frac{H}{h} \times T$ minutes. Since the probability of finding a block is the same at any time, the probability distribution adheres to possion distribution. As a result, the probability density function that the player find a block at $t$ is $F(t) = \lambda e ^{\lambda t}$. Note that $\mathbb{E}(tF) = \frac{1}{\lambda}$, so we have $\lambda = \frac{HT}{h}$, the variance of a solo miner is $\text{Var}(F) = \lambda = \frac{HT}{h} \propto \frac{H}{h}$

That would be too big and thereby risky for a personal miner, as the proportion of personal hash power is rather small. For example, $H$ is around $800 \text{EH/s}$. $\text{EH/s}$ is a unit which stands for $10^{18}$ hash operation per second. For a personal computer's GPU (e.g. RTX 3090), the hash power $h$ is around $100 {MH/s}$, which is magnitudes smaller than $H$. For some designated hash machines, the $h$ is around $100 {TH/s}$, still much slower than $H$.
