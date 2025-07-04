---
title: My environment config v2.0
date: 2025-06-23 16:38:25
updated:
tags: [基础知识]
categories: [计算机, 工具]
keywords: [cuda, gpg, Python]
cover: https://s2.loli.net/2025/06/27/MW5js87vLXJgklo.png
mathjax: false
description: I hate configuring environment.
---

Recently, I frequently setup my environment on different cuda machines. Therefore, I keep a record of my environment config here. After this, we will have:

1. Customized zsh
2. C++ & cuda develop environement (with clangd)
3. Python develop environement (uv + pip)

First, we need to start from a docker image. I like the image from nvidia. It's clean.

```bash
# In a remote machine
docker pull nvidia/cuda:12.8.0-devel-ubuntu22.04
docker run -itd --shm-size 32g --gpus all --ipc=host --network=host --privileged --name cuda_dark nvidia/cuda:12.8.0-devel-ubuntu22.04 /usr/bin/bash
docker exec -it cuda_dark /usr/bin/bash
```

In docker, we need to update and change to first.

```bash
# Now in docker
apt update
apt install zsh tmux git ccache ninja-build cmake curl wget vim python3 pip lsb-release software-properties-common gnupg -y
chsh $(whoami) -s $(which zsh)
# for ubuntu 24.04 image, you may need:
apt install python3.12-venv
```

To use VSCode tunnel, we will need to download VScode and run `./code tunnel` in a seperate terminal.

```bash
# Now in zsh
zsh
mkdir vscode
cd vscode
curl -Lk 'https://code.visualstudio.com/sha/download?build=stable&os=cli-alpine-x64' --output vscode_cli.tar.gz
tar -xf vscode_cli.tar.gz
# Now in tmux, we set up VSCode tunnel :)
tmux
./code tunnel
# Ctrl + B and then D, we leave tmux
```

This is my customized zsh.

```bash
sh -c "$(curl -fsSL https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh)"
git clone https://github.com/zsh-users/zsh-autosuggestions ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-autosuggestions
git clone https://gitee.com/Annihilater/zsh-syntax-highlighting.git ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-syntax-highlighting
git clone https://github.com/romkatv/powerlevel10k.git $ZSH_CUSTOM/themes/powerlevel10k

# We now open zshrc in VSCode
code ~/.zshrc

# find and set as following:
# ZSH_THEME="powerlevel10k/powerlevel10k"
# plugins=(
#  git
#  gitfast
#  zsh-autosuggestions
#  zsh-syntax-highlighting
#  z
#  uv
#)
```

My python environment.

```bash

# only on ubuntu 24.04, we can't install in system pip
python3 -m venv .venv
source .venv/bin/activate

# Now we need to restart zsh (or source ~/.zshrc)
# we need to configure p10k...
# after that, configure python
pip install uv

# then add the following in ~/.zshrc
# alias pip="uv pip"
# alias pip3="uv pip3"

# When creating a venv
uv venv
source .venv/bin/activate
uv pip install uv pip

# Some other things i may need
pip install nvitop
```

My C++ environment (no need to set up things like `--query-driver` in docker).

```bash
# we need to set up clangd for C++ development
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
./llvm.sh 19
rm llvm.sh
update-alternatives --install /usr/bin/clangd llvm /usr/bin/clangd-19 100
```
