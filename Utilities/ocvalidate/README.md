ocvalidate
====================

Utility to validate whether a `config.plist` matches requirements and conventions imposed by OpenCore.

## Usage
- Pass one single path to `config.plist` to verify it.
- Pass `--version` for current supported OpenCore version.

## Technical background
### At a glance
- ocvalidate firstly calls `OcSerializeLib` which performs fundamental checks in terms of syntax and semantics. After that, the following will be checked.
- The error message `<OCS: No schema for xxx>` complained by `OcSerializeLib` indicates unknown keys that can be deprecated in new versions of OpenCore. Such keys should be ***removed*** in order to avoid undefined behaviours.
- Under active development, newer versions of OpenCore hardly have backward compatibility at this moment. As a result, please first run `ocvalidate --version` to check which version of OpenCore is supported, and thus please only use the specific version. 

### Global Rules
- All entries must be set once only. Duplication is strictly prohibited.
- All strings (fields with plist `String` format) throughout the whole config only accept ASCII printable characters at most. Stricter rules may apply. For instance, some fields only accept specified values, as indicated in [Configuration.pdf](https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/Configuration.pdf).
- Most binary patches must have `Find`, `Replace`, `Mask` (if used), and `ReplaceMask` (if used) identical size set. Also, `Find` requires `Mask` (or `Replace` requires `ReplaceMask`) to be active (set to non-zero) for corresponding bits.
- `MinKernel` and `MaxKernel` entries should follow conventions specified in [Configuration.pdf](https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/Configuration.pdf). (TODO: Bring decent checks for this)
- Entries taking file system path only accept `0-9, A-Z, a-z, '_', '-', '.', '/', and '\'`.
- Device Paths (e.g. `PciRoot(0x0)/Pci(0x1b,0x0)`) only accept strings in canonic string format.
- Paths of UEFI Drivers only accept `0-9, A-Z, a-z, '_', '-', '.', and '/'`.
- Entries requiring bitwise operations (e.g. `ConsoleAttributes`, `PickerAttributes`, or `ScanPolicy`) only allow known bits stated in [Configuration.pdf](https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/Configuration.pdf) to be set.
- Entries involving GUID (mainly in Section `NVRAM`) must have correct format set.

### ACPI
#### Add
- Entry[N]->Path: Only `.aml` and `.bin` filename suffix are accepted.
- Entry[N]->Path: If a customised DSDT is added and enabled, `RebaseRegions` in `ACPI->Quirks` should be enabled.

### Booter
#### MmioWhitelist
- Entry[N]->Enabled: When at least one entry is enabled, `DevirtualiseMmio` in `Booter->Quirks` should be enabled.
#### Patch
- Entry[N]->Arch: Only `Any`, `i386`, or `x86_64` are accepted.
- Entry[N]->Identifier: Only `Any`, `Apple`, or a specified bootloader with `.efi` sufffix, are accepted.
#### Quirks
- When `AllowRelocationBlock` is enabled, `ProvideCustomSlide` should be enabled altogether.
- When `EnableSafeModeSlide` is enabled, `ProvideCustomSlide` should be enabled altogether.
- If `ProvideMaxSlide` is set to a number greater than zero, `ProvideCustomSlide` should be enabled altogether.
- When `DisableVariableWrite`, `EnableWriteUnprotector`, or `ProvideCustomSlide` is enabled, `OpenRuntime.efi` should be loaded in `UEFI->Drivers`.

### DeviceProperties
- Requirements here all follow Global Rules.

### Kernel
#### Add
- Entry[N]->Arch: Only `Any`, `i386`, or `x86_64` are accepted.
- Entry[N]->BundlePath: Filename should have `.kext` suffix.
- Entry[N]->PlistPath: Filename should have `.plist` suffix.
- Entry[N]: If `Lilu.kext` is used, `DisableLinkeditJettison` should be enabled in `Kernel->Quirks`.
- For some known kexts, their `BundlePath`, `ExecutablePath`, and `PlistPath` must match against each other. Current list of rules can be found [here](https://github.com/acidanthera/OpenCorePkg/blob/master/Utilities/ocvalidate/ValidateKernel.h).
- Plugin kext must be placed after parent kext. For example, [plugins of Lilu](https://github.com/acidanthera/Lilu/blob/master/KnownPlugins.md) must be placed after `Lilu.kext`.
#### Delete
- Entry[N]->Arch: Only `Any`, `i386`, or `x86_64` are accepted.
- Entry[N]->Identifier: At least one dot (`.`) should exist, because any identifier looks like a domain sequence (`vendor.product`).
#### Quirks
- `CustomSMBIOSGuid` requires `PlatformInfo->UpdateSMBIOSMode` set to `Custom`.
#### Scheme
- KernelArch: Only `Auto`, `i386`, `i386-user32`, or `x86_64` are accepted.
- KernelCache: Only `Auto`, `Cacheless`, `Mkext`, or `Prelinked` are accepted.

### Misc
#### Boot
- HibernateMode: Only `None`, `Auto`, `RTC`, or `NVRAM` are accepted.
- PickerMode: Only `Builtin`, `External`, or `Apple` are accepted. When set to `External`, `OpenCanopy.efi` should be loaded in `UEFI->Drivers`.
- `PickerAudioAssist` requires `AudioSupport` in `UEFI->Audio` to be enabled.
#### Security
- AuthRestart: If enabled, `VirtualSMC.kext` should be present in `Kernel->Add`.
- BootProtect: Only `None`, `Bootstrap`, or `BootstrapShort` are accepted. When set to the latter two, `RequestBootVarRouting` should be enabled in `UEFI->Quirks`.
- DmgLoading: Only `Disabled`, `Signed`, or `Any` are accepted.
- Vault: Only `Optional`, `Basic`, or `Secure` are accepted.
- SecureBootModel: Only `Default`, `Disabled`, `j137`, `j680`, `j132`, `j174`, `j140k`, `j780`, `j213`, `j140a`, `j152f`, `j160`, `j230k`, `j214k`, `j223`, `j215`, `j185`, `j185f`, or `x86legacy` are accepted.

### NVRAM
- Requirements here all follow Global Rules.

### PlatformInfo
- UpdateSMBIOSMode: Only `TryOverwrite`, `Create`, `Overwrite`, or `Custom` are accepted.
#### Generic
- SystemProductName: Only real Mac models are accepted.
- SystemMemoryStatus: Only `Auto`, `Upgradable`, or `Soldered` are accepted.
- ProcessorType: Only known first byte can be set.

### UEFI
#### APFS
- When `EnableJumpstart` is enabled, `ScanPolicy` in `Misc->Security` should have `OC_SCAN_ALLOW_FS_APFS` (bit 8) set, together with `OC_SCAN_FILE_SYSTEM_LOCK` (bit 0) set. Or `ScanPolicy` should be `0` (failsafe value).
#### Audio
- When `AudioSupport` is enabled, AudioDevice cannot be empty and must be a valid path.
#### Quirks
- When `RequestBootVarRouting` is enabled, `OpenRuntime.efi` should be loaded in `UEFI->Drivers`.
#### Drivers
- When `OpenUsbKbDxe.efi` is in use, `KeySupport` in `UEFI->Input` should never be enabled altogether.
- When `Ps2KeyboardDxe.efi` is in use, `KeySupport` in `UEFI->Input` should always be enabled altogether.
- `OpenUsbKbDxe.efi` and `Ps2KeyboardDxe.efi` should never co-exist.
- When HFS+ filesystem driver or `AudioDxe.efi` is in use, `ConnectDrivers` should be enabled altogether.
- When `OpenCanopy.efi` is in use, `PickerMode` in `Misc->Boot` should be set to `External`.
#### Input
- KeySupportMode: Only `Auto`, `V1`, `V2`, or `AMI` are accepted.
- When `PointerSupport` is enabled, the value of `PointerSupportMode` should only be `ASUS`.
#### Output
- `ClearScreenOnModeSwitch`, `IgnoreTextInGraphics`, `ReplaceTabWithSpace`, and `SanitiseClearScreen` only apply to `System` TextRenderer
- `Resolution` should match `NUMBERxNUMBER` or `NUMBERxNUMBER@NUMBER` sequences (unless it is an `Empty string` or is set to `Max`).
#### ReservedMemory
- Type: Only `Reserved`, `LoaderCode`, `LoaderData`, `BootServiceCode`, `BootServiceData`, `RuntimeCode`, `RuntimeData`, `Available`, `Persistent`, `UnusableMemory`, `ACPIReclaimMemory`, `ACPIMemoryNVS`, `MemoryMappedIO`, `MemoryMappedIOPortSpace`, or `PalCode` are accepted.
