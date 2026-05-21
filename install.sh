#!/usr/bin/env bash

set -e

wget https://github.com/NoSavedDATA/Neve/releases/download/neve-bin/neve
wget https://github.com/NoSavedDATA/nsm/releases/download/latest/nsm
wget https://github.com/NoSavedDATA/Neve/releases/download/neve-bin/sys.tar.bz2

PREFIX="$HOME/.local/neve"
USER_HOME="$HOME"

BIN_DIR="$HOME/.local/bin"
mkdir -p "$BIN_DIR"

mkdir -p "$PREFIX/bin"
mkdir -p "$PREFIX/lib"
mkdir -p "$PREFIX/sys_lib"

mv ./neve "$PREFIX/bin"
mv ./nsm "$PREFIX/bin"

chmod +x "$PREFIX/bin/neve"
chmod +x "$PREFIX/bin/nsm"

tar -xjf sys.tar.bz2 -C "$PREFIX"
rm sys.tar.bz2



cat > "$BIN_DIR/neve" <<EOF
#!/usr/bin/env bash
export NEVE_LIBS="$PREFIX/lib"
exec "$PREFIX/bin/neve" "\$@"
EOF

chmod +x "$BIN_DIR/neve"




echo "✅ neve installed"
