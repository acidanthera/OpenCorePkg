ocvalidate Checklist
=====================

As of commit <TODO>, ocvalidate performs the following checks:

## ACPI
- [ ] Add->Entry[N]->Path: Only `0-9, A-Z, a-z, '_', '-', '.', '/', and '\'` are accepted.
- [ ] Add->Entry[N]->Path: `.dsl` filename suffix is not accepted.
- [ ] Add->Entry[N]->Path: If a customised DSDT is added, remind to enable RebaseRegions in Quirks.
- [ ] Add->Entry[N]->Comment: Only ASCII printable characters are accepted.
- [ ] Delete->Entry[N]->Comment: Only ASCII printable characters are accepted.
- [ ] Delete->Entry[N]->OemTableId/TableSignature: No size check for these two, as serialisation kills it.
- [ ] Patch->Entry[N]->Comment: Only ASCII printable characters are accepted.
- [ ] Patch->Entry[N]->OemTableId/TableSignature: No size check for these two, as serialisation kills it.
- [ ] Patch->Entry[N]->Find/Replace/Mask/ReplaceMask: Identical size of these patterns guaranteed.

## Booter
- [ ] MmioWhitelist->Entry[N]->Comment: Only ASCII printable characters are accepted.
- [ ] MmioWhitelist->Entry[N]->Enabled: When at least one entry is enabled, remind to enable DevirtualiseMmio in Quirks.
- [ ] Patch->Entry[N]->Comment: Only ASCII printable characters are accepted.
- [ ] Patch->Entry[N]->Arch: Only `Empty string, Any, i386, x86_64` are accepted.
- [ ] Patch->Entry[N]->Identifier: Only `Empty string, Any, Apple`, or a specified bootloader with `.efi` sufffix, are accepted.
- [ ] Patch->Entry[N]->Find/Replace/Mask/ReplaceMask: Identical size of these patterns guaranteed.
- [ ] Quirks->AllowRelocationBlock: When enabled, ProvideCustomSlide should be enabled altogether.
- [ ] Quirks->AllowRelocationBlock: When enabled, AvoidRuntimeDefrag is highly recommended to be enabled as well.
- [ ] Quirks->EnableSafeModeSlide: When enabled, ProvideCustomSlide should be enabled altogether.

## DeviceProperties
- [ ] TODO

## Kernel
- [ ] TODO

## Misc
- [ ] TODO

## NVRAM
- [ ] TODO

## PlatformInfo
- [ ] TODO

## UEFI
- [ ] TODO
