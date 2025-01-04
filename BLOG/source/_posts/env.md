---
title: 我讨厌配环境
date: 2024-07-25 20:51:42
updated: 2024-07-27 21:50:59
tags: [基础知识]
categories: [计算机, 工具]
keywords: [cuda, gpg, Python]
cover: https://s3.bmp.ovh/imgs/2024/07/27/3a45622b731f9c3d.png
mathjax: false
description: 配环境, 然后破防.
---

你说得对, 但是又到了经典的配环境时刻. 每次配环境都是一个令人头疼的过程, 在没有配好环境写不了代码的时候, 内心总是感到非常焦急, 只能干楞着却做不了事情. 参考了 Conless 的配环境 [cheat sheet](https://conless.dev/blog/2024/server-cheatsheet/), 笔者决定也记录一下万恶的配环境之路.

以下的所有环境配置默认是在 Linux 进行.

## shell

在第一次连上远程服务器的时候, 需要配置 shell, 否则连 vscode 都直接通过 ssh 连上去.

我拿到的服务器集群默认 shell 是 zsh, 笔者由于之前没有使用过 zsh, 因此完全按照的是 [Wankupi](https://www.wankupi.top) 同学的建议配置的, 简单来说就是 zsh + oh-my-zsh. 过程几乎完全参考 [https://ohmyz.sh/](https://ohmyz.sh/), 只需要一条指令即可:

```shell
sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
```

笔者不太熟悉 zsh, 如果想要为 zsh 添加更多更强的插件, 请参考前面 Conless 的 [blog](https://conless.dev/blog/2024/server-cheatsheet/)

在配置好最简单的 shell 环境以后, 就可以通过 VSCode remote-SSH 连接到主机, 快乐的 coding & exploring 了!

### 一些笔者新学会的小知识

> 小贴士: 可以试着 `chmod 700 .`, 保护你的文件, 阻止别人看到你的文件!

我们的集群中, 新开的用户的目录默认权限是 755. 这是很危险的, 因为 5 = 4 | 1, 表示其他人可以读/执行你的文件. chmod 的每一位可以是 0 ~ 7 中的任何一个数, 其中 bit 0 (1) 表示执行权限, bit 1 (2) 表示写权限, bit 2 (4) 表示读权限. 你可以通过 `stat .` 查看当前目录的权限.

```shell
# 第一行含有 Access: (0755...
# 其中 0755 是当前的权限
# 当然, Linux only
stat . | grep Access
```

一般有四个数字 (从高到低的第一个貌似经常省略 (?)), 有如下含义:

1. 第一个数字: 笔者不知道, 貌似不常用.
2. 第二个数字: 表示文件所有者的权限.
3. 第三个数字: 表示文件所属组的权限.
4. 第四个数字: 表示其他用户的权限.

~~不过貌似集群会定期重置为 0755, 貌似没啥用~~

## miniconda

笔者需要配 python 环境, 自然是逃不了配 conda. 笔者选择的是 miniconda. 这个东西的安装也是非常的简单, 没有什么技术难度, 直接按照 [miniconda 官网](https://docs.anaconda.com/miniconda/)的教程来就行了.

```shell
mkdir -p ~/miniconda3
wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda3/miniconda.sh
bash ~/miniconda3/miniconda.sh -b -u -p ~/miniconda3
rm -rf ~/miniconda3/miniconda.sh
```

最后, 别忘记了给自己的 shell 注入一下 conda 的初始化 setup :).

```shell
# bash version
~/miniconda3/bin/conda init bash
# zsh version
~/miniconda3/bin/conda init zsh
```

在这以后, 就可以正常的 conda 初始化了~

## cuda

幸运的是, 集群是有必要的 cuda 环境的, 这免去了我二次坐牢的体验. 为什么是二次呢, 因为在这之前几天, 我刚刚在本机上配了 cuda 的环境, 简直是一个坐牢. 这里简单讲一讲我在本机 Windows + WSL2 环境下是如何配置 cuda 环境的.

~~首先, 你必须要有nvidia 显卡和驱动~~ 一般来讲, 一台有 nvidia 的显卡, 安装了驱动后, 可以在命令行输入 nvidia-smi 查看驱动信息. 笔者笔记本上的 cuda 驱动信息大致如下所示:

```plaintext
+---------------------------------------------------------------------------------------+
| NVIDIA-SMI 546.80                 Driver Version: 546.80       CUDA Version: 12.3     |
|-----------------------------------------+----------------------+----------------------+
| GPU  Name                     TCC/WDDM  | Bus-Id        Disp.A | Volatile Uncorr. ECC |
| Fan  Temp   Perf          Pwr:Usage/Cap |         Memory-Usage | GPU-Util  Compute M. |
|                                         |                      |               MIG M. |
|=========================================+======================+======================|
|   0  NVIDIA GeForce RTX 3050 ...  WDDM  | 00000000:01:00.0  On |                  N/A |
| N/A   47C    P5               8W /  60W |   1796MiB /  4096MiB |     15%      Default |
|                                         |                      |                  N/A |
+-----------------------------------------+----------------------+----------------------+

+---------------------------------------------------------------------------------------+
| Processes:                                                                            |
|  GPU   GI   CI        PID   Type   Process name                            GPU Memory |
|        ID   ID                                                             Usage      |
|=======================================================================================|
|    0   N/A  N/A      2148    C+G   ...e\VSCode\Microsoft VS Code\Code.exe    N/A      |
+---------------------------------------------------------------------------------------+
```

非常的低配, 但是勉强够用了(真生产力还是得靠集群).

在此之后, 需要安装对应版本的 cuda toolkit. 比如, 笔者在 WSL2 安装的就是 cuda 12.3 toolkit. 安装方法也是非常的简单, 直接在浏览器搜索: cuda toolkit 12.3, 然后跟着教程来. 例如 [cuda toolkit 12.3](https://developer.nvidia.com/cuda-12-3-0-download-archive?target_os=Linux)

```shell
# WSL-Ubuntu 的安装方法, 具体请参考本机配置及官网
wget https://developer.download.nvidia.com/compute/cuda/repos/wsl-ubuntu/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb
sudo apt-get update
sudo apt-get -y install cuda-toolkit-12-3
```

在此之前, 笔者曾试图安装和驱动 cuda version 不一致的 cuda toolkit 12.5, 结果在编译的时候喜提链接错误以及头文件缺失, 所以保险起见还是暂时先安装同版本的 toolkit.

特别注意的是, 对于部分较老的 cuda toolkit, 其不一定支持新版本的 gcc, 这意味着你可能需要把你的 gcc 降级. 笔者用 gcc-13 + nvcc 12.3 编译会提示不支持, 如果强行忽略会出现链接错误, 降低版本到 gcc-12 后解决.

笔者本地管理版本用的是 update-alternatives, 可以方便的在不同版本的 gcc 中切换 (即, 使得默认的 gcc --version 得到不同的结果,).

在此, 笔者特别感谢 [Conless Pan](https://conless.dev/) 同学在这过程中给予的帮助!

## git + ssh + gpg

配完 conda, 笔者第一件做的事情就是 `git clone` 仓库, 然后发现本地并没有 ssh-key :(.

配置 ssh-key 也不难, 只需要按照 github 的官方指令来就行了, [链接在此](https://docs.github.com/zh/authentication/connecting-to-github-with-ssh), 重点是生成新的 ssh-key 以及添加到 github.

```shell
# 请把下面字符串中的邮箱替换成你自己的 :(
ssh-keygen -t rsa -b 4096 -C "2040703891@qq.com"
# 把打出来的东西直接加到 github 的 ssh-key 里面即可
cat ~/.ssh/id_rsa.pub
```

当系统提示 "Enter a file in which to save the key" 时，按 Enter 键接受默认文件位置, 简而言之就是一路默认.

当然, 之前在本机 WSL 环境下配置了 gpg, ~~github 签名 verified 是真的好看~~, 所以我们也需要在新的设备上配置 gpg. 这一步也不难, 同样是按照 github 的官方教程来, [链接在此](https://docs.github.com/zh/authentication/managing-commit-signature-verification/generating-a-new-gpg-key).

```shell
gpg --full-generate-key
gpg --list-secret-keys --keyid-format=long
```

执行指令的时候会有一些操作, 反正一切跟随默认原则即可. 在执行完以下两条指令后, 应该会看到类似的输出:

```plaintext
------------------------------------
sec   4096R/3AA5C34371567BD2 2016-03-10 [expires: 2017-03-10]
uid                          Hubot <hubot@example.com>
ssb   4096R/4BB6D45482678BE3 2016-03-10
```

这时, 根据想要的 key 生成密钥, 例如上述例子 (来自前面那个官方例子), 如果选择第一个, 那就是:

```shell
gpg --armor --export 3AA5C34371567BD2
```

此时, 复制以 -----BEGIN PGP PUBLIC KEY BLOCK----- 开头并以 -----END PGP PUBLIC KEY BLOCK----- 结尾的 GPG 密钥. 将 GPG 密钥新增到 GitHub 帐户, 即可.

当然, github 的教程是到此为止了, 但是本地的配置其实还不够. 为了让我们在 git commit 时自动跳出窗口, 并且强制输入密码采用 gpg 签名, 我们还需要一些额外的设置.

```shell
# 强制每次 commit 都要 gpg 签名
git config --global commit.gpgsign true
# 修改 ~/.zshrc 或者 ~/.bashrc
# 这是为了让 gpg 在 git commit 的时候弹出窗口
# 否则会报错, 只能通过命令行签名.
export GPG_TTY=$(tty)
```

## Small summary

一切按照官方教程, 基本不会出错. 遇事不决请选择 default. 前人的试错经验是非常珍贵的, 再次感谢 Conless, Wankupi 等同学给予的莫大的帮助!
