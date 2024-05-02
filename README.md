<img src="/Docs/Logos/OpenCore_with_text_Small.png" width="200" height="48"/>

[![Build Status](https://github.com/acidanthera/OpenCorePkg/workflows/CI/badge.svg?branch=master)](https://github.com/acidanthera/OpenCorePkg/actions) [![Scan Status](https://scan.coverity.com/projects/18169/badge.svg?flat=1)](https://scan.coverity.com/projects/18169)
-----

OpenCore bootloader with development SDK.

## Branch build-0.8.7

This branch contains a buildable version of OpenCore prior to integration
of the new, secure PE/COFF loader. The purpose is to enable source-level
debugging, when needed, of the operation of the code prior to this
integration.

This build (particularly the version of [audk](https://github.com/acidanthera/audk/tree/build-0.8.7) which it links to)
has been reconstructed, i.e. it is not exactly the same as was used in any
particular version of OpenCore, but is intended to be the same in all or most
important respects as was used to build OpenCore 0.8.7 (see [issue](https://github.com/acidanthera/bugtracker/issues/2195)).

In addition to the exact commits up to OpenCore `0.8.7` tag, this version
adds:

 - All the more recent updates to OpenDuet made in versions 0.9.9 and 1.0.0,
   which are required to enable source-level debugging in OpenDuet.
 - A new fix to the OpenDuet 0.8.7 and earlier EfiLdr HOB (also required for
   source-level debugging, in these versions).
 - Some minor changes early in this branch, required to get the build
   working locally with the reconstructed audk version.
 - Various more recent commits with minor fixes, taken from the OpenCore
   master branch since the 0.8.7 version, as required to get CI fully
   working.
 - The current `Debug` folder, since this works with both old (this
   branch) and new (master branch) executable formats, but has
   various useful updates compared to the debug code from the 0.8.7 branch.

Commits from the master branch have been cherry-picked, with the same commit
date, author and commit message as the original, in their original order,
to try to make it easy to see where they are from.

*Unlike the OpenCore master branch, there is no particular intention to
avoid force-pushes to this branch. Of course you should still be able
to rebase any changes you might make on top of this branch, just be aware
that you may well need to do so.*

## Libraries

This repository also contains additional UEFI support common libraries shared by other projects in [Acidanthera](https://github.com/acidanthera). The primary purpose of the library set is to provide supplemental functionality for Apple-specific UEFI drivers. Key features:

- Apple disk image loading support
- Apple keyboard input aggregation
- Apple PE image signature verification
- Apple UEFI secure boot supplemental code
- Audio management with screen reading support
- Basic ACPI and SMBIOS manipulation
- CPU information gathering with timer support
- Cryptographic primitives (SHA-256, RSA, etc.)
- Decompression primitives (zlib, lzss, lzvn, etc.)
- Helper code for ACPI reads and modifications
- Higher level abstractions for files, strings, UEFI variables
- Overflow checking arithmetics
- PE image loading with no UEFI Secure Boot conflict
- Plist configuration format parsing
- PNG image manipulation
- Text output and graphics output implementations
- XNU kernel driver injection and patch engine

Early history of the codebase could be found in [AppleSupportPkg](https://github.com/acidanthera/AppleSupportPkg) and PicoLib library set by The HermitCrabs Lab.

#### OcGuardLib

This library implements basic safety features recommended for the use within the project. It implements fast
safe integral arithmetics mapping on compiler builtins, type alignment checking, and UBSan runtime,
based on [NetBSD implementation](https://blog.netbsd.org/tnf/entry/introduction_to_µubsan_a_clean).

The use of UBSan runtime requires the use of Clang compiler and `-fsanitize=undefined` argument. Refer to
[Clang documentation](https://releases.llvm.org/7.0.0/tools/clang/docs/UndefinedBehaviorSanitizer.html) for more
details.

#### Credits

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

#### Discussion

Please refer to the following [list of OpenCore discussion forums](/Docs/FORUMS.md).
