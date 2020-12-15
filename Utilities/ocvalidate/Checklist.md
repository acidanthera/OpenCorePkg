ocvalidate Checklist
=====================

As of commit <TODO>, ocvalidate performs the following checks:

## Global Rules
- For all comments (Section `Comment` in all possible fields throughout the whole config) only ASCII printable characters are accepted.
- For all patches, excluding section `Kernel->Patch` (where `Base` is not empty), their `Find`, `Replace`, `Mask`, and `ReplaceMask` must have identical size in most cases.
- For all `MinKernel` and `MaxKernel` settings, they should follow the conventions indicated in [Configuration.pdf](https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/Configuration.pdf).

## ACPI
- Add->Entry[N]->Path: Only `0-9, A-Z, a-z, '_', '-', '.', '/', and '\'` are accepted.
- Add->Entry[N]->Path: `.dsl` filename suffix is not accepted.
- Add->Entry[N]->Path: If a customised DSDT is added, remind to enable RebaseRegions in Quirks.

## Booter
- MmioWhitelist->Entry[N]->Enabled: When at least one entry is enabled, remind to enable DevirtualiseMmio in Quirks.
- Patch->Entry[N]->Arch: Only `Empty string, Any, i386, x86_64` are accepted.
- Patch->Entry[N]->Identifier: Only `Empty string, Any, Apple`, or a specified bootloader with `.efi` sufffix, are accepted.
- Quirks->AllowRelocationBlock: When enabled, ProvideCustomSlide should be enabled altogether.
- Quirks->EnableSafeModeSlide: When enabled, ProvideCustomSlide should be enabled altogether.
- Quirks->ProvideMaxSlide: If set to a number greater than zero, ProvideCustomSlide should be enabled altogether.
- Quirks->DisableVariableWrite/EnableWriteUnprotector/ProvideCustomSlide: When either is enabled, `OpenRuntime.efi` should be loaded under `UEFI->Drivers`.
- Quirks->EnableWriteUnprotector/RebuildAppleMemoryMap: These two cannot be enabled simultaneously.
- Quirks->RebuildAppleMemoryMap: When enabled, SyncRuntimePermissions is highly recommended to be enabled as well.

## DeviceProperties
- TODO

## Kernel
- Add->Entry[N]->Arch: Only `Any, i386, x86_64` are accepted.
- Add->Entry[N]->BundlePath: Only `0-9, A-Z, a-z, '_', '-', '.', '/', and '\'` are accepted.
- Add->Entry[N]->ExecutablePath: Only `0-9, A-Z, a-z, '_', '-', '.', '/', and '\'` are accepted.
- Add->Entry[N]->PlistPath: Only `0-9, A-Z, a-z, '_', '-', '.', '/', and '\'` are accepted.
- Delete->Entry[N]->Arch: Only `Any, i386, x86_64` are accepted.
- Delete->Entry[N]->Identifier: Only `0-9, A-Z, a-z, '_', '-', '.', '/', and '\'` are accepted.

## Misc
- TODO

## NVRAM
- TODO

## PlatformInfo
- TODO

## UEFI
- APFS->EnableJumpstart: When enabled, Misc->Security->ScanPolicy should have OC_SCAN_ALLOW_FS_APFS (bit 8) set, or ScanPolicy should be 0 (failsafe value).
- Quirks->RequestBootVarRouting: When enabled, `OpenRuntime.efi` should be loaded under `UEFI->Drivers`.
- Quirks->DeduplicateBootOrder: When enabled, RequestBootVarRouting should be enabled altogether.
- Driver OpenUsbKbDxe.efi: When in use, UEFI->Input->KeySupport should never be enabled altogether.
- Driver Ps2KeyboardDxe.efi: When in use, UEFI->Input->KeySupport should be enabled altogether.
- OpenUsbKbDxe.efi and Ps2KeyboardDxe.efi should never co-exist.
- Output->ClearScreenOnModeSwitch/IgnoreTextInGraphics/ReplaceTabWithSpace/SanitiseClearScreen: These only apply to `System` TextRenderer
- Output->ConsoleMode: Only ASCII printable characters are accepted.
- Output->TextRenderer: Only ASCII printable characters are accepted.
- Drivers[N]: Only ASCII printable characters are accepted.
