#!/usr/bin/env bash

set -e

wget https://github.com/NoSavedDATA/Neve/releases/download/neve-bin/neve
wget https://github.com/NoSavedDATA/nsm/releases/download/latest/nsm
wget https://github.com/NoSavedDATA/Neve/releases/download/neve-bin/sys.tar.bz2

PREFIX="$HOME/.local/neve"
USER_HOME="$HOME"


mkdir -p "$PREFIX/bin"
mkdir -p "$PREFIX/lib"
mkdir -p "$PREFIX/sys_lib"

mv ./neve "$PREFIX/bin"
mv ./nsm "$PREFIX/bin"

chmod +x "$PREFIX/bin/neve"
chmod +x "$PREFIX/bin/nsm"

tar -xjf sys.tar.bz2 -C "$PREFIX"
rm sys.tar.bz2


# Set up bashrc
BASHRC="$USER_HOME/.bashrc"


# add neve binary
if ! grep -q 'export PATH=.*/neve/bin' "$BASHRC"; then
    echo "export PATH=\"$PREFIX/bin:\$PATH\"" >> "$BASHRC"
fi
# track neve .so libs
if ! grep -q 'export NEVE_LIBS=.*/lib' "$BASHRC"; then
    echo "export NEVE_LIBS=$PREFIX/lib" >> "$BASHRC"
fi

echo "✅ neve installed"
echo "run ~/.bashrc"
