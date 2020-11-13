<img src="https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/Logos/OpenCore_with_text_Small.png" width="200" height="48"/>

[![Build Status](https://travis-ci.com/acidanthera/OpenCorePkg.svg?branch=master)](https://travis-ci.com/acidanthera/OpenCorePkg) [![Scan Status](https://scan.coverity.com/projects/18169/badge.svg?flat=1)](https://scan.coverity.com/projects/18169)
-----

OpenCore bootloader 以及开发 SDK.

## 语言
- 简体中文 (当前语言)
- [English](README.md)

## 论坛

- [AppleLife.ru](https://applelife.ru/threads/razrabotka-opencore.2943955) - 俄语
- [Hackintosh-Forum.de](https://www.hackintosh-forum.de/forum/thread/42353-opencore-bootloader) - 德语
- [InsanelyMac.com](https://www.insanelymac.com/forum/topic/338527-opencore-development/) - 英语
- [MacRumors.com](https://forums.macrumors.com/threads/opencore-on-the-mac-pro.2207814/) - 英语，旧款苹果设备
- [KVM-OpenCore](https://github.com/Leoyzen/KVM-Opencore) - 英语, KVM 配置
- [macOS86.it](https://www.macos86.it/showthread.php?4570-OpenCore-aka-OC-Nuovo-BootLoader) - 意大利语
- [PCbeta.com](http://bbs.pcbeta.com/viewthread-1815623-1-1.html) - 中文

## 库

本源也包括了在 [Acidanthera](https://github.com/acidanthera) 项目中额外的 UEFI 支持库。库列表的主要目的是给系统提供 Apple-specific UEFI 驱动的功能。 主要的功能:

- Apple 镜像挂载
- 集成 Apple 键盘输入
- Apple PE 镜像签名认证
- Apple UEFI 安全启动追加代码
- 屏幕阅读的音频支持
- 基础 ACPI 以及 SMBIOS 控制
- 支持 timer 的 CPU 信息收集
- 原始加密的支持 (SHA-256, RSA, 等)
- 原始解密的支持 (zlib, lzss, lzvn, 等)
- ACPI 帮助代码的可读和修改
- 文件，字符串，UEFI 变量的高级抽象化
- Overflow 校验算法
- 挂载 PE 镜像并避免 UEFI 安全启动冲突
- Plist 配置文件格式校验
- PNG 图片修改
- 文字以及图像输出
- 注入 XNU 核心以及修补引擎

codebase 之前的历史可以在 [AppleSupportPkg](https://github.com/acidanthera/AppleSupportPkg) 以及 PicoLib library set by The HermitCrabs Lab 找到。

#### OcGuardLib

这个库提供了本项目建议使用的基本的安全功能。
基于 [NetBSD implementation](https://blog.netbsd.org/tnf/entry/introduction_to_µubsan_a_clean)， 它能够在编译器内置命令中提供快捷安全的积分算法映射，输入对齐校验以及 UBSan runtime。

UBSan runtime 需要使用 Clang compiler 以及 `-fsanitize=undefined` 参数。 请参考
[Clang documentation](https://releases.llvm.org/7.0.0/tools/clang/docs/UndefinedBehaviorSanitizer.html) 以获得更详细信息。
#### 感谢

- The HermitCrabs Lab
- 所有提供了代码的第三方程序 (参考文件的头部)
- [AppleLife](https://applelife.ru) 团队以及用户提供的源
- Chameleon 和 Clover 团代带来的启发和此前的工作
- [al3xtjames](https://github.com/al3xtjames)
- [Andrey1970AppleLife](https://github.com/Andrey1970AppleLife)
- [Download-Fritz](https://github.com/Download-Fritz)
- [Goldfish64](https://github.com/Goldfish64)
- [nms42](https://github.com/nms42)
- [PMheart](https://github.com/PMheart)
- [savvamitrofanov](https://github.com/savvamitrofanov)
- [usr-sse2](https://github.com/usr-sse2)
- [vit9696](https://github.com/vit9696)
