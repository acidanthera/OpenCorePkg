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
- All the paths relative to OpenCore root must be less than or equal to 128 bytes (`OC_STORAGE_SAFE_PATH_MAX`) in total including '\0' terminator.
- Most binary patches must have `Find`, `Replace`, `Mask` (if used), and `ReplaceMask` (if used) identical size set. Also, `Find` requires `Mask` (or `Replace` requires `ReplaceMask`) to be active (set to non-zero) for corresponding bits.
- `MinKernel` and `MaxKernel` entries should follow conventions specified in [Configuration.pdf](https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/Configuration.pdf). (TODO: Bring decent checks for this)
- `MinKernel` cannot be a value that is below macOS 10.4 (Darwin version 8).
- Entries taking file system path only accept `0-9, A-Z, a-z, '_', '-', '.', '/', and '\'`.
- Device Paths (e.g. `PciRoot(0x0)/Pci(0x1b,0x0)`) only accept strings in canonic string format.
- Paths of UEFI Drivers only accept `0-9, A-Z, a-z, '_', '-', '.', and '/'`.
- Entries requiring bitwise operations (e.g. `ConsoleAttributes`, `PickerAttributes`, or `ScanPolicy`) only allow known bits stated in [Configuration.pdf](https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/Configuration.pdf) to be set.
- Entries involving GUID (mainly in Section `NVRAM`) must have correct format set.

### ACPI
#### Add
- Entry[N]->Path: Only `.aml` and `.bin` filename suffix are accepted.

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
- `ResizeAppleGpuBars` must be set to `0` or `-1`.
- When `DisableVariableWrite`, `EnableWriteUnprotector`, or `ProvideCustomSlide` is enabled, `OpenRuntime.efi` should be loaded in `UEFI->Drivers`.

### DeviceProperties
- Requirements here all follow Global Rules.

### Kernel
#### Add
- Entry[N]->Arch: Only `Any`, `i386`, or `x86_64` are accepted.
- Entry[N]->BundlePath: Filename should have `.kext` suffix.
- Entry[N]->PlistPath: Filename should have `.plist` suffix.
- Entry[N]: If `Lilu.kext` is used, `DisableLinkeditJettison` should be enabled in `Kernel->Quirks`.
- `BrcmFirmwareRepo.kext` must not be injected by OpenCore.
- For some known kexts, their `BundlePath`, `ExecutablePath`, and `PlistPath` must match against each other. Current list of rules can be found [here](https://github.com/acidanthera/OpenCorePkg/blob/master/Utilities/ocvalidate/KextInfo.c).
- Plugin kext must be placed after parent kext. For example, [plugins of Lilu](https://github.com/acidanthera/Lilu/blob/master/KnownPlugins.md) must be placed after `Lilu.kext`.
#### Delete
- Entry[N]->Arch: Only `Any`, `i386`, or `x86_64` are accepted.
- Entry[N]->Identifier: At least one dot (`.`) should exist, because any identifier looks like a domain sequence (`vendor.product`).
#### Quirks
- `CustomSMBIOSGuid` requires `PlatformInfo->UpdateSMBIOSMode` set to `Custom`.
- `SetApfsTrimTimeout` cannot be a value that is greater than `MAX_UINT32`, or less than `-1`.
#### Scheme
- KernelArch: Only `Auto`, `i386`, `i386-user32`, or `x86_64` are accepted.
- KernelCache: Only `Auto`, `Cacheless`, `Mkext`, or `Prelinked` are accepted.

### Misc
#### BlessOverride
- Entries cannot be `\EFI\Microsoft\Boot\bootmgfw.efi` or `\System\Library\CoreServices\boot.efi` since OpenCore knows these paths.
#### Boot
- HibernateMode: Only `None`, `Auto`, `RTC`, or `NVRAM` are accepted.
- PickerMode: Only `Builtin`, `External`, or `Apple` are accepted.
- `PickerAudioAssist` requires `AudioSupport` in `UEFI->Audio` to be enabled.
- LauncherOption: Only `Disabled`, `Full`, `Short`, or `System` are accepted.
- `LauncherPath` cannot be empty string.
#### Security
- AuthRestart: If enabled, `VirtualSMC.kext` should be present in `Kernel->Add`.
- DmgLoading: Only `Disabled`, `Signed`, or `Any` are accepted.
- Vault: Only `Optional`, `Basic`, or `Secure` are accepted.
- SecureBootModel: Only `Default`, `Disabled`, `j137`, `j680`, `j132`, `j174`, `j140k`, `j780`, `j213`, `j140a`, `j152f`, `j160`, `j230k`, `j214k`, `j223`, `j215`, `j185`, `j185f`, or `x86legacy` are accepted.
#### Serial
- RegisterAccessWidth: Only `8` or `32` are accepted.
- BaudRate: Only `921600`, `460800`, `230400`, `115200`, `57600`, `38400`, `19200`, `9600`, `7200`, `4800`, `3600`, `2400`, `2000`, `1800`, `1200`, `600`, `300`, `150`, `134`, `110`, `75`, or `50` are accepted.
- PciDeviceInfo: The last byte must be `0xFF`.
- PciDeviceInfo: Excluding the last byte `0xFF`, the rest must be divisible by 4.
- PciDeviceInfo: Maximum allowed size is 41.

### NVRAM
- Requirements here all follow Global Rules. In addition, the following keys and values are checked:
#### gAppleBootVariableGuid (`7C436110-AB2A-4BBB-A880-FE41995C9F82`)
- `nvda_drv` must have type `Plist Data` with the value of `0x30` or `0x31`.
- `boot-args` must be an ASCII string (thus `Plist String`) without trailing `\0`.
- `bootercfg` must be an ASCII string (thus `Plist String`) without trailing `\0`.
- `csr-active-config` must have type `Plist Data` and have length of 4 bytes.
- `StartupMute` must have type `Plist Data` and have length of 1 byte.
- `SystemAudioVolume` must have type `Plist Data` and have length of 1 byte.
#### gAppleVendorVariableGuid (`4D1EDE05-38C7-4A6A-9CC6-4BCCA8B38C14`)
- `UIScale` must have type `Plist Data` with the value of `0x01` or `0x02`.
- `FirmwareFeatures` must have type `Plist Data` and have length of 4 bytes.
- `ExtendedFirmwareFeatures` must have type `Plist Data` and have length of 8 bytes.
- `FirmwareFeaturesMask` must have type `Plist Data` and have length of 4 bytes.
- `ExtendedFirmwareFeatures` must have type `Plist Data` and have length of 8 bytes.
- `DefaultBackgroundColor` must have type `Plist Data` and have length of 4 bytes. Also, its last byte must be `0x00`.

### PlatformInfo
- UpdateSMBIOSMode: Only `TryOverwrite`, `Create`, `Overwrite`, or `Custom` are accepted.
#### Generic
- SystemProductName: Only real Mac models are accepted.
- SystemMemoryStatus: Only `Auto`, `Upgradable`, or `Soldered` are accepted.
- SystemUUID: Only empty string, `OEM` or valid UUID are accepted.
- ProcessorType: Only known first byte can be set.

### UEFI
#### APFS
- When `EnableJumpstart` is enabled, `ScanPolicy` in `Misc->Security` should have `OC_SCAN_ALLOW_FS_APFS` (bit 8) set, together with `OC_SCAN_FILE_SYSTEM_LOCK` (bit 0) set. Or `ScanPolicy` should be `0` (failsafe value).
#### Audio
- When `AudioSupport` is enabled, `AudioDevice` must be either empty or a valid path.
- When `AudioSupport` is enabled, `AudioOutMask` must be non-zero.
#### Quirks
- When `RequestBootVarRouting` is enabled, `OpenRuntime.efi` should be loaded in `UEFI->Drivers`.
- `ResizeGpuBars` must be set to an integer value between `-1` and `19`.
#### Drivers
- When `OpenUsbKbDxe.efi` is in use, `KeySupport` in `UEFI->Input` should never be enabled altogether.
- When `Ps2KeyboardDxe.efi` is in use, `KeySupport` in `UEFI->Input` should always be enabled altogether.
- `OpenUsbKbDxe.efi` and `Ps2KeyboardDxe.efi` should never co-exist.
- When HFS+ filesystem driver or `AudioDxe.efi` is in use, `ConnectDrivers` should be enabled altogether.
- When `OpenCanopy.efi` is in use, `PickerMode` in `Misc->Boot` should be set to `External`.
- When `OpenVariableRuntimeDxe.efi` is in use, its `LoadEarly` option must be set to `TRUE`.
- `OpenRuntime.efi` must be placed after `OpenVariableRuntimeDxe.efi` when both are in use.
- `LoadEarly` for any other driver but `OpenVariableRuntimeDxe.efi` and `OpenRuntime.efi` must be set to `FALSE`.
#### Input
- KeySupportMode: Only `Auto`, `V1`, `V2`, or `AMI` are accepted.
- When `PointerSupport` is enabled, the value of `PointerSupportMode` should only be `ASUS`.
#### Output
- `ClearScreenOnModeSwitch`, `IgnoreTextInGraphics`, `ReplaceTabWithSpace`, and `SanitiseClearScreen` only apply to `System` TextRenderer
- `Resolution` should match `NUMBERxNUMBER` or `NUMBERxNUMBER@NUMBER` sequences (unless it is an `Empty string` or is set to `Max`).
- `UIScale` must be set to an integer value between `-1` and `2`.
#### ReservedMemory
- Type: Only `Reserved`, `LoaderCode`, `LoaderData`, `BootServiceCode`, `BootServiceData`, `RuntimeCode`, `RuntimeData`, `Available`, `Persistent`, `UnusableMemory`, `ACPIReclaimMemory`, `ACPIMemoryNVS`, `MemoryMappedIO`, `MemoryMappedIOPortSpace`, or `PalCode` are accepted.
