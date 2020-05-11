#!/bin/bash

#
# Configuration variables:
# LLDB          - path to LLDB debugger
#                  defaults to finding in PATH
# GDB           - path to GDB debugger
#                  defaults to finding in PATH
# EFI_ARCH      - architecture to debug
#                  defaults to X64
# EFI_PORT      - debugger TCP connection port
#                  defaults to 8864 for X64 and 8832 for IA32
# EFI_HOST      - debugger TCP connection host
#                  defaults to localhost
# EFI_DEBUGGER  - debugger to use
#                  defaults to LLDB on macOS and GDB otherwise, fallbacks to other when missing
# EFI_TOOLCHAIN - toolchain to use
#                  defaults to XCODE5 on macOS and GCC5 otherwise
# EFI_SYMS      - symbols to use
#                  defaults to files in GdbSyms/Bin
# EFI_SYMS_PDB  - optional PDB symbols for LLDB
#                  defaults to "-s GdbSyms/Bin/${EFI_ARCH}_CLANGPDB/GdbSyms.pdb" for CLANGPDB
# EFI_TRIPLE    - optional target triple for LLDB
#                  defaults to x86_64-apple-macosx for XCODE5, x86_64-pc-windows-msvc for CLANGDWARF/CLANGPDB,
#                  x86_64-linux-gnu otherwise.
#

RUNDIR=$(dirname "$0")
pushd "${RUNDIR}" >/dev/null  || exit 1
RUNDIR=$(pwd)
popd >/dev/null || exit 1

cd "$RUNDIR" || exit 1

find_gdb() {
  if [ "${GDB}" = "" ]; then
    GDB=$(which ggdb)
  fi

  if [ "${GDB}" = "" ]; then
    GDB=$(which gdb-multiarch)
  fi

  if [ "${GDB}" = "" ]; then
    GDB=$(which gdb)
  fi
}

find_lldb() {
  if [ "${LLDB}" = "" ]; then
    LLDB=$(ls /opt/local/bin/lldb-mp* 2>/dev/null)
  fi

  if [ "${LLDB}" = "" ]; then
    LLDB=$(which lldb)
  fi
}

choose_debugger() {
  find_gdb
  find_lldb

  if [ "${EFI_ARCH}" = "" ]; then
    EFI_ARCH="X64"
  fi

  if [ "${EFI_HOST}" = "" ]; then
    EFI_HOST="localhost"
  fi

  if [ "${EFI_PORT}" = "" ]; then
    if [ "${EFI_ARCH}" = "IA32" ]; then
      EFI_PORT=8832
    else
      EFI_PORT=8864
    fi
  fi

  if [ "${EFI_DEBUGGER}" = "" ]; then
    if [ "${LLDB}" != "" ] && [ "$(uname)" = "Darwin" ]; then
      EFI_DEBUGGER="LLDB"
    elif [ "${GDB}" != "" ]; then
      EFI_DEBUGGER="GDB"
    elif [ "${LLDB}" != "" ]; then
      EFI_DEBUGGER="LLDB"
    else
      echo "Cannot find installed GDB or LLDB debugger!"
      echo "Hint: set GDB or LLDB variables to debugger path."
      exit 1
    fi
  fi

  if [ "${EFI_TOOLCHAIN}" = "" ]; then
    if [ "$(uname)" = "Darwin" ]; then
      EFI_TOOLCHAIN="XCODE5"
    else
      EFI_TOOLCHAIN="GCC5"
    fi
  fi

  if [ "${EFI_SYMS}" = "" ]; then
    if [ "${EFI_TOOLCHAIN}" = "XCODE5" ]; then
      EFI_SYMS="GdbSyms/Bin/${EFI_ARCH}_XCODE5/GdbSyms.dll"
    elif [ "${EFI_TOOLCHAIN}" = "CLANGPDB" ]; then
      EFI_SYMS_PDB="-s GdbSyms/Bin/${EFI_ARCH}_CLANGPDB/GdbSyms.pdb"
      EFI_SYMS="GdbSyms/Bin/${EFI_ARCH}_CLANGPDB/GdbSyms.dll"
    else
      EFI_SYMS="GdbSyms/Bin/${EFI_ARCH}_${EFI_TOOLCHAIN}/GdbSyms.debug"
    fi
  fi

  if [ ! -f "${EFI_SYMS}" ]; then
    echo "Cannot find symbols: ${EFI_SYMS}!"
    echo "Hint: compile this file or set EFI_SYMS variable."
    exit 1
  fi

  if [ "${EFI_TRIPLE}" = "" ]; then
    if [ "${EFI_ARCH}" = "IA32" ]; then
      triple_arch=i386
    else
      triple_arch=x86_64
    fi

    if [ "${EFI_TOOLCHAIN}" = "XCODE5" ]; then
      export EFI_TRIPLE="${triple_arch}-apple-macosx"
    elif [ "${EFI_TOOLCHAIN}" = "CLANGPDB" ] || [ "${EFI_TOOLCHAIN}" = "CLANGDWARF" ]; then
      export EFI_TRIPLE="${triple_arch}-pc-windows-msvc"
    else
      export EFI_TRIPLE="${triple_arch}-linux-gnu"
    fi
  fi
}

choose_debugger

if [ "${EFI_DEBUGGER}" = "GDB" ] || [ "${EFI_DEBUGGER}" = "gdb" ]; then
  "${GDB}" -ex "target remote ${EFI_HOST}:${EFI_PORT}" \
    -ex "source Scripts/gdb_uefi.py" \
    -ex "set pagination off" \
    -ex "reload-uefi" \
    -ex "b DebugBreak" \
    "${EFI_SYMS}"
elif [ "${EFI_DEBUGGER}" = "LLDB" ] || [ "${EFI_DEBUGGER}" = "lldb" ]; then
  "$LLDB" -o "settings set plugin.process.gdb-remote.target-definition-file Scripts/x86_64_target_definition.py" \
    -o "gdb-remote ${EFI_HOST}:${EFI_PORT}" \
    -o "target create ${EFI_SYMS_PDB} ${EFI_SYMS}" \
    -o "command script import Scripts/lldb_uefi.py" \
    -o "command script add -c lldb_uefi.ReloadUefi reload-uefi" \
    -o "reload-uefi" \
    -o "b DebugBreak"
else
  echo "Unsupported debugger ${EFI_DEBUGGER}!"
  echo "Hint: use GDB or LLDB in EFI_DEBUGGER variable."
  exit 1
fi
