---
title: GDB 使用笔记
date: 2024-03-02 12:05:14
updated: 2024-05-24 12:05:14
tags: [调试]
categories: [计算机, 工具]
keywords: [调试工具]
cover: https://s3.bmp.ovh/imgs/2024/02/19/42c9a6adfd88c991.jpg
mathjax: false
description:
---
写 Kernel 的时候，需要用到 GDB + QEMU 调试，这里记录一些 GDB 的常用指令，会动态更新。

## 简单安装

首先，需要安装 `riscv64-unknown-elf-gdb`。作为一个懒狗，笔者参考了 rcore tutorial 的[安装教程](https://rcore-os.cn/rCore-Tutorial-Book-v3/chapter0/5setup-devel-env.html#gdb)。需要注意的是，按照该教程下载完 `.tar.gz` 文件后，需要解压，然后把解压后 /bin 里面的 `riscv64-unknown-elf-gdb` 移动到 `/usr/local/bin` 下即可。

## 启动指令

默认是 riscv64-unknown-elf-gdb。QEMU 采用默认端口 1234。其中 xxx 是可执行文件路径。

```bash
riscv64-unknown-elf-gdb \
    -ex 'file xxx' \
    -ex 'set arch riscv:rv64' \
    -ex 'target remote localhost:1234'
```

## 调试指令

| 指令       | 参数    | 作用                          | 缩写 |
| ---------- | ------- | ----------------------------- | ---- |
| backtrace  | -       | 查看函数调用栈                | bt   |
| breakpoint | (*addr) | 在地址 addr /当前位置设置断点 | b    |
| continue   | -       | 继续执行程序，直到断点        | c    |
| delete     | (num)   | 删除第 num 个/所有断点        | d    |
| disable    | (num)   | 禁用第 num 个/所有断点        | dis  |
| enable     | (num)   | 启用第 num 个/所有断点        | e   |
| info       | ...     | 显示具体信息                  | i    |
| print      | expr    | 显示表达式的值                | p    |
| x          | addr    | 显示内存地址 addr 的内容      | x    |
| list       | -       | 显示当前执行的代码是哪个文件   | l    |
| step       | (num)   | 执行 num/单行代码, 会进入函数 | s    |
| next       | (num)   | 执行 num/单行代码, 会跳过函数 | n    |

需要注意的是, 如果 `next` 和 `step` 后添加后缀 `i`, 即 `nexti` 和 `stepi`, 则对应的是汇编指令级别的执行 (即一条汇编指令), 而不是 C 代码级别的执行 (即一行 C 代码), 遇到函数的处理和 `next` 和 `step` 是类似的, 缩写对应的是 `ni` 和 `si`.

以下是一些常用指令:

```bash
x/4 0x11451400  # 显示内存 0x11451400 地址开始的 4 个 word
x/5i $pc        # 显示当前 pc 往后 5 条指令。i 表示显示汇编指令
                # $pc 这种形式可用于显示寄存器的值
x/3i $pc + 4096 # 显示 pc + 4096 地址开始的 3 条指令
                # 事实上，参数貌似可以是任意表达式
p/d $t0         # 显示寄存器 t0 的值, /d 表示以 10 进制显示
p/x 114514      # 显示 114514 的 16 进制表示，不过这么做挺无聊的...
p/t 114514      # 显示 114514 的 2 进制表示
p/c 48          # 显示 48 的字符表示 (这里是'0')
```

这里简单总结一下显示数值类型的参数:

| 参数 | 显示格式    | 寻址单位 (byte) |
| ---- | ------------ | -------- |
| /a   | 十六进制 | 8 |
| /b   | 不变     | 1 |
| /c   | 字符     | 1 |
| /d   | 十进制   | 不变 |
| /f   | 浮点数   | 4 或 8 |
| /h   | 不变     | 2 |
| /i   | 指令     | 4 |
| /o   | 八进制   | 不变 |
| /s   | 字符串   | 字符串 |
| /t   | 二进制   | 不变 |
| /u   | 无符号   | 不变 |
| /w   | 不变     | 4 |
| /x   | 十六进制 | 不变 |
