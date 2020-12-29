ocvalidate Checklist
====================

This version of ocvalidate performs the following checks:

## Global Rules
- For all strings (fields with plist `String` format) throughout the whole config, only ASCII printable characters are accepted at most. Stricter rules may apply. For instance, some fields only accept specified values, as indicated in [Configuration.pdf](https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/Configuration.pdf).
- For all patches, excluding section `Kernel->Patch` (where `Base` is not empty), their `Find`, `Replace`, `Mask`, and `ReplaceMask` must have identical size in most cases. Also, `Find` requires `Mask` (or `Replace` requires `ReplaceMask`) to be active (set to non-zero) for corresponding bits.
- For all `MinKernel` and `MaxKernel` settings, they should follow the conventions indicated in [Configuration.pdf](https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/Configuration.pdf). (TODO: Bring decent checks for this)
- For all entries taking file system path only `0-9, A-Z, a-z, '_', '-', '.', '/', and '\'` are accepted.
- For all Device Paths (e.g. `PciRoot(0x0)/Pci(0x1b,0x0)`) only strings in canonic string format are accepted.
- For all paths of UEFI Drivers, only `0-9, A-Z, a-z, '_', '-', '.', and '/'` are accepted.
- For all entries requiring bitwise operations (e.g. `ConsoleAttributes`, `PickerAttributes`, or `ScanPolicy`), only known bits can be set.
- For all entries involving GUID (mainly at Section `NVRAM`), correct format must be ensured.

## ACPI
#### Add
- Entry[N]->Path: Only `.aml` and `.bin` filename suffix are accepted.
- Entry[N]->Path: If a customised DSDT is added and enabled, `RebaseRegions` in `Quirks` should be enabled.

## Booter
#### MmioWhitelist
- Entry[N]->Enabled: When at least one entry is enabled, `DevirtualiseMmio` in `Quirks` should be enabled.
#### Patch
- Entry[N]->Arch: Only `Any`, `i386`, or `x86_64` are accepted.
- Entry[N]->Identifier: Only `Any`, `Apple`, or a specified bootloader with `.efi` sufffix, are accepted.
#### Quirks
- When `AllowRelocationBlock` is enabled, `ProvideCustomSlide` should be enabled altogether.
- When `EnableSafeModeSlide` is enabled, `ProvideCustomSlide` should be enabled altogether.
- If `ProvideMaxSlide` is set to a number greater than zero (i.e. is enabled), `ProvideCustomSlide` should be enabled altogether.
- When `DisableVariableWrite`, `EnableWriteUnprotector`, or `ProvideCustomSlide` is enabled, `OpenRuntime.efi` should be loaded under `UEFI->Drivers`.

## DeviceProperties
- Check requirements for Device Paths in Section Global Rules.

## Kernel
#### Add
- Entry[N]->Arch: Only `Any`, `i386`, or `x86_64` are accepted.
- Entry[N]->BundlePath: Filename should have `.kext` suffix.
- Entry[N]->PlistPath: Filename should have `.plist` suffix.
- Entry[N]: If `Lilu.kext` is used, `DisableLinkeditJettison` should be enabled at `Kernel->Quirks`.
- For some known kexts, their `BundlePath`, `ExecutablePath`, and `PlistPath` must match against each other. Current list of rules can be found [here](https://github.com/acidanthera/OpenCorePkg/blob/master/Utilities/ocvalidate/ValidateKernel.c).
- Known [Lilu plugins](https://github.com/acidanthera/Lilu/blob/master/KnownPlugins.md) must have proper `Add` precedence. That is to say, plugins must be placed after `Lilu.kext`.
#### Delete
- Entry[N]->Arch: Only `Any`, `i386`, or `x86_64` are accepted.
- Entry[N]->Identifier: At least one dot (`.`) should exist, because any identifier looks like a domain sequence (`vendor.product`).
#### Quirks
- `CustomSMBIOSGuid` requires `UpdateSMBIOSMode` at `PlatformInfo` set to `Custom`.
#### Scheme
- KernelArch: Only `Auto`, `i386`, `i386-user32`, or `x86_64` are accepted.
- KernelCache: Only `Auto`, `Cacheless`, `Mkext`, or `Prelinked` are accepted.

## Misc
#### Boot
- HibernateMode: Only `None`, `Auto`, `RTC`, or `NVRAM` are accepted.
- PickerMode: Only `Builtin`, `External`, or `Apple` are accepted.
#### Security
- BootProtect: Only `None`, `Bootstrap`, or `BootstrapShort` are accepted. When set to `Bootstrap` or `BootstrapShort`, `RequestBootVarRouting` should be enabled at `UEFI->Quirks`.
- DmgLoading: Only `Disabled`, `Signed`, or `Any` are accepted.
- Vault: Only `Optional`, `Basic`, or `Secure` are accepted.
- SecureBootModel: Only `Default`, `Disabled`, `j137`, `j680`, `j132`, `j174`, `j140k`, `j780`, `j213`, `j140a`, `j152f`, `j160`, `j230k`, `j214k`, `j223`, `j215`, `j185`, `j185f`, or `x86legacy` are accepted.

## NVRAM
- Check requirements for GUID in Section Global Rules.

## PlatformInfo
- UpdateSMBIOSMode: Only `TryOverwrite`, `Create`, `Overwrite`, or `Custom` are accepted.
#### Generic
- SystemProductName: Only real Mac models are accepted.
- SystemMemoryStatus: Only `Auto`, `Upgradable`, or `Soldered` are accepted.

## UEFI
#### APFS
- When `EnableJumpstart` is enabled, `ScanPolicy` at `Misc->Security` should have `OC_SCAN_ALLOW_FS_APFS` (bit 8) set, or `ScanPolicy` should be `0` (failsafe value).
#### Quirks
- When `RequestBootVarRouting` is enabled, `OpenRuntime.efi` should be loaded under `UEFI->Drivers`.
#### Drivers
- No drivers should be loaded more than once (i.e. there should NOT be any duplicated entries in this section).
- When `OpenUsbKbDxe.efi` is in use, `KeySupport` at `UEFI->Input` should NEVER be enabled altogether.
- When `Ps2KeyboardDxe.efi` is in use, `KeySupport` at `UEFI->Input` should be enabled altogether.
- `OpenUsbKbDxe.efi` and `Ps2KeyboardDxe.efi` should never co-exist.
#### Input
- The value of `KeySupportMode` can only be `Auto`, `V1`, `V2`, or `AMI`.
- When `PointerSupport` is enabled, the value of `PointerSupportMode` should only be `ASUS`.
#### Output
- `ClearScreenOnModeSwitch`, `IgnoreTextInGraphics`, `ReplaceTabWithSpace`, and `SanitiseClearScreen` only apply to `System` TextRenderer
- `Resolution` should match `NUMBERxNUMBER` or `NUMBERxNUMBER@NUMBER` sequences (unless it is an `Empty string` or is set to `Max`).
#### ReservedMemory
- Type: Only `Reserved`, `LoaderCode`, `LoaderData`, `BootServiceCode`, `BootServiceData`, `RuntimeCode`, `RuntimeData`, `Available`, `Persistent`, `UnusableMemory`, `ACPIReclaimMemory`, `ACPIMemoryNVS`, `MemoryMappedIO`, `MemoryMappedIOPortSpace`, and `PalCode` are accepted.
