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

## Credits

- The HermitCrabs Lab
- [Download-Fritz](https://github.com/Download-Fritz)
- [savvamitrofanov](https://github.com/savvamitrofanov)
- [vit9696](https://github.com/vit9696)
