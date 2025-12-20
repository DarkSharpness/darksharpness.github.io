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

## Docker

First, we need to start from a docker image. I like the image from nvidia. It's clean.

```bash
# In a remote machine
docker pull nvidia/cuda:12.8.0-devel-ubuntu22.04
docker run -itd --shm-size 32g --gpus all --ipc=host --network=host --privileged --name cuda_dark nvidia/cuda:12.8.0-devel-ubuntu22.04 /usr/bin/bash
docker exec -it cuda_dark /usr/bin/bash
# my own cuda docker image
docker run -itd --shm-size 32g --gpus all --ipc=host --pid=host --network=host --privileged --name cuda_dark darksharpness/cuda_12_9 zsh
```

In docker, we need to update and change to first.

```bash
# Now in docker
apt update
apt install zsh tmux git ccache ninja-build cmake curl wget vim python3 pip lsb-release software-properties-common gnupg -y
chsh $(whoami) -s $(which zsh)
# for ubuntu 24.04 image, you may need:
apt install python3.12-venv
# for sglang development
apt install libnuma-dev libnuma1
# force to update system pip
/usr/bin/pip install pip uv nvitop --break-system-packages
```

## VSCode

To use VSCode tunnel, we will need to download VScode and run `./code tunnel` in a seperate terminal.

```bash
# Now in zsh
zsh
mkdir vscode
cd vscode
curl -Lk 'https://code.visualstudio.com/sha/download?build=stable&os=cli-alpine-x64' --output vscode_cli.tar.gz
tar -xf vscode_cli.tar.gz

# Before tmux, we may set up ~/.tmux.conf
set -g mouse on

# Now in tmux, we set up VSCode tunnel :)
tmux
./code tunnel
```

## Shell config

This is my customized zsh.

```bash
sh -c "$(curl -fsSL https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh)"
git clone https://github.com/zsh-users/zsh-autosuggestions ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-autosuggestions
git clone https://gitee.com/Annihilater/zsh-syntax-highlighting.git ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-syntax-highlighting
git clone https://github.com/romkatv/powerlevel10k.git $ZSH_CUSTOM/themes/powerlevel10k

# We now open zshrc in VSCode
code ~/.zshrc
```

Then set zshrc as following (you need to override `ZSH_THEME` and `plugins`):

``` bash
ZSH_THEME="powerlevel10k/powerlevel10k"
plugins=(
  git
  gitfast
  zsh-autosuggestions
  zsh-syntax-highlighting
  z
  uv
)

# For a colorful terminal, you might need:
export TERM="xterm-256color"

# Bind ctrl + backspace to kill one word
bindkey '^\b' vi-backward-kill-word
```

My vimrc (stolen from [hnyls2002](https://github.com/hnyls2002/setup/blob/master/.vimrc)).

```bash
set cindent
set number
set autoindent
set smartindent
set tabstop=4
set softtabstop=4
set shiftwidth=4
set backspace=2
set mouse=a
set cursorline
set cursorcolumn
set clipboard=unnamed
set hlsearch
set wrap

" Make the search case insensitive
set ignorecase
" But make them case-sensitive if they include capitals
set smartcase

map <C-h> 5h
map <C-j> 5j
map <C-k> 5k
map <C-l> 5l
imap jk <Esc>

map <C-a> ggVG
imap <C-a> ggVG

map <F3> : w<CR>:!python3 %<CR>

let g:powerline_loaded=1

" Use Ctrl-H to delete the previous word in insert mode
inoremap <C-H> <C-W>
```

My tmux.conf (actually, [zellij](#zellij) can be a better tmux):

```bash
set -g mouse on
```

## Python env

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

## C++ env

My C++ environment (no need to set up things like `--query-driver` in docker).

```bash
# we need to set up clangd for C++ development
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
./llvm.sh 21 all # add all if u just want all llvm packages
rm llvm.sh
update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-21 100 \
    --slave /usr/bin/clang-format clang-format /usr/bin/clang-format-21
```

## zellij

Optional: A modern shell (zellij) configuration (no conflict with zsh):

```bash
curl -LO https://github.com/zellij-org/zellij/releases/latest/download/zellij-x86_64-unknown-linux-musl.tar.gz
tar -xzf zellij-x86_64-unknown-linux-musl.tar.gz
mkdir -p "$HOME/.local/bin/"
mv zellij "$HOME/.local/bin/zellij"
```

Then write the following to your zshrc (not execute it in terminal)

```bash
if [ -z $SHELL ]; then
  export SHELL=$(which zsh)
fi

# export ZELLIJ_AUTO_ATTACH="true"
export ZELLIJ_AUTO_EXIT="true"

alias zellij="$HOME/.local/bin/zellij"

# don't auto-start zellij if we're in VSCode's integrated terminal or inside tmux
if [ -z "$VSCODE_INJECTION" ] && [ -z "$TMUX" ]; then
    eval "$(zellij setup --generate-auto-start zsh)"
fi
```
