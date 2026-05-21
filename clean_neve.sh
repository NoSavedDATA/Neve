#!/usr/bin/env bash

set -e



if [ -n "$SUDO_USER" ]; then
    PREFIX="/usr/bin/neve"
    USER_HOME=$(getent passwd "$SUDO_USER" | cut -d: -f6)
else
    PREFIX="$HOME/.local/neve"
    USER_HOME="$HOME"
fi

rm -r "$PREFIX"


BASHRC="$USER_HOME/.bashrc"

# remove neve PATH entry
sed -i "\|^export PATH=\"$PREFIX/bin:\$PATH\"$|d" "$BASHRC"
# remove NEVE_LIBS export
sed -i "\|^export NEVE_LIBS=$PREFIX/lib$|d" "$BASHRC"

echo "✅ neve environment entries removed"
echo "ℹ️  Run: source ~/.bashrc (or open a new shell)"
