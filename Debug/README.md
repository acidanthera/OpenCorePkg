UEFI Debugging with GDB
=======================

These scripts provide support for easier UEFI code debugging on virtual machines like VMware Fusion
or QEMU. The code is based on [Andrei Warkentin](https://github.com/andreiw)'s
[DebugPkg](https://github.com/andreiw/andreiw-wip/tree/master/uefi/DebugPkg) with improvements
in macOS support, pretty printing, and bug fixing.

The general approach is as follows:

1. Build GdbSyms binary with EDK II type info in DWARF
1. Locate `EFI_SYSTEM_TABLE` in memory by its magic
1. Locate `EFI_DEBUG_IMAGE_INFO_TABLE` by its GUID
1. Map relocated images within GDB
1. Provide handy functions and pretty printers

#### Preparing Source Code

By default EDK II optimises produced binaries, so to build a "real" debug binary one should target
`NOOPT`. Do be aware that it strongly affects resulting binary size:

```
build -a X64 -t XCODE5   -b NOOPT -p OpenCorePkg/OpenCorePkg.dsc # for macOS
build -a X64 -t CLANGPDB -b NOOPT -p OpenCorePkg/OpenCorePkg.dsc # for other systems
```

`GdbSyms.dll` is built as a part of OpenCorePkg, yet prebuilt binaries are also available:

- `GdbSyms/Bin/X64_XCODE5/GdbSyms.dll` is built with XCODE5

To wait for debugger connection on startup `WaitForKeyPress` functions from `OcMiscLib.h` can be
utilised. Do be aware that this function additionally calls `DebugBreak` function, which may
be broken at during GDB init.

#### VMware Configuration

VMware Fusion contains a dedicated debugStub, which can be enabled by adding the following
lines to .vmx file. Afterwards vmware-vmx will listen on TCP ports 8832 and 8864 (on the host)
for 32-bit and 64-bit gdb connections respectively, similarly to QEMU:
```
debugStub.listen.guest32 = "TRUE"
debugStub.listen.guest64 = "TRUE"
```

In case the debugging session is remote the following lines should be appended:
```
debugStub.listen.guest32.remote = "TRUE"
debugStub.listen.guest64.remote = "TRUE"
```

To halt the virtual machine upon executing the first instruction the following line code be added.
Note, that it does not seem to work on VMware Fusion 11 and results in freezes:
```
monitor.debugOnStartGuest32 = "TRUE"
```

To force hardware breakpoints (instead of software INT 3 breakpoints) add the following line:
```
debugStub.hideBreakpoints = "TRUE"
```

To stall during POST for 3 seconds add the following line. Pressing any key will boot into firmware
settings:
```
bios.bootDelay = "3000"
```

#### QEMU configuration

In addition to VMware it is also possible to use [QEMU](https://www.qemu.org). QEMU debugging
on macOS host is generally rather limited and slow, but it is enough for generic troubleshooting
when no macOS guest booting is required.

1. Build OVMF firmware in NOOPT mode to be able to debug it:

    ```
    git submodule init
    git submodule update     # to clone OpenSSL
    build -a X64 -t XCODE5   -b NOOPT -p OvmfPkg/OvmfPkgX64.dsc # for macOS
    build -a X64 -t CLANGPDB -b NOOPT -p OvmfPkg/OvmfPkgX64.dsc # for other systems
    ```

    To build OVMF with SMM support add `-D SMM_REQUIRE=1`. To build OVMF with serial debugging
    add `-D DEBUG_ON_SERIAL_PORT=1`.

2. Prepare launch directory with OpenCore as usual. For example, make a directory named
    `QemuRun` and `cd` to it. You should have a similar directory structure:

    ```
    .
    └── ESP
        └── EFI
            ├── BOOT
            │   └── BOOTx64.efi
            └── OC
                ├── OpenCore.efi
                └── config.plist
    ```

3. Run QEMU (`OVMF_BUILD` should point to OVMF build directory, e.g.
    `$HOME/UefiWorkspace/Build/OvmfX64/NOOPT_XCODE5/FV`):

    ```
    qemu-system-x86_64 -L . -bios "$OVMF_BUILD/OVMF.fd" -hda fat:rw:ESP \
      -machine q35 -m 2048 -cpu Penryn -smp 4,cores=2 -usb -device usb-mouse -gdb tcp::8864
    ```

    To run QEMU with SMM support use the following command:
    ```
    qemu-system-x86_64 -L . -global driver=cfi.pflash01,property=secure,value=on \
      -drive if=pflash,format=raw,unit=0,file="$OVMF_BUILD"/OVMF_CODE.fd,readonly=on \
      -drive if=pflash,format=raw,unit=1,file="$OVMF_BUILD"/OVMF_VARS.fd -hda fat:rw:ESP \
      -global ICH9-LPC.disable_s3=1 -machine q35,smm=on,accel=tcg -m 2048 -cpu Penryn \
      -smp 4,cores=2 -usb -device usb-mouse -gdb tcp::8864
    ```

    You may additionally pass `-S` flag to QEMU to stop at first instruction
    and wait for GDB connection. To use serial debugging add `-serial stdio`.

#### Debugger Configuration

For simplicitly `efidebug.tool` performs all the necessary GDB or LLDB scripting.
Note, that you need to run `reload-uefi` after any new binary loads.

Check `efidebug.tool` header for environment variables to configure your setup.
For example, you can use `EFI_DEBUGGER` variable to force LLDB (`LLDB`) or GDB (`GDB`).

#### GDB Configuration

It is a good idea to use GDB Multiarch in case different debugging architectures are planned to be
used. This can be done in several ways:

- https://www.gnu.org/software/gdb/ — from source
- https://macports.org/ — via MacPorts (`sudo port install gdb +multiarch`)
- Your preferred method here

Once GDB is installed you can use `efidebug.tool` for debugging. In case you do not
want to use `efidebug.tool`, the following set of commands can be used as a reference:

```
$ ggdb /opt/UDK/Build/OpenCorePkg/NOOPT_XCODE5/X64/OpenCorePkg/Debug/GdbSyms/GdbSyms/DEBUG/GdbSyms.dll.dSYM/Contents/Resources/DWARF/GdbSyms.dll

target remote localhost:8864
source /opt/UDK/OpenCorePkg/Debug/Scripts/gdb_uefi.py
set pagination off
reload-uefi
b DebugBreak
```

#### CLANGDWARF

CLANGDWARF toolchain is an LLVM-based toolchain that directly generates
PE/COFF images with DWARF debug information via LLD linker. LLVM 9.0 or
newer with working dead code stripping in LLD is required for this to work
([LLD patches](https://bugs.llvm.org/show_bug.cgi?id=45273)).

*Installation*: After applying `ClangDwarf.patch` hack onto EDK II `CLANGPDB`
toolchain will behave as if it was `CLANGDWARF`.

For debugging support it may be necessary to set `EFI_SYMBOL_PATH`
environment variable to `:`-separated list of paths with `.debug` files,
for example:

```
export EFI_SYMBOL_PATH="$WORKSPACE/Build/OvmfX64/NOOPT_CLANGPDB/X64:$WORKSPACE/Build/OpenCorePkg/NOOPT_CLANGPDB/X64"
```

The reason for this requirement is fragile `--add-gnudebug-link` option
[implementation in llvm-objcopy](https://bugs.llvm.org/show_bug.cgi?id=45277).
It strips path from the debug file preserving only filename and also does not
update DataDirectory debug entry.

#### References

1. https://communities.vmware.com/thread/390128
1. https://wiki.osdev.org/VMware
1. https://github.com/andreiw/andreiw-wip/tree/master/uefi/DebugPkg
