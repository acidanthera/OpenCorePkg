OpenDuetPkg
===========

Acidanthera variant of DuetPkg. Specialties:

- Significantly improved boot performance.
- BDS is simplified to load `EFI/OC/OpenCore.efi`.
- EDID support removed for performance and compatibility.
- PCI option ROM support removed for security and performance.
- Setup and graphical interface removed.
- Resolved PCIe device path instead of PCI.
- Increased variable storage size to 64 KB.
- Always loads from bootstrapped volume first.
- Some changes inherited from Clover EFI bootloader.

## Simplified bootstrap process

1. `boot0` is loaded by BIOS from MBR at `7C00h`.
2. `boot0` finds active MBR or ESP GPT partition.
3. `boot0` loads `boot1` from found partition at `9000h`.
4. `boot1` finds `boot` file at partition root.
5. `boot1` loads `boot` at `20000h` (begins with `StartX64` or `StartIA32`).
6. `Start` enables paging (prebuilt for `X64` or generated for `IA32`).
7. `Start` switches to native CPU mode (`X64` or `IA32`) with paging.
8. `Start` jumps to concatentated `EfiXX` at `21000h` (`Efi64` or `Efi32`).
9. `EfiXX` loads `EfiLdr` (concatenated after `EfiXX`) at `10000h`.
10. `EfiLdr` decompresses and maps `DUETFV`, `DxeIpl`, `DxeCore`.
11. `EfiLdr` jumps to `DxeIpl`, which starts `DxeCore` from `DUETFV`.
12. `DxeCore` starts BDS, which loads `EFI/OC/OpenCore.efi`.

| Name      | PHYS        | VIRT        |
|:----------|:------------|:------------|
| `boot0`   | `7C00h`     |             |
| `boot1`   | `9000h`     |             |
| `Start`   | `20000h`    | `20000h`    |
| `EfiXX`   | `21000h`    | `21000h`    |
| `EfiLdr`  | `22000h`    | `10000h`    |
| `DxeIpl`+ | `22000h+`   | `???`       |

## Error codes

- `BOOT MISMATCH!` - Bootstrap partition signature identification failed.
    BDS code will try to lookup `EFI/OC/OpenCore.efi` on any partition in 3 seconds.
- `BOOT FAIL!` - No bootable `EFI/OC/OpenCore.efi` file found on any partition.
    BDS code will dead-loop CPU in 3 seconds.

## Compilation

By default [ocbuild](https://github.com/acidanthera/ocbuild)-like compilation is used via `macbuild.tool`.
As an alternative it is possible to perform in-tree compilation by using `INTREE=1` environment variable:

```
TARGETARCH=X64 TARGET=RELEASE INTREE=1 DuetPkg/macbuild.tool
```

*Note*: 32-bit version may not work properly when compiled with older Xcode version (tested 11.4.1).

## Configuration

Builtin available drivers are set in `OpenDuetPkg.fdf` (included drivers) and `OpenDuetPkg.dsc`
(compiled drivers, may not be included). Adding more drivers may result in the need to
change firmware volume size. To do this update `NumBlocks` in `DuetPkg.fdf`
(number of 64 KB blocks in the firmware).

*Note*: OHCI driver is not bundled with DuetPkg (and EDK II) and can be found in
`edk2-platforms/Silicon/Intel/QuarkSocPkg/QuarkSouthCluster/Usb/Ohci/Dxe`.
