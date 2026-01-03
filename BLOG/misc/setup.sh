# preliminary
apt update -y
apt install zsh tmux git ccache ninja-build cmake curl wget vim python3 pip lsb-release software-properties-common gnupg -y
apt install libnuma-dev libnuma1 -y
/usr/bin/pip install pip uv nvitop --break-system-packages
echo "Preliminary setup done"

# VSCode
mkdir -p /_vscode
cd /_vscode
curl -Lk 'https://code.visualstudio.com/sha/download?build=stable&os=cli-alpine-x64' --output vscode_cli.tar.gz
tar -xf vscode_cli.tar.gz
echo "VSCode setup done"

# clangd
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
./llvm.sh 21 all # add all if u just want all llvm packages
rm llvm.sh
update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-21 100 \
    --slave /usr/bin/clang-format clang-format /usr/bin/clang-format-21

# zellij
curl -LO https://github.com/zellij-org/zellij/releases/latest/download/zellij-x86_64-unknown-linux-musl.tar.gz
tar -xzf zellij-x86_64-unknown-linux-musl.tar.gz
mkdir -p "$HOME/.local/bin/"
mv zellij "$HOME/.local/bin/zellij"

# update zshrc once
ZSHRC="$HOME/.zshrc"
MARKER_BEGIN="# >>> zellij auto-start (managed) >>>"
MARKER_END="# <<< zellij auto-start (managed) <<<"

BLOCK=$(cat <<'EOF'
# >>> zellij auto-start (managed) >>>
if [ -z "$SHELL" ]; then
  export SHELL="$(command -v zsh)"
fi

# export ZELLIJ_AUTO_ATTACH="true"
export ZELLIJ_AUTO_EXIT="true"

alias zellij="$HOME/.local/bin/zellij"

# don't auto-start zellij if we're in VSCode's integrated terminal or inside tmux
if [ -z "$VSCODE_INJECTION" ] && [ -z "$TMUX" ]; then
    eval "$(zellij setup --generate-auto-start zsh)"
fi
# <<< zellij auto-start (managed) <<<
EOF
)

if ! grep -Fq "$MARKER_BEGIN" "$ZSHRC"; then
  printf '\n%s\n' "$BLOCK" >> "$ZSHRC"
fi
