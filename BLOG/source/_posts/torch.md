---
title: dispatcher is all you need
date: 2024-11-27 02:08:43
updated:
tags: [C++]
categories: [C++, Framework]
keywords: [C++, pytorch]
cover: https://s3.bmp.ovh/imgs/2024/11/27/3e28dd3239526ce1.jpg
mathjax: false
description: pytorch 写的是真的好. 本文会简单分析 torch 的 dispatcher, 以及实现在 C++ 侧实现自定义的 torch extension.
---

<!-- 咕咕咕, 等 OSDI 后再写. 如果你来 ping 我, 我会更快的更新. -->

[参考文献](http://blog.ezyang.com/2020/09/lets-talk-about-the-pytorch-dispatcher/). 这篇很经典, 讲的比我好多了. 本文只是基于笔者的实践, 对于原文的选择性翻译. 本文全部图片来自那篇 blog.

具体的实验配置请参考 [环境配置](#环境配置) 章节.

## 前言

在你使用 PyTorch 的时候, 你是否有想过, 对应的函数例如 `torch.add` 具体是如何操纵数据的? 更进一步, 在你调用 `with torch.no_grad()` 的时候, 究竟是什么机制使得所有的 kernel 都不会保留 gradient?

你可能会说: `if` ! 对的, 等效的来看, `torch` 的实现无非是借助一个一个的 `if`, 如果你的 tensor 在 GPU 上, 并且你的数据类型是 `float16`, 那么就会调用对应的 `cuda + float16 + add` 的 kernel.

```python
if dtype == torch.float16 and device.type == 'cuda':
    return cuda_float16_add
```

但是, 考虑到 dtype 不是一成不变的 (学术界会提出越来越多的 quantization dtype 例如 `bfloat16`, `float8_e5m2`), 以及未来潜在的新的 device (比如 TPU, FPGA 等等). 在考虑可拓展性的情况下, 如果每种 kernel 都写成这种又臭又长的 `if` 条件判断的话, 那么后期的代码维护将会变得异常困难. 每多一个新的 dtype 或者 device, 开发者就需要把每个 kernel 都加上新的 `if`. 显然, 我们希望对于一个新的场景, 可以在不侵入修改原有代码的情况下实现.

这时候, 你可能会从你大一的程序设计课程中寻找灵感, 于是(笔者假设)你找到了一个类似的场景: 对于同一个函数, 表现出多态性, 可以使用虚函数 + 继承来实现. 这的确可以直接解决烦人的一堆 `if`, 但也有自己的局限性.

1. 虚函数一般是由虚表实现的, 而虚表是静态的, 无法在运行时注册用户的函数, 需要在编译期就全部确定, 用户拓展体验极差.
2. 虚函数无法做到 bypass. 即对于同一个 tensor 的同一个函数 (比如前面的 `cuda + fp16 + add`), 在不同状态下 (比如 `with torch.no_grad()`), 选择不同的 kernel 去调用.

为了更好的动态性和可维护性, 我们需要一套更加灵活的框架, 于是就有了 torch 的 Dispatcher.

## 一些术语

`op`(operator): 算子, 表示对于数据的某种抽象操作, 比如 `add`.
`kernel`: 算子的某种具体实现 (一般场景下可能指的是 cuda kernel, 但是本文放宽到所有类型的实现)
`dtype`: torch 中表示数据类型的类, 常见的数据类型有 `int32`, `float32` 等等.
`device`: torch 中表示数据所在的设备的类, 常见的设备有 `cuda`, `cpu` 等等.
`layout`: torch 中表述数据存储形式的类, 常见的有 `strided`, `sparse_coo` 等等.
`Tensor`: torch 的核心数据抽象, 可以理解为一块多维的矩阵, 可以认为是一个 `shared_ptr` + 内部实现.
`IValue`(Interpreter Value): torch 的一种值类型, 类似 `std::any`, 可以存储常见值类型 (比如 `Tensor`, `int`).

## 什么是 Dispatcher

Dispatcher, 简单来说, 就是每个 op 具体应该调用哪个 kernel 的决定者. 决定调用哪个 op, 首先肯定要取决于输入 op 的 tensor 的本身的 dtype 和 device. 同时, 为了能够支持全局的某些开关 (比如 `torch.no_grad()`), 我们在 dispatch 的时候也需要考虑全局 global 的一些设定.

于是乎, 我们可以开始快乐的手搓动态虚表啦. 我们给每个可能的 key (op + device + dtype) 注册一个函数, 保留对应的函数指针. 但是, 事情并没有想象的那么简单.

### Boxing

首先, 不同的 op 接受的参数是不一样的. `add` 需要输入两个 tensor 作为输入, 但是 `to` 就需要一个 tensor 加上对应的 dtype 或者 device. C++ 作为一种静态语言, 调用的 function 的函数签名必须是编译期确定并且匹配的, 但是每个 op 的函数签名是不尽相同的. 如果要中心化的用 dispatcher 来管理 dispatch, 那么 C++ 静态特性要求你的函数签名必须是完全相同的, 否则你就需要给每种函数签名单独维护一个 mapping, 这样的代码也会变得臃肿而难以维护.

因此在这里, 我们需要引入 boxing 的概念. boxing 指的是, 我们对于 torch 中的用到的类型, 比如 `tensor`, `int64`, `float32` 等等, 统一装到一个类型里面. 看到这里, 相信读者很自然的能联想到 `std::any`, `std::variant` 之类的东西. 这就是 torch 里面重要的值类型: `IValue`. 他把类型签名全部抹去, 把不同的类都尽可能装入了同一个 `IValue` 类里面, 从而部分解决了 C++ 静态类型的问题. 当然, 这样是不够的, 除了参数类型不同, 参数数量可能也是不一样的, 所以我们还需要手动维护一个 std::vector 类似的结构, 使得输入的参数类型可以是变长的即可. 以下即为一种简单的实现:

```cpp
// Possible combination of keys
struct KeyType {
    std::string op_name;
    Dtype data;
    Device device;
};
// Possible Implementation
struct DispatchTable {
    std::unordered_map<KeyType, std::function<std::any(std::vector<std::any>)>> table;
};
```

![关于 box 的示意图](http://blog.ezyang.com/img/pytorch-dispatcher/slide-18.png).

通过 boxing, 我们就可以很方便的中心化管理一个大 mapping, 其本质上是 C++ 中类型擦除的思想, 在 `std::any` 和 `std::variant` 中都有涉及.

### Dispatch Logic

boxing 解决了接口一致性的问题, 使得中心化管理变得可能. 但这样一个框架, 他不一定方便. 假如 torch 真的采用了类似我们的伪代码的实现, 那么其很难支持动态的插入 `torch.no_grad()` 等涉及全局状态逻辑.

对于每一种涉及全局状态的逻辑, 如果把 global state 也直接暴力融入 dispatch key 之中, 那么就会遇到复杂度爆炸: 对于每一种可能的组合, 你都需要给予一种映射规则, 但是在考虑上全局状态后, 每一个额外的状态都会使得组合的可能乘以 2 (e.g. device + dtype + auto_grad + tracing......). 而全局状态到处都是: tracing, auto_grad, fake tensor 等等. 重新审视我们的全局状态, 很多时候我们其实不 care 两个 state 同时启用的时候是如何协同的, 在层层抽象之后, 不同模块之间应当已经良好的解耦了.

对于 tracing, auto_grad, fake_tensor 这些类似 hook 的功能, 我们其实只是希望能在调用真实的 kernel 前后先做点事情. 更近一步, 我们更希望的是他能像 python 的 decorator 一样, 由具体的实现来决定是否继续调用下面的 kernel:

```python
def dummy_hook_function(f, args):
    # in reality, you may do something before/after `f`
    # you may even avoid calling `f` under certain circumstance
    return f(args)
# original
real_kernel(args);
# original + hook
hook_function(real_kernel, args);
```

因此, 我们实际要做的是, 对于某些全局状态, 如果启用了, 那么在 dispatch 的时候, 在调用真实 kernel 的前后插入一些执行逻辑.

```cpp
// Possible combination of keys
struct KeyType {
    std::string op_name;
    Dtype data;
    Device device;
};
// Possible Implementation
struct DispatchTable {
    using Args_t = std::vector<std::any>;
    using F = std::function<std::any(Args_t)>;
    std::unordered_map<KeyType, F> table;
    std::vector <std::pair<F, Args_t>> hooks;
};
```

事实上, 复杂度爆炸一个很重要的原因就是, 存在协同操作的可能. 如果功能两两不相交, 那么 dispatch 的时候只需要调用最高优先级的那个 kernel 就可以了, 是否继续往下调用取决于最高优先级的 kernel, 这些适用于绝大部分的全局状态. 如果维护两两相交的状态, 假设一共有 $n$ 种状态, 那么复杂度一下子就从 $O(n)$ 上升到了 $O(n^2)$, 更别说所有的一起考虑, 那就是 $O(2^n)$ 了.

在实际的 torch dispatcher 的实现中, 是以 op 为中心维护的. 对于一个 op, 其维护了一个 key_set, 表示可能的 backend (即 device) 以及 functional key (即前面说的 Fake Tensor, Autocast 之类的). 而 DispatchKeySet 则是由两者拼接而成的, 本质上是一个 bitset.

```cpp
// Part of the code from PyTorch: include/c10/core/DispatchKey.h

#define C10_FORALL_BACKEND_COMPONENTS(_, extra) \
  _(CPU, extra)                                 \
  _(CUDA, extra)                                \
  _(HIP, extra)                                 \
  _(XLA, extra)                                 \
  _(MPS, extra)                                 \
  _(IPU, extra)                                 \
  _(XPU, extra)                                 \
  _(HPU, extra)                                 \
  _(VE, extra)                                  \
  _(Lazy, extra)                                \
  _(MTIA, extra)                                \
  _(PrivateUse1, extra)                         \
  _(PrivateUse2, extra)                         \
  _(PrivateUse3, extra)                         \
  _(Meta, extra)

enum class BackendComponent : uint8_t {
  InvalidBit = 0,
#define DEFINE_BACKEND_COMPONENT(n, _) n##Bit,
  C10_FORALL_BACKEND_COMPONENTS(DEFINE_BACKEND_COMPONENT, unused)
#undef DEFINE_BACKEND_COMPONENT

  // Define an alias to represent end of backend dispatch keys.
  // If you add new backend keys after PrivateUse3, please also update it here.
  EndOfBackendKeys = MetaBit,
};

enum class DispatchKey {
  // ...

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FIN ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
  EndOfFunctionalityKeys, // End of functionality keys.
};

static_assert(
    (static_cast<uint8_t>(BackendComponent::EndOfBackendKeys) +
     static_cast<uint8_t>(DispatchKey::EndOfFunctionalityKeys)) <= 64,
    "The BackendComponent and DispatchKey enums (below EndOfFunctionalityKeys)"
    " both map to backend and functionality bits"
    " into a 64-bit bitmask; you must have less than 64 total entries between them");
```

```cpp
// Part of the code from PyTorch: include/c10/core/DispatchKeySet.h

class DispatchKeySet final {
public:
  constexpr explicit DispatchKeySet(BackendComponent k) {
    if (k == BackendComponent::InvalidBit) {
      repr_ = 0;
    } else {
      repr_ = 1ULL << (static_cast<uint8_t>(k) - 1);
    }
  }

  constexpr explicit DispatchKeySet(DispatchKey k) {
    // NOLINTNEXTLINE(bugprone-branch-clone)
    if (k == DispatchKey::Undefined) {
      // Case 1: handle Undefined specifically
      repr_ = 0;
    } else if (k <= DispatchKey::EndOfFunctionalityKeys) {
      // Case 2: handle "functionality-only" keys
      // These keys have a functionality bit set, but no backend bits
      // These can technically be either:
      // - valid runtime keys (e.g. DispatchKey::AutogradOther,
      // DispatchKey::FuncTorchBatched, etc)
      // - "building block" keys that aren't actual runtime keys (e.g.
      // DispatchKey::Dense or Sparse)
      uint64_t functionality_val = 1ULL
          << (num_backends + static_cast<uint8_t>(k) - 1);
      repr_ = functionality_val;
    } else if (k <= DispatchKey::EndOfRuntimeBackendKeys) {
      // Case 3: "runtime" keys that have a functionality bit AND a backend bit.
      // First compute which bit to flip for the functionality.
      auto functionality_k = toFunctionalityKey(k);
      // The - 1 is because Undefined is technically a "functionality" that
      // doesn't show up in the bitset. So e.g. Dense is technically the second
      // functionality, but the lowest functionality bit.
      uint64_t functionality_val = 1ULL
          << (num_backends + static_cast<uint8_t>(functionality_k) - 1);

      // then compute which bit to flip for the backend
      // Case 4a: handle the runtime instances of "per-backend functionality"
      // keys For example, given DispatchKey::CPU, we should set:
      // - the Dense functionality bit
      // - the CPUBit backend bit
      // first compute which bit to flip for the backend
      auto backend_k = toBackendComponent(k);
      uint64_t backend_val = backend_k == BackendComponent::InvalidBit
          ? 0
          : 1ULL << (static_cast<uint8_t>(backend_k) - 1);
      repr_ = functionality_val + backend_val;
    } else {
      // At this point, we should have covered every case except for alias keys.
      // Technically it would be possible to add alias dispatch keys to a
      // DispatchKeySet, but the semantics are a little confusing and this
      // currently isn't needed anywhere.
      repr_ = 0;
    }
  }

private:
  constexpr DispatchKeySet(uint64_t repr) : repr_(repr) {}
  uint64_t repr_ = 0;
};
```

当然, 在实践中, torch 还有很多的细节, 比如 DispatchKey enum 并不只有 functional key, 还有 backend + functional 的交叉 key 组合 (比如 SparseCUDA, QuantizedMPS). 在计算 DispatchKeySet 的时候, 对于组合的 key 会手动的拆成 bitset 中的两个 bit. 在实际决定按照哪个 dispatch key 优先的时候, 会把所有的 tensor input 含有的 key 汇集在一起, 加上全局的一些状态量, 最后先调用 DispatchKeySet 中的最高的那个 bit 对应的 kernel.

> Remark: 如果仔细看一眼 DispatchKeySet 的构造函数的逻辑, 你会发现 functionality 的 key 的优先级是高于 device 的 key 的. 这很合理, 功能性一般都是 override 在 function 之上的.

![Which kernel shall i call?](http://blog.ezyang.com/img/pytorch-dispatcher/slide-07.png)

## 实验部分

说了这么多, 该写代码了. Talk is cheap, show me the code.

### 环境配置

笔者使用的是 `nvcc 12.4` + `PyTorch 2.5.1` + `gcc 13.2` + `clangd 19.1.2` 的组合. 关于 torch 的安装, 请参考 [官网](https://pytorch.org/get-started/locally/). 笔者选择的是 `2.5.1 + Linux + pip + cuda 12.4`. 环境管理笔者用的是 conda:

```bash
conda create -n torch python=3.10 -y
# 这一段请参考官网配置
conda activate torch
pip3 install torch torchvision torchaudio
```

然后创建一个 `test.cu` 文件, 作为测试代码用的文件, 后面的改动都将放在这个文件中. 这里参考了 torch 官方给出的 [custom C++ extension](https://pytorch.org/tutorials/advanced/cpp_extension.html) 的部分.

```cpp
#include <torch/extension.h>
PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
}
```

为了 setup 整个项目, 还需要一个 setup.py 文件, 随后只需要 `python setup.py install` 即可编译并安装 `my_test` 库.

```python
from setuptools import setup
from torch.utils import cpp_extension

setup(name='my_test',
      ext_modules=[cpp_extension.CppExtension('my_test', ['test.cu'],
                                              extra_compile_args=['-O3'])],
      cmdclass={'build_ext': cpp_extension.BuildExtension})
```

当然, VSCode 默认 C/C++ 插件的代码补全显然是找不到需要的头文件的, 这会使得我们的编程变得极其痛苦. 笔者推荐使用 clangd 插件 + compile_commands.json.

```json
[
    {
        "directory": "/home/dark/workspace/torch",
        "arguments": [
            "/usr/local/cuda/bin/nvcc",
            "-I/home/dark/miniconda3/envs/torch/lib/python3.10/site-packages/torch/include",
            "-I/home/dark/miniconda3/envs/torch/lib/python3.10/site-packages/torch/include/torch/csrc/api/include",
            "-I/home/dark/miniconda3/envs/torch/lib/python3.10/site-packages/torch/include/TH",
            "-I/home/dark/miniconda3/envs/torch/lib/python3.10/site-packages/torch/include/THC",
            "-I/home/dark/miniconda3/envs/torch/include/python3.10",
            "-c",
            "test.cu",
            "-o",
            "build/temp.linux-x86_64-cpython-310/test.o",
            "-D__CUDA_NO_HALF_OPERATORS__",
            "-D__CUDA_NO_HALF_CONVERSIONS__",
            "-D__CUDA_NO_BFLOAT16_CONVERSIONS__",
            "-D__CUDA_NO_HALF2_OPERATORS__",
            "-O3",
            "-DTORCH_API_INCLUDE_EXTENSION_H",
            "-DPYBIND11_COMPILER_TYPE=\"_gcc\"",
            "-DPYBIND11_STDLIB=\"_libstdcpp\"",
            "-DPYBIND11_BUILD_ABI=\"_cxxabi1011\"",
            "-DTORCH_EXTENSION_NAME=my_test",
            "-D_GLIBCXX_USE_CXX11_ABI=0",
            "-std=c++17"
        ],
        "file": "test.cu"
    }
]
```

读者只需修改其中 miniconda, nvcc, 工作目录等路径即可. 当然, 这样可能不是非常具有可拓展性. 笔者获取 compile_commands.json 的方式是, 先直接运行一次 `python setup.py install`, 他会在命令行输出对应的编译指令. 读者可以复制其给出的编译指令, 然后用以下的 python 脚本拆分:

```python
msg = "" # 这里换成你的编译命令
seg = msg.split()
import json
print(json.dumps(seg, indent=4))
```

至此, 你的 clangd 大概是能工作了, 读者也可以自行删除一些会让 clangd 无法识别的命令行参数.

### 年轻人的第一个 pybind

首先先写一个 Hello World 来熟悉一下 pybind 的流程, 这里暂时不涉及 torch 的内容. pybind 简单来说就是允许你从 python 一侧调用 C++ 的函数, 对于基本类型提供了自动转换的功能 (比如 C++ 的 bool 到 python 的 bool). 我们在 test.cu 中加入一个 `hello` 函数:

```cpp
#include <iostream>
#include <string>
#include <torch/extension.h>

static auto hello(int n) -> std::string {
    std::cout << "Hello World! " << n << std::endl;
    return std::to_string(n);
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
    m.def("hello_world", hello); // Define hello_world function in python
}
```

然后, 使用 `python setup.py install` 来编译并安装. 在安装好后, 在 python 中运行以下的代码:

```python
import torch # 这是一个小细节, 似乎必须先 import torch 再 import 自己的拓展
import my_test

x = my_test.hello_world(1)
# print(x, type(x))
```

你应该会得到意料之中的结果, 即输出了 `"Hello World! 1"`, 并且 x 是字符串 `"1"`.

### 年轻人的第一个 torch extension

当然, 我们写 torch extension 大概率不只是为了用到 pybind 的功能, 我们可能还想要自己定义一些 op. 在 torch 中, 你可以自己注册一个 op, 也可以为已有的 op 按照一定规则绑定 key 和 kernel. 简单来说, 有三种注册模式:

1. 注册一个新的 op 并且绑定默认的 kernel
2. 为某一个 key 绑定 kernel (fallback)
3. 为某个 op + key 绑定 kernel

这里引用一下原文的图来解释:

![new op](http://blog.ezyang.com/img/pytorch-dispatcher/slide-15.png)
![functionality](http://blog.ezyang.com/img/pytorch-dispatcher/slide-16.png)
![op + functionality](http://blog.ezyang.com/img/pytorch-dispatcher/slide-14.png)
![summary](http://blog.ezyang.com/img/pytorch-dispatcher/slide-17.png)

下面, 我们用一些简单的代码来演示这些功能.

> Remark: 以下部分和原 blog 稍有出入, 毕竟那篇 blog 都是 5 年前的玩意了

```cpp
#include <cstddef>
#include <iostream>
#include <string>
#include <torch/extension.h>

static auto hello(int n) -> std::string {
    std::cout << "Hello World! " << n << std::endl;
    return std::to_string(n);
}

static auto add_1_forward(at::Tensor x) -> at::Tensor {
    auto &&op  = c10::Dispatcher::singleton().findSchemaOrThrow("aten::add_1", "");
    auto stack = c10::Stack{};
    stack.push_back(static_cast<at::TensorBase>(x));
    // call boxed kernel
    op.callBoxed(&stack);
    return stack[0].toTensor();
}

static auto add_1_cpu(at::Tensor x) -> at::Tensor {
    return x + 1;
}

static constexpr auto kKeySet = c10::DispatchKeySet{c10::DispatchKey::TESTING_ONLY_GenericMode};

static auto custom_hook(const c10::OperatorHandle &op, c10::Stack *stack) -> void {
    const auto kGuard   = c10::impl::ExcludeDispatchKeyGuard{kKeySet};
    static auto counter = std::size_t{};
    std::cout << "Custom hook called " << counter++ << " times" << std::endl;
    return op.callBoxed(stack);
}

static auto enable_hook(bool enable) -> void {
    static auto guard = std::optional<c10::impl::IncludeDispatchKeyGuard>{};
    if (enable) {
        guard.emplace(kKeySet);
    } else {
        guard.reset();
    }
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
    m.def("hello_world", hello); // Define hello_world function in python
    m.def("add_1", add_1_forward);
    m.def("enable_hook", enable_hook);
}

TORCH_LIBRARY_FRAGMENT(aten, m) {
    // register add_1 function for aten namespace
    m.def("add_1", [](at::Tensor) -> at::Tensor {
        throw std::runtime_error("Debug: Not implemented yet!");
    });
}

TORCH_LIBRARY_IMPL(aten, CPU, m) {
    // register add_1 function for aten namespace
    m.impl("add_1", add_1_cpu);
}

TORCH_LIBRARY_IMPL(_, TESTING_ONLY_GenericMode, m) {
    m.fallback(torch::CppFunction::makeFromBoxedFunction<custom_hook>());
}
```

代码很长, 但是核心是最后三个以 TORCH_LIBRARY 开头的东西. 如果你希望在原有的 aten library 里面增加 op, 需要用到 `TORCH_LIBRARY_FRAGMENT` 这个宏,  `TORCH_LIBRARY` 代表的是一个新的 library, 这一点和原文略有不同. 在编译完上述代码之后, 可以用以下 python 代码进行测试:

```python
import torch
import my_test

x = torch.tensor([1.0, 2.0, 3.0])
print(my_test.add_1(x))

my_test.enable_hook(True)
y = my_test.add_1(x)
y = y + 1
my_test.enable_hook(False)

print(y)
```

理论上, 你应该看到以下的输出:

```text
tensor([2., 3., 4.])
Custom hook called 0 times
  Operator: aten::add_1
Custom hook called 1 times
  Operator: aten::add
tensor([3., 4., 5.])
```

这里鼓励读者多做一些尝试. 比如尝试 `x = x.to('cuda')`, 看看这时候再调用 `my_test.add_1(x)` 有何效果. 同时, 读者也可以尝试去除 `custom_hook` 中的 `kGuard`, 这个 guard 的作用是在构造的时候临时 disable 指定的 DispatchKeySet, 并且在析构的时候恢复原样 (类似 `std::lock_guard` 的原理), 看看结果是否和你想的一样. 或者, 读者也可以自行修改 `custom_hook`, 比如尝试不调用 `callBoxed` 等等. 同时, 观察一下 `custom_hook`, 相信读者也能意识到 boxing 的重要性: 如果没有 boxing, 根本无法为不同函数签名的各种 op 注册一个统一的 fallback.

## 总结

没啥好总结的, PyTorch 为解决 kernel 分发问题提出的 Dispatcher 抽象是非常成功的. 其解决了多种功能 key 组合带来的复杂度爆炸的问题, 把 backend 和 functionality 作为 DispatchKeySet 的一部分, Dispatcher 每次调用最高优先级的 key 对应的 kernel, 由被调用的 kernel 决定是否继续 redispatch 调用其他 kernel, 并且提出了 boxing 的抽象, 成功解决了虚函数的静态性的问题, 实现了一套统一的 dispatch 的机制, 避免了满天飞的 `if` 特判.

这一套框架乍一看很自然, 但是在你自己写的是很难想到如此多的细节的, 很容易写着写着就多出来一堆 `if`, 最后代码逻辑复杂到自己都看不懂. 不仅如此, PyTorch 还做了大量工程上的优化, 提供了许多工程上的便利, 这部分非常考验 C++ 的功底. 比如 DispatchKeySet 实际上就是压位压到了 64-bit 以内, 从而可以用一个整数直接表示, 这样避免了用动态 bitset 甚至是 vector 之类的结构, 减少了内存占用. 同时, 通过 `DispatchKeyGuard` 可以很方便的临时 `enable/disable` 某些全局 key, 这类 RAII 的思想也使得代码变得非常清晰易读 (这里点名批评 C 的 goto 作为 cleanup). 而 boxing 的核心 `IValue` 本身也用到了 `sso` 的思路 (可以参考 <a href="{% post_path cpp %}#small-size-optimization"> 这一部分 </a>), 从而减少了内存的分配, 增强了数据的局部性, 提升了整体的性能.

总之, 要多看啊.
