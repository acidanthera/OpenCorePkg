OpenCore Changelog
==================

#### v0.0.3
- Added complete modern platform database (2012+)
- Added `DisableIoMapper` kernel quirk
- Fixed ACPI modification failures with nested multiboot
- Dropped `IgnoreForWindows` quirk legacy
- Added basic AMD Zen CPU support
- Added `Misc` -> `Tools` section to add third-party tools
- Added `Kernel` -> `Emulate` section for CPUID patches
- Added `CustomSMBIOSGuid` quirk for Custom SMBIOS update mode
- Added `PanicNoKextDump` quirk to avoid kext dump in panics
- Switched to EDK II stable and reduced image size
- Added `LapicKernelPanic` kernel quirk
- Added `AppleXcpmExtraMsrs` quirk and improved XCPM patches
- Added `(external)` suffix for external drives in boot menu
- Added `UsePicker` option, do enable for OC boot management
- Added nvram.plist loading for legacy and incompatible platforms
- Improved instructions for legacy and Windows installation
- Added Windows Boot Camp switching support
- Added basic hibernation detection
- Added `ResetHwSig` ACPI quirk to workaround hibernation
- Removed `Custom` subfolder requirement from `ACPI` tables

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
