# OpenCore Content Flavours

See [Configuration.pdf](Configuration.pdf) for a technical description of the OpenCore content flavour system.

This document provides a list of recommended flavours, so that icon pack artists understand what they must and can do, and so that different icon packs can use consistent names for optional icons.

***Please follow this guide when providing non-default icons in icon packs.***

Please raise a Pull Request or create an Issue to request additional flavours added to the lists below.


## Recommended Icon Flavours

In general, users are recommended to apply the full specified flavour names from the lists below (in **bold**), and icon pack authors are strongly encouraged to create icons with file names from the lists below (in `monotype`)

Icon pack authors are encouraged to provide only those icons for which there is real demand. The complete lists below are provided for consistency of naming, and not because there is a requirement to provide all (or even most, or any - except for the required system icons listed last) of the icons listed here in a given icon pack.

> Note that for all boot entry icon files `<Flavour>.icns` specified below, an `Ext<Flavour>.icns` version may be provided as well. If the selected flavour is specified for an external drive then, if present, `Ext<Flavour>.icns` will be used, otherwise for security reasons OpenCore will use `ExtHardDrive.icns` instead (not `<Flavour>.icns`). Providing `Ext<Flavour>.icns` is optional for icon providers, the fallback behaviour will be perfectly acceptable anyway, in most cases.

## macOS

In the case of macOS only, a flavour based on the detected OS version is applied automatically (as shown below), and the user does not normally need to override this.

For icon pack authors, the **Apple** icon is recommended, **AppleRecovery** and **AppleTM** are suggested, all others are entirely optional.

 - **Apple12:Apple** - Monterey (`Apple12.icns`)
 - **Apple11:Apple** - Big Sur (`Apple11.icns`)
 - **Apple10_15:Apple** - Catalina (`Apple10_15.icns`, etc.)
 - **Apple10_14:Apple** - Mojave
 - **Apple10_13:Apple** - High Sierra
 - **Apple10_12:Apple** - Sierra
 - **Apple10_11:Apple** - El Capitan
 - **Apple10_10:Apple** - Yosemite
 - **Apple10_9:Apple** - Mavericks
 - **Apple10_8:Apple** - Mountain Lion
 - **Apple10_7:Apple** - Lion
 - **Apple10_6:Apple** - Snow Leopard
 - **Apple10_5:Apple** - Leopard
 - **Apple10_4:Apple** - Tiger

In addition, per-OS Recovery variants and Time Machine variants may be supported by adding `Recv` and `TM` to each element of the flavour as shown below.

Recovery flavours also automatically add the OS version:

 - **AppleRecv11:AppleRecv:Apple11:Apple** - macOS Big Sur Recovery (`AppleRecv11.icns`, etc.)
 - ...
 - **AppleRecv10_4:AppleRecv:Apple10_4:Apple** - macOS Tiger Recovery

 Time machine icons _do not_ automatically the add OS version, so per-OS versions would require manual specification by the user (e.g. in `.contentFlavour`) as follows, in order to be found and used:

 - **AppleTM:Apple** - macOS Time Machine, default flavour, not version specific
 - **AppleTM11:AppleTM:Apple11:Apple** - macOS Big Sur Time Machine (`AppleTM11.icns`, etc.)
 - ...
 - **AppleTM10_4:AppleTm:Apple10_4:Apple** - macOS Tiger Time Machine

## Windows

Windows is automatically detected by OpenCore, so the basic `Windows` flavour will be applied automatically. Unlike macOS, Windows versions are not detected automatically, so per-version icons will require manual specification by the user (e.g. in `.contentFlavour`) as follows, in order to be found and used.

**Windows** icon is recommended, all others are optional.

 - **Windows** - Microsoft Windows (auto-detected by OC; fallback: **HardDrive**)
 - **Windows11:Windows** - Microsoft Windows 11 (`Windows11.icns`)
 - **Windows10:Windows** - Microsoft Windows 10 (`Windows10.icns`)
 - **Windows8_1:Windows** - Microsoft Windows 8.1 (`Windows8_1.icns`, etc.)
 - **Windows8:Windows** - Microsoft Windows 8
 - **Windows7:Windows** - Microsoft Windows 7


## Linux

Icon support for Linux is optional. It is not required to provide all flavours even if some are provided; icon pack authors are encouraged to respond to real user demand.

> Note: Media pack authors should understand that it is not required, but optional, to provide `Linux.icns` even if, for example, `Debian.icns` or `Ubuntu.icns` are provided. Provided icons will be used (if selected in flavours specified by the user); icons which are not present will always fall back eventually to OC defaults. (In the case of an OS this will fall back to `HardDrive.icns`.) Similar rules apply in all sections.

Please open an Issue or Pull Request if an additional Linux flavour is required. (Please base this on actual need, e.g. it will affect at least several users.)

 - **Linux** - Base icon for Linux (`Linux.icns`)
 - **Arch:Linux** - Arch Linux (`Arch.icns`, etc.)
 - **Astra:Linux** - Astra Linux
 - **Debian:Linux** - Debian
 - **Deepin:Linux** - Deepin
 - **elementaryOS:Linux** - elementary OS
 - **Endless:Linux** - Endless OS
 - **Gentoo:Linux** - Gentoo Linux
 - **Fedora:Linux** - Fedora
 - **KDEneon:Linux** - KDE neon
 - **Kali:Linux** - Kali Linux
 - **MX:Linux** - MX Linux
 - **Mageia:Linux** - Mageia (fork of former Mandriva)
 - **Manjaro:Linux** - Manjaro
 - **Mint:Linux** - Linux Mint
 - **Oracle:Linux** - Oracle Linux
 - **PopOS:Linux** - Pop!_OS
 - **RHEL:Linux** - Red Hat Enterprise Linux
 - **Solus:Linux** - Solus
 - **Ubuntu:Linux** - Ubuntu
 - **Lubuntu:Ubuntu:Linux** - Lubuntu (`Lubuntu.icns`, etc.)
 - **UbuntuMATE:Ubuntu:Linux** - Ubuntu MATE
 - **Xubuntu:Ubuntu:Linux** - Xubuntu
 - **Zorin:Linux** - Zorin OS
 - **openSUSE:Linux** - openSUSE

## Other Operating Systems

All optional.

Additional sub-flavours are obviously possible for Android, but none are currently defined in this list. Create a Pull Request or Issue if you believe there is a genuine need - e.g. affecting at least several users - to include these.

 - **Android** - Android OS
   - It is down to user preference whether to specify the flavour as **Android:Linux** or just **Android** - depending on whether the user wishes to use `Linux.icns` as a fallback where available, or to fallback directly to **HardDrive** if no `Android.icns` is available
 - **BSD** - Base icon for BSD
 - **FreeBSD:BSD** - FreeBSD
   - Typically at most one of the above two icon files will be provided
 - **ReactOS** - ReactOS (alpha version open-source Windows Server binary compatible OS; not currently UEFI bootable, but reserved here anyway) (`ReactOS.icns`)
   - It is user preference whether to specify **ReactOS:Windows** or just **ReactOS** - depending on whether the user wishes to use `Windows.icns` as a fallback, or to fallback directly to `HardDrive.icns` if no `ReactOS.icns` is available.

## Tools

Create an Issue or Pull Request to request additional tool icons. If doing so please provide a clear case, e.g. showing that it affects at least several users.

### Generic Tool Icon

It is recommended to provide this icon.

 - **Tool** - Any tool entry
   - If provided, is used as fallback for non-OS entries in OC; if not provided falls back again to **HardDrive** (which is required)

### Shell Tools

It is recommended to provide _one_ of these files (more is optional, and not normally required).

If providing just one file, name it `Shell.icns` if the theming of the icon is generic to any shell tool (as in the examples in OcBinaryData). Name it `OpenShell.icns` or `UEFIShell.icns` if the theming is more specific (e.g. includes specific text on the icon).

 - **Shell** - Any shell-style tool
 - **UEFIShell:Shell** - EDK II UEFI Shell
   - As an example of how flavours work: **UEFIShell:Shell** will try `UEFIShell.icns`, then `Shell.icns` (and then, by OC default behaviour, `Tool.icns`, then `HardDrive.icns`)
   - _**NB**: Including **UEFIShell** anywhere in the flavour triggers picker audio-assist for "UEFI Shell"_
 - **OpenShell:UEFIShell:Shell** - Themed specifically for OpenCore OpenShell (which is a variant of the EDK II UEFI Shell)
   - This is the recommended flavour to use for `OpenShell.efi`, as is done in the sample config files
   - Although this is the recommended *flavour*, icon artists are not required to provide this icon file, since this flavour will automatically find and use `Shell.icns` or `UEFIShell.icns` anyway
   - Because **UEFIShell** is included in this flavour, it will also trigger picker audio-assist for "UEFI Shell"
 - **GRUBShell:Shell** - GRUB Shell themed icon

### NVRAM Tools

It is recommended to either i) only provide `NVRAMTool.icns`, as this will be applied to the Reset NVRAM tool and the Toogle SIP tool, or else ii) provide `ResetNVRAM.icns` and `ToggleSip.icns` separately (in which case `NVRAMTools.icns` is not normally required).

If providing `NVRAMTool.icns`, it should be themed so that it could be applied to any generic NVRAM tool.

 - **NVRAMTool** - Any NVRAM-related tool (`NVRAMTool.icns`)
 - **ResetNVRAM:NVRAMTool** - A reset NVRAM tool specifically  (`ResetNVRAM.icns`)
   - This is the recommended flavour, it is automatically used for the entry created by config setting `AllowNvramReset`, or to use for the `CleanNvram.efi` tool.
   - As another example of how flavours work: **ResetNVRAM:NVRAMTool** will look for `ResetNVRAM.icns`, then `NVRAMTool.icns` (and then, by OC default behaviour, `Tool.icns` then `HardDrive.icns`)
   - _**NB**: Including **ResetNVRAM** anywhere in the flavour triggers picker audio-assist for "Reset NVRAM"_
 - **ToggleSIP:NVRAMTool** - Icon themed for Toggle SIP tool specifically  (`ToggleSIP.icns`)

### Other Tools

A list of other known tools which are common enough that some icon pack artists may wish to provide a standard icon for them:

 - **MemTest** - A system memory testing tool such as that available from [memtest86.com](https://www.memtest86.com/) (`MemTest.icns`)

## Bootloaders

Certain well-known bootloaders have also been assigned a flavour:

 - **Boatloader** - Generic bootloader icon (`Bootloader.icns`)
 - **Grub:Bootloader** - Icon for the GRUB2 bootloader (`Grub.icns`)
 - **OpenCore:Bootloader** - OpenCore intentionally does not offer to start instances of itself which have had the OC binary signature applied (i.e. standard release versions), however a) it will show non-signed versions and b) ofc we have to have our own flavour (`OpenCore.icns`)

---

## System Boot Entry Icons

### Required

OpenCanopy will not start if these are not present.

 - **ExtHardDrive** - Bootable OS (on external drive)
 - **HardDrive** - Bootable OS

### Optional

Provided by OcBinaryData. Used automatically by OC in some circumstances, if provided in an icon pack.

 - **AppleRecv** - Apple Recovery (fallback: **HardDrive**)
 - **AppleTM** - Apple Time Machine (fallback: **HardDrive**)
 - **ExtAppleRecv** - Apple Recovery (on external drive) (fallback: **ExtHardDrive**)
 - **ExtAppleTM** - Apple Time Machine (on external drive) (fallback: **ExtHardDrive**)
 - **Shell** - Shell tool (fallback: **Tool**)
 - **Tool** - Generic tool (fallback: **HardDrive**)
 - **Windows** - Microsoft Windows (fallback: **HardDrive**)

### Additional Optional

NOT provided by OcBinaryData. Are used automatically by OC in some circumstances, if provided.

 - **Apple** - macOS (fallback: **HardDrive**)
 - **ExtApple** - macOS (on external drive) (fallback: **ExtHardDrive**)
 - **ExtWindows** - Microsoft Windows (on external drive) (fallback: **ExtHardDrive**)
 - **ResetNVRAM** - Reset NVRAM tool (fallback: **Tool**)

---

## System Icons

Required. OpenCanopy will not start if these are not present.

These icons are not directly related to boot entry flavours, but they are included here to provide all relevant icon pack info in one place.

 - **BtnFocus** - show focus on additional (shutdown and restart) buttons (`BtnFocus.icns`)
 - **Cursor** - mouse cursor (`Cursor.icns`, etc.)
 - **Dot** - password entry hidden character dot
 - **Enter** - password entry enter symbol
 - **Left** - additional picker entries to left
 - **Lock** - password lock
 - **Password** - password entry text area
 - **Restart** - additional button: restart
 - **Right** - additional picker entries to right
 - **Selected** - selected entry background
 - **Selector** - show selected entry
 - **SetDefault** - show selected entry in 'set default' mode
 - **ShutDown** - additional button: shut down

In addition, **Background** (`Background.icns`) is used as the background image for the OpenCanopy boot picker if provided.

