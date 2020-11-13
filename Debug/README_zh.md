GDB 的 UEFI 调试
=======================
#### 语言
- 简体中文 (当前语言)
- [English](https://github.com/acidanthera/OpenCorePkg/blob/master/Debug/README.md)

这些代码能在虚拟机（例如 VMware Fusion 和QEMU）中提供更简单的 UEFI code 以进行代码调试。
这些代码基于 [Andrei Warkentin](https://github.com/andreiw) 的 [DebugPkg](https://github.com/andreiw/andreiw-wip/tree/master/uefi/DebugPkg) 并做出改进。
在 macOS 中支持规整输出以及漏洞修复。

通常我们会进行以下尝试:

1. 在 DWARF 中包含 EDK II type info 编译 GdbSyms
2. 神奇的在内存中定位 `EFI_SYSTEM_TABLE`
3. 根据 GUID 定位 `EFI_DEBUG_IMAGE_INFO_TABLE`
4. 映射在 GDB 中重新定位的镜像
5. 提供便捷的功能和规整输出

#### 准备源代码

默认情况下， EDK II optimises 产生二进制文件, 所以当我们要编译一个 "真正的" 调试二进制文件我们需要把目标放在
`NOOPT` 上。 需要注意的是二进制文件的大小将会对它产生严重的影响:

```
build -a X64 -t XCODE5   -b NOOPT -p OpenCorePkg/OpenCorePkg.dsc # for macOS
build -a X64 -t CLANGPDB -b NOOPT -p OpenCorePkg/OpenCorePkg.dsc # for other systems
```

`GdbSyms.dll` 是 OpenCorePkg 中的一部分，但是预编译的二进制文件也可以这样获得:

- `GdbSyms/Bin/X64_XCODE5/GdbSyms.dll` 使用 XCODE5 编译

要在启动时等待调试器连接，可以从 `OcMiscLib.h` 中获取 `WaitForKeyPress` 函数。请注意，此函数还会调用 `DebugBreak` 函数，这可能会在GDB初始化期间被破坏。

#### VMware 配置

VMware Fusion 包含专用的 debugStub，可以通过添加以下内容来启用它行到 .vmx 文件。vmware-vmx将侦听TCP端口8832和8864（在主机上）分别用于32位和64位gdb连接，类似于QEMU：

```
debugStub.listen.guest32 = "TRUE"
debugStub.listen.guest64 = "TRUE"
```

如果调试是远程进行的，则应添加以下行：
```
debugStub.listen.guest32.remote = "TRUE"
debugStub.listen.guest64.remote = "TRUE"
```

要在执行第一条指令时停止虚拟机，请添加以下行代码。
请注意，该代码似乎无法在 VMware Fusion 11 上运行，会导致卡死：
```
monitor.debugOnStartGuest32 = "TRUE"
```

要强制使用硬件断点（而不是软件 INT 3 断点），请添加以下行：
```
debugStub.hideBreakpoints = "TRUE"
```

要在 POST 中停顿 3 秒钟，请添加以下行。
按任意键将启动固件设置：
```
bios.bootDelay = "3000"
```

#### QEMU 配置

除了 VMware 我们也可以使用 [QEMU](https://www.qemu.org)。
通常情况下，在 macOS 主机上进行 QEMU 调试非常有限且缓慢，但是对于不需要进行 macOS 客人引导的常规故障排除上来说这已经足够了。

1. 在 NOOPT 模式下构建 OVMF 固件，以便对其进行调试：

    ```
    git submodule init
    git submodule update     # to clone OpenSSL
    build -a X64 -t XCODE5   -b NOOPT -p OvmfPkg/OvmfPkgX64.dsc # for macOS
    build -a X64 -t CLANGPDB -b NOOPT -p OvmfPkg/OvmfPkgX64.dsc # for other systems
    ```

    要使用 SMM 支持构建 OVMF，请添加 `-D SMM_REQUIRE = 1` 。 使用串行调试构建OVMF 添加 `-D DEBUG_ON_SERIAL_PORT=1`。

1. 照常准备 OpenCore 启动目录。例如，创建一个名为 `QemuRun` 和 `cd`的目录。你的目录结构应该像下面这样：

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

3. 运行 QEMU （`OVMF_BUILD` 应该指向OVMF构建目录, 例如
    `$HOME/UefiWorkspace/Build/OvmfX64/NOOPT_XCODE5/FV`):

    ```
    qemu-system-x86_64 -L . -bios "$OVMF_BUILD/OVMF.fd" -hda fat:rw:ESP \
      -machine q35 -m 2048 -cpu Penryn -smp 4,cores=2 -usb -device usb-mouse -gdb tcp::8864
    ```

    要在 SMM 支持下运行 QEMU，请使用以下命令：
    ```
    qemu-system-x86_64 -L . -global driver=cfi.pflash01,property=secure,value=on \
      -drive if=pflash,format=raw,unit=0,file="$OVMF_BUILD"/OVMF_CODE.fd,readonly=on \
      -drive if=pflash,format=raw,unit=1,file="$OVMF_BUILD"/OVMF_VARS.fd -hda fat:rw:ESP \
      -global ICH9-LPC.disable_s3=1 -machine q35,smm=on,accel=tcg -m 2048 -cpu Penryn \
      -smp 4,cores=2 -usb -device usb-mouse -gdb tcp::8864
    ```

    您还可以添加 `-S` 指令使 QEMU 在第一条指令处停止并等待 GDB 连接。要使用串行调试，请添加 `-serial stdio`。
    
#### 调试器配置

为简单起见，使用 `efidebug.tool` 执行所有必需的 GDB 或 LLDB 脚本。
需要注意，在任何新的二进制加载之后，您需要再运行 `reload-uefi`。

请检查 `efidebug.tool` 标头中的环境变量以配置您的设置。

例如，您可以使用 `EFI_DEBUGGER` 变量来强制 LLDB（`LLDB`）或 GDB（`GDB`）。

#### GDB 配置

如果你计划使用不同的调试体结构进行调试，则最好使用 GDB Multiarch。
这可以通过几种方式实现：

- https://www.gnu.org/software/gdb/ — 资料源
- https://macports.org/ — 通过 MacPorts (`sudo port install gdb +multiarch`)
- 你自己喜欢的方式

一旦安装了 GDB，就可以使用 `efidebug.tool` 进行调试。如果您不想使用 `efidebug.tool`，可以参考以下命令：

```
$ ggdb /opt/UDK/Build/OpenCorePkg/NOOPT_XCODE5/X64/OpenCorePkg/Debug/GdbSyms/GdbSyms/DEBUG/GdbSyms.dll.dSYM/Contents/Resources/DWARF/GdbSyms.dll

target remote localhost:8864
source /opt/UDK/OpenCorePkg/Debug/Scripts/gdb_uefi.py
set pagination off
reload-uefi
b DebugBreak
```

#### CLANGDWARF

CLANGDWARF 工具链是基于 LLVM 的工具链，可直接生成具有 LWA 链接器的 DWARF 调试信息的 PE / COFF 镜像。 LLVM 9.0 或需要使用 LLD 中工作分离无效代码的更新版本才能工作。
([LLD patches](https://bugs.llvm.org/show_bug.cgi?id=45273)).

*安装*: 在将 `ClangDwarf.patch` 破解应用到 EDK II 之后， `CLANGPDB`
工具链将会像 `CLANGDWARF` 一样工作。

为了调试支持，可能需要设置 `EFI_SYMBOL_PATH` 环境变量以 `:` 分隔带有 `.debug` 后缀的一系列文件路径，
例如：

```
export EFI_SYMBOL_PATH="$WORKSPACE/Build/OvmfX64/NOOPT_CLANGPDB/X64:$WORKSPACE/Build/OpenCorePkg/NOOPT_CLANGPDB/X64"
```

以上要求是因为 `--add-gnudebug-link` 选项的不稳定性
[implementation in llvm-objcopy](https://bugs.llvm.org/show_bug.cgi?id=45277).

它仅从调试文件中剥离路径，保留文件名，且不会更新 DataDirectory 调试条目。

#### 参考文献

1. https://communities.vmware.com/thread/390128
1. https://wiki.osdev.org/VMware
1. https://github.com/andreiw/andreiw-wip/tree/master/uefi/DebugPkg
