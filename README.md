<div align=center>
  <img src="/Docs/Logos/Logo.png" width="200" height="200"/>
  <h1>OpenCore Bootloader</h1><p><i>Development SDK included</i></p>
  <img alt="Build Status" src="https://github.com/acidanthera/OpenCorePkg/actions/workflows/build.yml/badge.svg?branch=master">
  <img alt="Scan Status" src="https://scan.coverity.com/projects/18169/badge.svg?flat=1)](https://scan.coverity.com/projects/18169">
</div>

-----

## :inbox_tray: Downloads
All releases of OpenCore can be found in the [releases page](https://github.com/acidanthera/OpenCorePkg/releases).

## :books: Libraries
This repository includes common UEFI support libraries from [Acidanthera](https://github.com/acidanthera), providing extra features for Apple-specific UEFI drivers. Key features include:

- **Apple Software Support**
  - XNU kernel driver injection and patch engine
  - Plist configuration format parsing
  - Apple keyboard input aggregation
  - Apple disk image loading support
  - Apple PE image signature verification
  - Apple UEFI secure boot supplemental code
  - PE image loading with no UEFI Secure Boot conflict

- **UEFI Support**
  - Basic ACPI and SMBIOS manipulation
  - CPU information gathering with timer support
  - Helper code for ACPI reads and modifications
  - Higher level abstractions for files, strings, UEFI variables
  - Overflow checking arithmetics

- **Media Support**
  - Cryptographic primitives (SHA-256, RSA, etc.)
  - Decompression primitives (zlib, lzss, lzvn, etc.)
  - Audio management with screen reading support
  - PNG image manipulation
  - Text output and graphics output implementations

Early history of the codebase could be found in [AppleSupportPkg](https://github.com/acidanthera/AppleSupportPkg) and PicoLib library set by The HermitCrabs Lab.

### :lock: OcGuardLib
This library implements basic safety features recommended for the use within the project. It implements fast
safe integral arithmetics mapping on compiler builtins, type alignment checking, and UBSan runtime,
based on [NetBSD implementation](https://blog.netbsd.org/tnf/entry/introduction_to_µubsan_a_clean).

The use of UBSan runtime requires the use of Clang compiler and `-fsanitize=undefined` argument. Refer to
[Clang documentation](https://releases.llvm.org/7.0.0/tools/clang/docs/UndefinedBehaviorSanitizer.html) for more
details.

## :star2: Credits
Thank you to everyone who contributed to OpenCore! Your efforts mean a lot. :blue_heart:

<details>
<summary>Special thanks to:</summary>

- The HermitCrabs Lab
- All projects providing third-party code (refer to file headers)
- [AppleLife](https://applelife.ru) team and user-contributed resources
- Chameleon and Clover teams for hints and legacy
- [al3xtjames](https://github.com/al3xtjames)
- [Andrey1970AppleLife](https://github.com/Andrey1970AppleLife)
- [mhaeuser (ex Download-Fritz)](https://github.com/mhaeuser)
- [Goldfish64](https://github.com/Goldfish64)
- [MikeBeaton](https://github.com/MikeBeaton)
- [nms42](https://github.com/nms42)
- [PMheart](https://github.com/PMheart)
- [savvamitrofanov](https://github.com/savvamitrofanov)
- [usr-sse2](https://github.com/usr-sse2)
- [vit9696](https://github.com/vit9696)
</details>

## :speech_balloon: Discussion
Please refer to the following [list of OpenCore discussion forums](/Docs/FORUMS.md).
