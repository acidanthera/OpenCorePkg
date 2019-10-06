OpenCore Changelog
==================

#### v0.5.1
- Added support of kernel resource kext injection
- Added support for 0.25% clock slowdown on Xeon Scalable CPUs (thx @mrmiller)
- Replaced `MatchKernel` with `MinKernel` and `MaxKernel`
- Added `Arguments` to `Tools` and `Entries` sections
- Fixed broken timer for 300 series Intel chipsets
- Added `Input` section for mouse and keyboard aggregation

#### v0.5.0
- Added builtin firmware versions for new models 2019
- Fixed LogoutHook leaving random directories in `$HOME`
- Fixed FSBFrequency calculation on Xeon Scalable CPUs (thx @mrmiller)
- Fixed ARTFrequency specifying on Intel server and atom models
- Increased log size to 256 KB by default
- Added `ReplaceTabWithSpace` quirk to improve Shell experience
- Added `ClearScreenOnModeSwitch` quirk to avoid visual glitches
- Added `MISC_PWR_MGMT` patch to `AppleXcpmExtraMsrs` quirk (thx @mrmiller)
- Added `DevirtualiseMmio` quirk to `Booter` section
- Added FileVault 2 user interface protocols formerly in AppleUiSupport
- Improved kernel patch logging to include configuration comments
- Added MSFT basic data and Linux root fs recognition to `ScanPolicy`
- Fixed RT region protection restoration regression (thx Sniki)
- Added `OPT`, `CMD+R`, `CMD+OPT+P+R` boot action hotkey support
- Added `PollAppleHotKeys` to register boot.efi hotkeys in the picker
- Added `DisableSingleUser` quirk to prohibit single user mode
- Upgraded EDK II base package to edk2-stable201908
- Prohibited argument changing by BootNext

#### v0.0.4
- Fixed kext injection issues with dummy dependencies
- Fixed kext injection issues with reused vtables
- Fixed Custom SMBIOS table update patches
- Added timestamp to the log file and changed extension to txt
- Enhanced `LogoutHook` script used for emulated NVRAM saving
- Fixed multiple operating system support in APFS containers
- Added `AvoidHighAlloc` UEFI quirk to avoid high memory allocs
- Updated builtin firmware versions for 10.15 beta support
- Added `Booter` section for Apple bootloader preferences
- Dropped `AptioMemoryFix.efi` support for `Booter` and `FwRuntimeServices.efi`
- Fixed hibernation issues in Windows with `RequestBootVarRouting`
- Significantly improved boot stability on APTIO
- Added support for Windows & OpenCore on the same drive through `BlessOverride`
- Added advanced user-specified boot entries through `Misc` -> `Entries`
- Added `DisableVariableWrite` quirk to disable hardware NVRAM write in macOS

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
- Added basic hibernation detection & support
- Added `ResetHwSig` ACPI quirk to workaround hibernation
- Removed `Custom` subfolder requirement from `ACPI` tables
- Fixed kext injection in 10.7.x and 10.8.x
- Added ESP partition type detection to ScanPolicy
- Added support for third-party user interfaces

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
