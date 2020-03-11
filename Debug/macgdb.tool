#!/bin/bash

RUNDIR=$(dirname "$0")
pushd "$RUNDIR" >/dev/null
RUNDIR=$(pwd)
popd >/dev/null

cd "$RUNDIR"

if [ "$GDB" = "" ]; then
  GDB=$(which ggdb)
fi

if [ "$GDB" = "" ]; then
  GDB=$(which gdb-multiarch)
fi

if [ "$GDB" = "" ]; then
  GDB=$(which gdb)
fi

if [ "$GDB" = "" ]; then
  echo "Failed to find GDB"
  exit 1
fi

"$GDB" -ex "target remote localhost:8864" \
       -ex "source Scripts/gdb_uefi.py" \
       -ex "set pagination off" \
       -ex "reload-uefi" \
       -ex "b DebugBreak" \
       GdbSyms/Bin/X64_XCODE5/GdbSyms.dll
