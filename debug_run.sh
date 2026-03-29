#!/bin/bash
# Ensure the directory is correct
cd "$GTASA_DIR" || exit

# 1. Kill any hung sessions
wineserver -k
killall -9 gta_sa.exe gdbserver.exe 2>/dev/null

# 2. Setup environment
export WINEDEBUG=-all,err+dbghelp

# 3. Use the 32-bit gdbserver (x86) for GTA SA
# Note: Use ":" before the port to bind to all interfaces
# Note: Map the path to a Wine-friendly format or stay in the CWD
GDB_PATH="/usr/share/win32/gdbserver.exe"

wine "$GDB_PATH" :12345 gta_sa.exe