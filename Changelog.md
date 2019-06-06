OpenCore Changelog
==================

#### v0.0.3
- Added complete modern platform database (2012+)
- Added `DisableIoMapper` kernel quirk
- Fixed ACPI modification failures with nested multiboot
- Dropped `IgnoreForWindows` quirk legacy
- Added basic AMD Zen CPU support
- Added `Misc` -> `Tools` section to add third-party tools

#### v0.0.2
- Documentation improvements (see Differences.pdf)
- Platform information database updates
- Fixed misbehaving `Debug` -> `Target` enable bit
- Added `ResetLogoStatus` ACPI quirk
- Added `SpoofVendor` PlatformInfo feature
- Replaced `ExposeBootPath` with `ExposeSensitiveData`
- Added builtin implementation of Data Hub protocol
- Dropped `UpdateSMBIOSMode` `Auto` mode in favour of `Create`
- Fixed SMBIOS CPU detection for Xeon and Core models
- Moved `ConsoleControl` configuration to `Protocols`
- Added `Security` -> `ScanPolicy` preference
- Fixed invalid `board-rev` exposure in Data Hub
- Fixed SMBIOS Type 133 table exposure
- Added support for SMBIOS Type 134 table exposure

#### v0.0.1
- Initial developer preview release
