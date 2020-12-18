[English](/README.md)｜[简体中文](/README_zh-cn.md)

<img src="https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/Logos/OpenCore_with_text_Small.png" width="200" height="48"/>

[![Build Status](https://github.com/acidanthera/OpenCorePkg/workflows/CI/badge.svg?branch=master)](https://github.com/acidanthera/OpenCorePkg/actions) [![Scan Status](https://scan.coverity.com/projects/18169/badge.svg?flat=1)](https://scan.coverity.com/projects/18169)
-----

包含开发SDK的OpenCore引导加载程序。

## 论坛

- [AppleLife.ru](https://applelife.ru/threads/razrabotka-opencore.2943955) in Russian
- [Hackintosh-Forum.de](https://www.hackintosh-forum.de/forum/thread/42353-opencore-bootloader) in German
- [InsanelyMac.com](https://www.insanelymac.com/forum/topic/338527-opencore-development/) in English
- [MacRumors.com](https://forums.macrumors.com/threads/opencore-on-the-mac-pro.2207814/) in English, legacy Apple hardware
- [KVM-OpenCore](https://github.com/Leoyzen/KVM-Opencore) in English, KVM configuration
- [macOS86.it](https://www.macos86.it/showthread.php?4570-OpenCore-aka-OC-Nuovo-BootLoader) in Italian
- [PCbeta.com](http://bbs.pcbeta.com/viewthread-1815623-1-1.html) in Chinese

## 图书馆

此存储库还包含其他 UEFI 支持公共库，这些公共库由 [Acidanthera](https://github.com/acidanthera)的其它项目共享. 库集的主要目的是为特定于 Apple 的 UEFI 驱动程序提供补充功能。主要特点:

- 苹果磁盘映像加载支持
- 苹果键盘输入聚合
- 苹果PE图像签名验证
- 苹果 UEFI 安全启动补充代码
- 支持屏幕读取的音频管理
- 基本 ACPI 和 SMBIOS 操作
- 支持计时器的 CPU 信息收集
- 加密基元（SHA-256、RSA 等）
- 解压缩基元（zlib、lzss、lzvn 等）
- ACPI 读取和修改的帮助程序代码
- 文件、字符串、UEFI 变量的更高级别的抽象
- 溢出检查分析
- PE 映像加载，无 UEFI 安全启动冲突
- 列表配置格式分析
- PNG 图像操作
- 文本输出和图形输出实现
- XNU 内核驱动程序注入和修补程序引擎

代码库的早期历史可以在 [AppleSupportPkg](https://github.com/acidanthera/AppleSupportPkg)和 PicoLib 库中找到.

#### OcGuardLib

此库实现建议在项目中使用的基本安全功能。它基于[NetBSD implementation](https://blog.netbsd.org/tnf/entry/introduction_to_µubsan_a_clean)实现，在编译器内置、类型对齐检查和 UBSan 运行时上实现快速安全的整体参数映射。

使用 UBSan 运行时需要使用 Clang 编译器和参数。有关详细信息 [Clang 文档](https://releases.llvm.org/7.0.0/tools/clang/docs/UndefinedBehaviorSanitizer.html) `-fsanitize=undefined` 

#### Credits

- The HermitCrabs Lab
- All projects providing third-party code (refer to file headers)
- [AppleLife](https://applelife.ru) team and user-contributed resources
- Chameleon and Clover teams for hints and legacy
- [al3xtjames](https://github.com/al3xtjames)
- [Andrey1970AppleLife](https://github.com/Andrey1970AppleLife)
- [Download-Fritz](https://github.com/Download-Fritz)
- [Goldfish64](https://github.com/Goldfish64)
- [nms42](https://github.com/nms42)
- [PMheart](https://github.com/PMheart)
- [savvamitrofanov](https://github.com/savvamitrofanov)
- [usr-sse2](https://github.com/usr-sse2)
- [vit9696](https://github.com/vit9696)
