#!/bin/bash
cd "$GTASA_DIR" || exit

wineserver -k
killall -9 gta_sa.exe gdbserver.exe 2>/dev/null

export WINEDEBUG=-all,err+dbghelp
GDB_PATH="/usr/share/win32/gdbserver.exe"

wine "$GDB_PATH" :12345 gta_sa.exe