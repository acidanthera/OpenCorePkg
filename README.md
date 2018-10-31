OcSupportPkg
============

Additional UEFI support common libraries shared by other projects in [Acidanthera](https://github.com/acidanthera). The primary purpose of the library set is to provide supplemental functionality for Apple-specific UEFI drivers.

Early history of the codebase could be found in [AppleSupportPkg](https://github.com/acidanthera/AppleSupportPkg) and PicoLib library set by The HermitCrabs Lab.

## Features

- Apple PE image signature verification
- CPU information gathering
- Cryptographic primitives (SHA-256, RSA, etc.)
- Helper code for ACPI reads and modifications
- Higher level abstractions for files, strings, timers, variables
- Overflow checking arithmetics
- PE image loading with no UEFI Secure Boot conflict
- Plist configuration format parsing
- PNG image loading

#### OcGuardLib

This library implements basic safety features recommended for the use within the project. It implements fast
safe integral arithmetics mapping on compiler builtins, type alignment checking, and UBSan runtime,
based on [NetBSD implementation](https://blog.netbsd.org/tnf/entry/introduction_to_µubsan_a_clean).

The use of UBSan runtime requires the use of Clang compiler and `-fsanitize=undefined` argument. Refer to
[Clang documentation](https://releases.llvm.org/7.0.0/tools/clang/docs/UndefinedBehaviorSanitizer.html) for more
details. Among the known compatibility issues with EDK2 and UDK is broken `CR` implementation, causing false positives.
To workaround a flood of warnings, until a fix is merged upstream, replace the line:
```C
#define BASE_CR(Record, TYPE, Field)  ((TYPE *) ((CHAR8 *) (Record) - (CHAR8 *) &(((TYPE *) 0)->Field)))
```
in `MdePkg/Include/Base.h` with C-conformant implementation:
```C
#define BASE_CR(Record, TYPE, Field)  ((TYPE *) ((CHAR8 *) (Record) - OFFSET_OF (TYPE, Field)))
```

## Credits

- The HermitCrabs Lab
- All projects providing third-party code (refer to file headers)
- [Download-Fritz](https://github.com/Download-Fritz)
- [savvamitrofanov](https://github.com/savvamitrofanov)
- [vit9696](https://github.com/vit9696)
