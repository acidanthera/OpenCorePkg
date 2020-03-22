#!/bin/bash

RUNDIR=$(dirname "$0")
pushd "$RUNDIR" >/dev/null
RUNDIR=$(pwd)
popd >/dev/null

cd "$RUNDIR"

if [ "$LLDB" = "" ]; then
  LLDB=$(ls /opt/local/bin/lldb-mp* 2>/dev/null)
fi

if [ "$LLDB" = "" ]; then
  LLDB=$(which lldb)
fi

if [ "$LLDB" = "" ]; then
  echo "Failed to find LLDB"
  exit 1
fi

if [ "$LLDB_PORT" = "" ]; then
  LLDB_PORT=8864
fi

"$LLDB" -o "settings set plugin.process.gdb-remote.target-definition-file Scripts/x86_64_target_definition.py" \
  -o "gdb-remote $LLDB_PORT" \
  -o "target modules add GdbSyms/Bin/X64_CLANGDWARF/GdbSyms.debug" \
  -o "command script import Scripts/lldb_uefi.py" \
  -o "command script add -c lldb_uefi.ReloadUefi reload-uefi" \
  -o "reload-uefi" \
  -o "b DebugBreak"
