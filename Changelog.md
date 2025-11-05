OpenCore Changelog
==================
#### v1.0.6
- Added workaround for not detected CPU frequency in ProvideCpuInfo quirk, thx @hg13bs
- Updated QemuBuild.command to support `EFI` mode without Duet
- Increased `OC_STORAGE_SAFE_PATH_MAX` to 192 to support various plugin kexts
- Fixed vaulting failures when custom fonts are used, thx @al3xtjames
- Updated documentation for several Booter quirks
- Added `background-color` NVRAM variable to fix 10.9 boot screen
- Fixed debug build of OpenShell not starting on Mac EFI with > 25 file systems
- Improved build speed for Utilities in CI and local build
- Fixed ACPI 1.0 RSDP being reported under ACPI 2.0 GUID with Duet
- Improved logic for RSDP version checking in ACPI patching code
- Downgraded routine log messages such as 'Needs journal recovery, mounting read-only' from WARN to INFO in Ext4Dxe (allows DEBUG_WARN in HaltLevel)
- Improved OpenNtfsDxe stability, thx @stokescat

#### v1.0.5
- Fixed incorrect print in PCI device info dumping in `SysReport`
- Fixed ocvalidate error messages for overlong kext paths in Kernel section, thx @corpnewt
- Fixed kext injection compatibility issues with macOS 26
- Updated builtin firmware versions for SMBIOS and the rest
- Migrated to edk2-stable-202502 

#### v1.0.4
- Added support for booting from static IPv4 address in OpenCore-specific HttpBootDxe
- Added static IPv4 configuration options to OpenNetworkBoot
- Removed `--` prefix from OpenNetworkBoot arguments (modify driver arguments if using this driver)
- Updated `Unload` option to unload drivers in reverse of the order in which they were loaded
- Fixed `MSR_IA32_TSC_ADJUST` access on unsupported CPUs (e.g. Virtualization.framework), thx @t0rr3sp3dr0
- Downgraded WARN log level to INFO for ALREADY_STARTED in AudioDxe (restores ability to include DEBUG_WARN in HaltLevel if required when using this driver)
- Added `ClearTaskSwitchBit` Booter quirk to fix crashes when using 32-bit versions of macOS on Hyper-V Gen2 VMs
- Fixed `ProvideCurrentCpuInfo` and CPUID patching on older versions of macOS 10.4
- Removed ACPI0007 objects from `SSDT-HV-DEV.dsl`
- Removed `SSDT-HV-DEV-WS2022.dsl` as it is no longer required
- Added PCI class names to PCI device info dumping in `SysReport`

#### v1.0.3
- Fixed support for `AMD_CPU_EXT_FAMILY_1AH`, thx @Shaneee
- Fixed EHCI handoff logic in OpenDuet, causing older machines to hang at start
- Added Arrow Lake CPU detection
- Fixed Raptor Lake CPU detection
- Supported booting with TuneD in Fedora 41 in OpenLinuxBoot
- Fixed failure of vault `sign.command` to insert signature in correct location in some circumstances
- Added OpenNetworkBoot driver to support HTTP(S) and PXE boot
- Supported DMG loading and verification (e.g. macOS Recovery) over HTTP(S) boot

#### v1.0.2
- Fixed error in macrecovery when running headless, thx @mkorje
- Added support for `AMD_CPU_EXT_FAMILY_1AH`, thx @Shaneee
- Updated builtin firmware versions for SMBIOS and the rest
- Enabled `XcpmExtraMsrs MSR_MISC_PWR_MGMT` patch back on macOS 12+
- Fixed `XcpmExtraMsrs MSR_MISC_PWR_MGMT` patch on macOS 15
- Added `UEFI` `Unload` option to unload existing firmware drivers
- Fixed boot device selection with VirtIO disk drives used for macOS installations

#### v1.0.1
- Updated code and added progress bar to macrecovery, thx @soyeonswife63
- Bundled fat binary i386/x64 10.6+ compatible `nvramdump` with LogoutHook release
- Added support for manual build of i386/x64 10.6+ versions of userspace tools via `FATBIN32=1 make`
- Disabled `XcpmExtraMsrs MSR_MISC_PWR_MGMT` patch on macOS 12+ due to non-existence
- Fixed `ThirdPartyDrives` quirk on macOS 14.4 and above
- Resolved issue booting recovery for OS X 10.8 and earlier since 0.9.7
- Migrated to edk2-stable202405 

#### v1.0.0
- Updated builtin firmware versions for SMBIOS and the rest
- Switched to Apple silicon GitHub runner for CI, thx @Goooler
- Added Apple Silicon support in all provided utilities
- Utilities now require macOS 10.9+ (OpenCore itself still supports macOS 10.4+)
- Added `AllowRelocationBlock` support for 32-bit version
- Enabled additional serial logging in non-RELEASE builds of OpenDuet
- Added missing DxeCore ImageContext HOB in OpenDuet
- Fixed assert caused by dependency ordering in OpenDuet
- Prevented assert in normal situation when freeing memory above 4GB in OpenDuet
- Prevented debug assert reporting that optional Hii protocols are not present in OpenDuet
- Fixed problem loading non-firmware runtime drivers (e.g. OpenRuntime.efi) in OpenDuet
- Resolved issue using NOOPT debugging in OpenDuet
- Fixed alphabetical ordering in Configuration.pdf, thx @leon9078

#### v0.9.9
- Fixed incorrect warning in ocvalidate
- Modified `Launchd.command` to recreate its log file if deleted
- Updated `Launchd.command` to work with macOS Sonoma (re-run `./Launchd.command install` after upgrading to Sonoma)
- Fixed an incorrectly labelled MacBookPro11,3 model code in `macserial`, thx @Macschrauber
- Improved macrecovery download logic for slow connections to get chunklist first, thx @scriptod911

#### v0.9.8
- Updated OpenDuet to allow loading unsigned, unaligned legacy Apple images such as HfsPlusLegacy.efi
- Fixed CPU frequency calculation on AMD 10h family
- Swapped the position of Shutdown and Restart buttons to better match recent macOS
- Added `OC_ATTR_USE_REVERSED_UI` to allow access to previous default Shutdown and Restart button arrangement
- Fixed intro animation getting stuck in OpenCanopy if an entry which returns to menu is selected before animation ends
- Modified OpenCanopy to require presence of label images only when used due to `OC_ATTR_USE_GENERIC_LABEL_IMAGE`
- Provided `OC_ATTR_REDUCE_MOTION` to optionally disable non-required OpenCanopy menu animations
- Modified NVRAM logout hook to handle XML entities in string vars
- Fixed CPU frequency calculation on AMD 0Fh family
- Added kext blocker `Exclude` strategy for mkext
- Re-enabled AudioDxe failover to protocol GET mode for systems such as Acer E5 where it works when DisconnectHda doesn't
- Added `FirmwareSettingsEntry.efi` driver which adds menu entry to reboot into UEFI firmware settings
- Enabled use of picker shortcut keys which are read out in OpenCanopy when using `PickerAudioAssist`
- Modified builtin picker so as not to respond to keys queued while audio assist menu is being read out
- Fixed Linux EFI stub loading error when using OpenDuet since 0.8.8
- Fixed APFS JumpStart with OpenDuet and `SecureBootModel` `Disabled`
- Added TSC frequency calculation for xen hypervisor, thx @netanelc305
- Supported additional early Nvidia UEFI VBIOS in `EnableGop` `vBiosInsert.sh`

#### v0.9.7
- Updated recovery_urls.txt
- Changed OpenDuet to enforce `W^X` settings rather than fixing them in loaded images
- Updated `FixupAppleEfiImages` quirk to fix `W^X` errors in all non-Secure Boot Apple signed binaries
- Updated builtin firmware versions for SMBIOS and the rest
- Updated `AppleEfiSignTool` to work with new PE COFF loader
- Fixed recovery failing to boot on some systems
- Updated `ProvideCurrentCpuInfo` quirk to support CPUID leaf 0x2 cache size reporting on Mac OS X 10.5 and 10.6
- Updated `efidebug.tool` to support new standard image format

#### v0.9.6
- Updated builtin firmware versions for SMBIOS and the rest
- Fixed hang while generating boot entries on some systems
- Added `efidebug.tool` support for 32-bit on 32-bit using GDB or LLDB
- Fixed potential incorrect values in kernel image capabilities calculation
- Added `FixupAppleEfiImages` quirk to allow booting Mac OS X 10.4 and 10.5 boot.efi images on modern secure image loaders

#### v0.9.5
- Fixed GUID formatting for legacy NVRAM saving
- Fixed inability to open files in root directory on an NTFS filesystem
- Fixed hang while unloading NTFS driver
- Added UEFI quirk `ShimRetainProtocol`, allowing OpenCore chained from shim to verify Linux using shim's certificates
- Added `OpenLegacyBoot` driver for supporting legacy OS booting
- Added `shim-make.tool` to download and build rhboot/shim, for Linux SBAT and MOK integration

#### v0.9.4
- Fixed kext blocker `Exclude` strategy for prelinked on 32-bit versions of macOS
- Fixed `ForceAquantiaEthernet` quirk on macOS 14 beta 2, thx @Shikumo
- Added `InstanceIdentifier` to OpenCore and option to target `.contentVisibility` to specific instances (thx @dakanji)
- Improved `LapicKernelPanic` quirk on legacy versions of macOS
- Allowed `.contentVisibility` in same boot FS root locations as `.VolumeIcon.icns`, in order to survive macOS updates
- Fixed incorrect core count on Silvermont Atom/Celeron processors
- Fixed PM timer detection on Silvermont Atom/Celeron processors for TSC calculations
- Fixed PM timer detection on non-Intel chipsets when booted through OpenDuet
- Fixed `FadtEnableReset` on NVIDIA nForce chipset platforms
- Added BlockIoDxe alternative OpenDuet variant
- Added support for ATI cards when using `ForceResolution` option

#### v0.9.3
- Added `--force-codec` option to AudioDxe, thx @xCuri0
- Downgraded additional warning message in normal operation of emulated NVRAM to info
- Disabled not present DVL0 device in SSDT-SBUS-MCHC by default, thx @stevezhengshiqi
- Added EFI mandated box drawing, block element and arrow characters to `Builtin` renderer console font
- Improved support for overlong menu entries and very narrow console modes in builtin picker
- Made `Builtin` text renderer ignore UI Scale, when required to ensure that text mode reaches minimum UEFI supported size of 80x25
- Added save and restore of text and graphics mode round tools and failed boot entries
- Updated out-of-range cursor handling to work round minor display issue in memtest86
- Added optional `--enable-mouse-click` argument to `CrScreenshotDxe` driver to additionally respond on mouse click
- Added `--use-conn-none` option to `AudioDxe` driver to discover additional usable output channels on some systems
- Added `PciIo` protocol override used to fix Aptio IV compatiblity with Above 4G BARs, thx @xCuri0
- Fixed `AppleXcpmForceBoost` quirk on macOS 14
- Updated builtin firmware versions for SMBIOS and the rest
- Added `ConsoleFont` option to load custom console font for `Builtin` renderer
- Improved `XhciPortLimit` quirk on macOS 11 to 14

#### v0.9.2
- Added `DisableIoMapperMapping` quirk, thx @CaseySJ
- Fixed disabling single user mode when Apple Secure Boot is enabled
- Improved guard checks for `GopBurstMode` on systems where it's not needed
- Improved compatibility of `GopBurstMode` with some very non-standard GOP implementations
- Fixed possible hang with `GopBurstMode` enabled on DEBUG builds
- Enabled `GopBurstMode` even with natively supported cards, in EnableGop firmware driver
- Fixed inability to patch force-injected kexts
- Fixed `ExternalDiskIcons` quirk on macOS 13.3+, thx @fusion71au
- Fixed various recent reversions and some longer-standing minor bugs in `Builtin` text renderer
- Applied some additional minor optimizations to `Builtin` text renderer
- Implemented `InitialMode` option to allow fine control over text renderer operating mode
- Added support for `ConsoleMode` text resolution setting to `Builtin` renderer
- Fixed regression for ACPI quirks `RebaseRegions` and `SyncTableIds`
- Updated build process to provide stable and bleeding-edge versions of `EnableGop`
- Implemented minor improvements in `PickerMode` `Apple`
- Improved filtering algorithm for `LogModules` and added `?` filter for matching non-standard log lines
- Fixed crash when gathering system report on virtualised CPUs
- Fixed unnecessary warning when first booting with emulated NVRAM
- Enabled `AppleCpuPmCfgLock` quirk on macOS 13

#### v0.9.1
- Fixed long comment printing for ACPI patches, thx @corpnewt
- Added sample config for VS Code source level debugging with `gdb`
- Updated builtin firmware versions for SMBIOS and the rest
- Added GOP memory caching report to `SysReport`
- Implemented `GopBurstMode` quirk for faster GOP operation on older firmware
- Fixed `ThirdPartyDrives` quirk on macOS 13.3 and above

#### v0.9.0
- Resolved issues with verbose boot log appearing over picker graphics
- Added version number to EnableGop UI section, so tool builders can track it
- Added `ProvideCurrentCpuInfo` support for macOS 13.3 DP
- Added AMD support, GOP offset auto-detection and macOS 10.11+ support to EnableGop vBIOS insertion script
- Included precompiled EDK-II `EfiRom` and `GenFfs` in `Utilities/BaseTools` with OpenCore releases

#### v0.8.9
- Improved debug logging when applying ACPI patches
- Fixed loading macOS with legacy boot without Apple Secure Boot
- Added Linux support to legacy boot BootInstall script
- Updated builtin firmware versions for SMBIOS and the rest
- Fixed incomplete console mode initialisation when started in graphics mode
- Provided additional UEFI forge mode, for use in firmware drivers
- Implemented firmware driver enabling pre-OpenCore graphics on non-natively supported GPUs on EFI-era Macs
- Added `ResizeUsePciRbIo` quirk to workaround broken PciIo on some UEFI firmwares, thx @xCuri0
- Prevented unwanted clear screen to console background colour when in graphics mode
- Fixed crash while using `SysReport` on older Atom systems
- Fixed kexts without a Contents folder not being patched during a cacheless boot
- Added read-only sections (`.rdata`) to all drivers for better memory protection when supported
- Fixed crash while using `SysReport` on systems with non-audio HDA codecs
- Fixed debug script support for GDB and LLDB
- Fixed legacy boot debug builds asserting on macOS loading

#### v0.8.8
- Updated underlying EDK II package to edk2-stable202211
- Updated AppleKeyboardLayouts.txt from macOS 13.1
- Updated builtin firmware versions for SMBIOS and the rest
- Updated ocvalidate to allow duplicate tool if FullNvramAccess is different
- Fixed `Kernel` -> `Block` entries not being processed if one was skipped due to `Arch`
- Fixed intermittent prelinking failures caused by XML corruption when kext blocking is enabled
- Removed magic Acidanthera sequence from OpenCore files used for picker hiding
- Added `.contentVisibility` to hide and disable boot entries
- Added Linux support to QemuBuild.command used for Duet debugging
- Built in new secure PE/COFF loader
- Added prebuilt mtoc universal binary with Apple Silicon support
- Corrected OpenDuet build on Apple Silicon
- Added SD card device path support for boot device selection

#### v0.8.7
- Removed unwanted clear screen when launching non-text boot entry
- Fixed TSC/FSB for AMD CPUs in ProvideCurrentCpuInfo, thx @Shaneee
- Added `Misc` -> `Boot` -> `HibernateSkipsPicker` not to show picker if waking from macOS hibernation
- Changed macrecovery to download files into `com.apple.recovery.boot` by default, thx @dreamwhite
- Supported Apple native picker (using `BootKicker.efi` or `PickerMode` `Apple`) when running GPUs without Mac-EFI support on units such as the MacPro5,1 (thx @cdf, @tsialex)
- Enabled `PickerMode` `Apple` to successfully launch selected entry
- Enabled `BootKicker.efi` to successfully launch selected entry (via reboot) (thx @cdf)
- Added spoof proof UEFI 2.x checking to OpenVariableRuntimeDxe, thx @dakanji

#### v0.8.6
- Updated NVRAM save script for compatibilty with earlier macOS (Snow Leopard+ tested)
- Updated NVRAM save script to automatically install as launch daemon (Yosemite+) or logout hook (older macOS)
- Fixed maximum click duration and double click speed for non-standard poll frequencies
- Added support for pointer dwell-clicking
- Fixed recursive loop crash at first non-early log line on some systems
- Fixed early log preservation when using unsafe fast file logging
- Updated builtin firmware versions for SMBIOS and the rest
- Resolved wake-from-sleep failure on EFI 1.1 systems (including earlier Macs) with standalone emulated NVRAM driver
- Updated macrecovery commands with macOS 12 and 13, thx @Core-i99
- Updates SSDT-BRG0 with macOS-specific STA to avoid compatibility issues on Windows, thx @Lorys89
- Fixed memory issues in OpenLinuxBoot causing crashes on 32-bit UEFI firmware

#### v0.8.5
- Updated builtin firmware versions for SMBIOS and the rest
- Moved CPU objects that exist only in Windows Server 2022 into `SSDT-HV-DEV-WS2022.dsl`
- Updated Hyper-V device path expansion to support hot add/remove of disks
- Improved verbose logging during kernel patching

#### v0.8.4
- Added checks for `Driver` -> `LoadEarly` in ocvalidate
- Added `FullNvramAccess` option for tools which require direct access to NVRAM
- Replaced `SSDT-HV-CPU.dsl` with `SSDT-HV-DEV.dsl` for compatiblity with older macOS versions on Windows 10 and newer
- Updated builtin zlib library to 1.2.12
- Changed ocpasswordgen not to print characters on password input
- Added ProcessKernel utility for testing kext injection based on configs
- Fixed crash while using `SysReport` on Pentium 4 systems
- Fixed crash after ExitBootServices() is called while using DEBUG builds and file logging
- Fixed 32-bit userspace build support on macOS (use High Sierra 10.13 and below)
- Added basic set of NetworkPkg drivers with HTTP boot support

#### v0.8.3
- Added ext4 file system driver
- Added support for macOS 13 DP3 Kernel Collection
- Added `--force-device` option to AudioDxe, allowing UEFI audio on HDA contollers which misreport themselves as non-HDA audio devices
- Provided optional unsafe fast file logging (suitable only for firmware with a fully compliant FAT32 driver)
- Fixed incorrect OSBundleLibraries_x86_64 handling during cacheless injection
- Changed RsaTool not to link against system ssl on macOS
- Fixed crash during cacheless injection when kext blocking is enabled
- Removed default codec connection delay from AudioDxe
- Added optional `--codec-setup-delay` argument to AudioDxe
- Changed units of `Audio` -> `SetupDelay` from microseconds to milliseconds (divide previous value by 1000 if using this setting)
- Fixed incorrect FAT binary slice being selected under macOS 10.4.11 when performing a cacheless boot
- Fixed rare assertion caused by label animation initialisation in OpenCanopy
- Added `--show-csr` option for `Toggle SIP` boot menu entry
- Added macOS 10.4 and 10.5 support to `AllowRelocationBlock` Booter quirk
- Added CPU cache info injection for macOS 10.4 to `ProvideCurrentCpuInfo` quirk
- Added emulated NVRAM driver for use separately from OpenDuet
- Added support for NVRAM reset and set default boot entry when using emulated NVRAM
- Upgraded emulated NVRAM logout script to allow unsupervised installation of recent macOS OTA updates
- Added `Driver` -> `LoadEarly` for drivers which need to be loaded before NVRAM init

#### v0.8.2
- Fixed `AppleCpuPmCfgLock` on macOS 13
- Fixed `DummyPowerManagement` on macOS 13
- Updated builtin firmware versions for SMBIOS and the rest
- Added macOS 13 support for `AvoidRuntimeDefrag` Booter quirk
- Added injected kext bundle version printing in DEBUG builds
- Added Linux compatibility for CreateVault scripts

#### v0.8.1
- Improved `ExtendBTFeatureFlags` quirk on newer macOS versions, thx @lvs1974
- Added notes about DMAR table and `ForceAquantiaEthernet`, thx @kokowski
- Added System option in `LauncherOption` property, thx @stevezhengshiqi
- Updated note about `CustomPciSerialDevice`, thx @joevt
- Added read-only driver for NTFS
- Switched `Reset NVRAM` and `Toggle SIP` to configurable boot entry protocol drivers
- Supported optional Apple firmware-native NVRAM reset, thx @Syncretic
- Supported NVRAM reset optionally retaining BIOS boot entries
- Supported user specified `csr-active-config` value for Toggle SIP
- Added optional `Enabled` and `Disabled` flavours for `Toggle SIP` (allows theme designers to provide distinct icons)
- Added PIIX4 ACPI PM timer detection for TSC calculations on Hyper-V Gen1 VMs

#### v0.8.0
- Added support for early log preservation
- Switched to Python 3 in scripts (use `python /path/to/script` to force Python 2)
- Added `ForceAquantiaEthernet` for Aquantia AQtion AQC-107s based 10GbE network cards support, thx @Mieze and @Shikumo
- Updated builtin firmware versions for SMBIOS and the rest
- Added `Misc` -> `Serial` section to customise serial port properties
- Added `CustomPciSerialDevice` quirk for XNU to correctly recognise customised external serial devices

#### v0.7.9
- Added auto-detect `macOS Installer` volume name for use when `.disk_label` file cannot be displayed
- Added `--restore-nosnoop` flag to AudioDxe, making v0.7.7 fix for Windows sound opt-in
- Added new method to disable trim when `SetApfsTrimTimeout` is set to zero
- Fixed `SetApfsTrimTimeout` on macOS 12 (only works when set to zero)
- Added script to build qemu recovery images to macrecovery
- Fixed selecting `SecureBootModel` on hypervisors (should be `x86legacy`)
- Added kext blocking `Strategy` for prelinked and newer
- Added global MSR 35h fix to `ProvideCurrentCpuInfo`, allowing `-cpu host` in KVM
- Fixed potential memory corruption with AVX acceleration enabled
- Added `LogModules` for positive and negative log filtering by modules
- Renamed OpenLinuxBoot driver argument from `partuuidopts:{PARTUUID}` to `autoopts:{PARTUUID}`
- Supported booting Linux from stand-alone `/boot` partition without `/loader/entries` files (user must specify full kernel boot options)
- Handled XML entities in driver arguments
- Updated underlying EDK II package to edk2-stable202202

#### v0.7.8
- Updated ocvalidate to warn about insecure `DmgLoading` with secure `SecureBootModel` (already disallowed in runtime)
- Fixed AudioDxe not disabling unused channels after recent updates
- Allow gain to track OS volume on old macOS without `SystemAudioVolumeDB`
- Fixed crash on no mouse support when verifying password
- Fixed AppleInternal CSR bit being set with `ProvideCustomSlide` enabled
- Added support for `.contentFlavour` and `.contentDetails` files for boot entry protocol entries including OpenLinuxBoot
- Added `LINUX_BOOT_ADD_RW` flag to OpenLinuxBoot to support e.g. EndeavourOS
- Added `flags+=` and `flags-=` arguments to OpenLinuxBoot to simplify setting driver flags if needed
- Fixed OpenLinuxBoot entry name disambiguation when `LINUX_BOOT_USE_LATEST` flag is clear
- Updated builtin firmware versions for SMBIOS and the rest
- Fixed crash in OpenLinuxBoot with partly (re-)installed Linux distro
- Improved robustness in malformed PE image file parsing

#### v0.7.7
- Fixed rare crash caused by register corruption in the entry point
- Added `ProvideCurrentCpuInfo` support for Intel Alder Lake
- Fixed typo in `Cpuid1Data` recommendations for Intel Rocket Lake and newer
- Updated builtin firmware versions for SMBIOS and the rest
- Updated underlying EDK II package to edk2-stable202111
- Resolved crashes in QEMU with AudioDxe
- Added AudioDxe settings caching (avoids non-needed setup delays)
- Added DisconnectHda quirk to allow UEFI sound on Apple hardware and others
- Added workarounds for bugs in QEMU `intel-hda` driver to allow UEFI sound in QEMU
- Implemented multi-channel (e.g. bass+main speaker; speakers+headphones) UEFI sound with `AudioOutMask`
- Fixed AudioDxe startup stalls when Nvidia HDA audio is present
- Resolved AudioDxe disabling sound in Windows on some firmware
- Added pointer polling period tuning in the builtin AppleEvent implementation
- Added pointer device list tuning in the builtin AppleEvent implementation
- Added VREF handling to support UEFI sound on more Apple hardware
- Updated audio output channel detection to support UEFI sound on more Apple hardware
- Added manual GPIO config (use `--gpio-setup` AudioDxe driver argument for UEFI sound on Apple hardware)
- Switched UEFI audio levels to decibel gain to allow accurate matching of saved macOS volume levels
- Separated settings for minimum audio assist volume and minimum audible volume

#### v0.7.6
- Fixed stack canary support when compiling with GCC
- Added automatic scaling factor detection
- Explicitly restricted `ResizeAppleGpuBars` to 0 and -1
- Fixed OpenCanopy long labels fade-out over graphics background
- Fixed `ProvideConsoleGop` not disabling blit-only modes (e.g. on Z690)
- Fixed Alder Lake SMBIOS CPU model information
- Added XCPM CPU power management ACPI table for Intel Alder Lake
- Updated draw order to avoid graphics tearing in OpenCanopy
- Fixed handling PCI device paths with logical units in ScanPolicy
- Added `ReconnectGraphicsOnConnect` option for enabling alternative UEFI graphics drivers
- Added BiosVideo.efi driver to use with `ReconnectGraphicsOnConnect`
- Changed `FadtEnableReset` to avoid unreliable keyboard controller reset
- Added `EnableVmx` quirk to allow virtualization in other OS on some Macs
- Upgraded `ProtectUefiServices` to prevent GRUB shim overwriting service pointers when chainloading with Secure Boot enabled
- Removed deprecated SSDT-PNLFCFL
- Fixed handling of zero-sized Memory Attributes Table

#### v0.7.5
- Revised OpenLinuxBoot documentation
- Supported Linux ostree boot layout
- Fixed external drive icons for Boot Entry Protocol
- Added GPU Resize BAR quirks to reduce BARs on per-OS basis
- Fixed OpenLinuxBoot hang bug after correct detection of some distros
- Added DMG signature check during download, thx @jspraul and @zhangyoufu
- Updated builtin firmware versions for SMBIOS and the rest
- Updated recovery downloading commands to include macOS 11 and 12

#### v0.7.4
- Fixed Linux kernel sort order
- Added Linux detection optional log detail
- Fixed CPU core count detection for more legacy CPUs
- Added ability to fully override autodetect Linux boot options
- Added large BaseSystem support in `AdviseFeatures`
- Updated builtin firmware versions for SMBIOS and the rest
- Added tool to extract vendor secure boot certificate from GRUB shim file
- Added `BridgeOSHardwareModel` NVRAM variable to fix T2 SB AP models on macOS 12
- Changed `Default` Apple Secure Boot model to match SMBIOS for macOS 12
- Fixed `opencore-version` not being added to NVRAM variables

#### v0.7.3
- Improved SSDT-PNLF compatibility with CFL+ graphics
- Fixed OpenCanopy performance loss due to redrawing introduced in 0.6.9
- Added pattern-based automatic variable initialisation for better security
- Updated underlying EDK II package to edk2-stable202108
- Updated Apple Secure Boot variables for `x86legacy`
- Updated Linux variants in Flavours.md
- Implemented Boot Entry Protocol, allowing plug-in boot entry drivers
- Added StringBuffer and FlexArray libraries
- Updated Drivers to support arguments (requires config.plist update, see samples)
- Added OpenLinuxBoot driver: OC-native Linux autodetect and boot without chaining via GRUB
- Fixed overlong boot entry names breaking text flow in builtin menu
- Added `ForceOcWriteFlash` UEFI quirk to enable writing OC system variables

#### v0.7.2
- Fixed OSBundleLibraries/OSBundleLibaries64 handling
- Added `GraphicsInputMirroring` to fix lost keystrokes in some non-Apple graphical UEFI apps
- Added support for stack canaries (security cookies / stack guards)
- Fixed unintialised memory access in AudioDxe causing audio playback failure
- Changed `Default` Apple Secure Boot model to `x86legacy` for better security and compatibility
- Increased default APFS `MinDate` and `MinVersion` to macOS Big Sur for better security
- Updated builtin firmware versions for SMBIOS and the rest
- Improved SSDT-PNLF compatibility with Windows and newer graphics
- Fixed CLANGPDB OpenCore builds by shortening OC magic

#### v0.7.1
- Added `SyncTableIds` quirk to sync modified table OEM identifiers
- Added CPU Info (MSRs) dumping to `SysReport`
- Updated builtin firmware versions for SMBIOS and the rest
- Fixed `PowerTimeoutKernelPanic` on macOS 12
- Fixed transparency click detection on OpenCanopy boot entries
- Added PCI device info dumping to `SysReport`
- Fixed `SetApfsTrimTimeout` on macOS 12
- Documented requirement for SetDefault.icns width to match Selector.icns width
- Added explicit warn and safe fallback to builtin picker on failure to match the above
- Added VSCode source level IDE debug config example to debug docs
- Added other minor debug docs updates
- Fixed incorrect timeout of built-in picker on IA32
- Added support for custom kernels on ESP partition
- Fixed DEBUG ASSERT on pressing change entry keys with single boot entry in OpenCanopy
- Added recommended `Apple12` and `Windows11` flavours
- Added `TpmInfo` tool to DEBUG TPM status
- Fixed incorrect OpenCanopy initial display when default entry beyond right of screen
- Fixed `ProvideCurrentCpuInfo` MSR patch on macOS 12
- Fixed `AppleXcpmForceBoost` patch on macOS 12

#### v0.7.0
- Fixed NVRAM reset on firmware with write-protected `BootOptionSupport`
- Improved direct GOP renderer performance for certain cases
- Added support for display rotation in direct GOP renderer
- Fixed handling multinode device paths in LoadedImage and elsewhere
- Changed OpenCanopy image directory to support directory prefixes
- Changed OpenCanopy preferred image set to `Acidanthera\GoldenGate`
- Removed `<BOOTPATH>.icns` and `<TOOLPATH>.icns` support
- Added content flavour system allowing custom boot entry icons compatible across icon packs
- Added automatic flavour detection for macOS boot entries
- Added `ProvideCurrentCpuInfo` quirk to provide correct TSC/FSB for Hyper-V virtual machines
- Added Hyper-V device path expansion to allow setting default boot volume
- Added `Apple` variant of `GopPassThrough` to handle only `AppleFramebufferInfo` handles
- Fixed further kernel patches not being processed if a patch was skipped due to arch mismatch
- Added optional Toggle SIP system boot menu option
- Added `CsrUtil.efi` tool, similar to Apple `csrutil`
- Removed support for `<TOOLPATH>.lbl`/`.l2x` pre-drawn entry labels
- Fixed previous text not cleared before console mode tools and entries in OpenCanopy
- Fixed DEBUG build crashes with `GopPassThrough` and `UgaPassThrough`
- Added flavour for memory testing utilities
- Updated recommended `memtest86` config in sample `.plist` files
- Defined bootloader flavours
- Applied own flavour to OC build
- Added CPU topology fixes to `ProvideCurrentCpuInfo` quirk
- Updated OC default SIP disabled value
- Documented SIP values which affect macOS updates
- Added `csr-data` Apple NVRAM var to docs
- Fixed file alignment causing codesign issues with CLANGPDB images
- Replaced `AdviseWindows` with `AdviseFeatures` to support APFS

#### v0.6.9
- Fixed out-of-sync cursor movement rectangle when loading e.g. CrScreenshotDxe
- Updated underlying EDK II package to edk2-stable202102
- Applied consistent enforcement of required minimum Apple OEM Apple Event protocol version
- Changed CustomDelays to less surprising boolean setting with failsafe of false
- Changed key repeat failsafes and sample values to Apple OEM values
- Changed PointerSpeedMul failsafe to Apple OEM value
- Updated docs to include configuration of key repeat settings with and without KeySupport
- Prevented 'set default' UI when action not permitted by security config
- Added `ForgeUefiSupport` quirk to workaround legacy EFI 1.x firmwares compatibility
- Added `ReloadOptionRoms` quirk to force-load Option ROMs on PCI devices
- Added `OC_ATTR_USE_MINIMAL_UI` to allow running pickers with no Shutdown and Restart buttons
- Added display of OpenCore version number to OpenCanopy as well as builtin picker, depending on existing ExposeSensitiveData bit
- Added support for case-insensitive argument handling in the UEFI tools
- Added vector acceleration of SHA-512 and SHA-384 hashing algorithms, thx @MikhailKrichanov
- Fixed wraparound when using arrow keys in OpenCanopy
- Updated builtin firmware versions for SMBIOS and the rest
- Added bundled Linux versions for userspace utilities
- Fixed fallback SMBIOS `Manufacturer` value to `NO DIMM` for empty slots
- Fixed assertions when running OpenCanopy with low resolution, will fallbacks to builtin now

#### v0.6.8
- Switched to VS2019 toolchain for Windows builds
- Reduced legacy boot install interaction effort
- Increased OpenCanopy rendering performance
- Added OpenCanopy Shut Down and Restart buttons
- Reduced OpenCanopy mouse pointer input lag
- Fixed that cursor bounds could be different from OpenCanopy's
- Improved builtin picker rendering performance
- Added Memory Type decoding for SMBIOS in `Automatic` mode
- Properly support setting custom entries as default boot options
- Fixed creating log file when root file system is not writable
- Fixed `DisableSingleUser` not being enabled in certain cases
- Added `ForceBooterSignature` quirk for Mac EFI firmware
- Fixed OpenCanopy sometimes cutting off shown boot entries
- Further improved CPU frequency calculation on legacy CPUs
- Fixed SMBIOS SMC version encoding sequence
- Added TSC frequency reading from Apple Platform Info
- Added TSC frequency reading for Apple devices with nForce chipsets
- Added `Base` and `BaseSkip` lookup for ACPI patches
- Fixed ACPI table magic corruption during patching
- Fixed unnatural OpenCanopy and FileVault 2 cursor movement
- Fixed OpenCanopy interrupt handling causing missed events and lag
- Improved OpenCanopy double-click detection
- Reduced OpenCanopy touch input lag and improved usability
- Improved keypress responsiveness in OpenCanopy and builtin pickers
- Improved non-repeating key detection in OpenCanopy and builtin pickers
- Fixed Escape preventing OpenCanopy fade up until released, on some systems
- Fixed fast repeat then stall issue with key handling on some PS/2 systems
- Added accurate Shift+Enter/Shift+Index detection when using PollAppleHotKeys
- Added 'set default' indicator to builtin picker
- Replaced VerifyMsrE2 with ControlMsrE2 also allowing unlock on some firmwares
- Fixed OpenCanopy flicker when refreshing the entry view
- Added OpenCanopy TAB navigation support
- Added OpenCanopy graphical password interface
- Added OpenCanopy pulsing animation to signal timeout
- Added OpenCanopy 'set default' indicator
- Fixed OpenCanopy not aborting timeout on pointer click
- Fixed OpenCanopy intro animation not scaling with UIScale
- Add OpenCanopy boot entry label scrolling (fixes missing long labels)
- Added tabbable Shutdown and Restart buttons to builtin picker
- Fixed in-firmware shutdown for some systems running OpenDuet
- Added Zero as alias hotkey for Escape, to force show picker if hidden
- Added =/+ key as alias for CTRL to set default OS
- Added additional support for configuring correct key repeat behaviour with KeySupport mode
- Fixed CPU multiplier detection on pre-Nehalem Intel CPUs
- Fixed incorrect handling of multiple processors and processor cache in SMBIOS
- Matched default Apple boot picker cursor start position
- Updated OpenShell `devices` command to support misaligned device names returned by some Apple firmware
- Added `(dmg)` suffix to DMG boot options in OpenCanopy
- Added identifiers for Rocket Lake and Tiger Lake CPUs
- Added PickerAudioAssist 'disk image' indication
- Fixed PickerAudioAssist indications played twice in rare cases
- Improved OpenCanopy pointer acceleration
- Added more precise control on `AppleEvent` protocol properties and features
- Added dynamic keyboard protocol installation on CrScreenshotDxe
- Support starting UEFI tools with argument support (e.g. `ControlMsrE2`) without arguments from picker
- Fixed OpenCanopy font height calculation, may reject previously working fonts and mitigate memory corruption
- Fixed incorrect identification of Xeon E5XXX/E5-XXXX and Xeon WXXXX/W-XXXX CPUs
- Added RSDP, RSDT, and XSDT handling to `NormalizeHeaders` ACPI quirk

#### v0.6.7
- Fixed ocvalidate return code to be non-zero when issues are found
- Added `OEM` values to `PlatformInfo` in `Automatic` mode
- Improved CPU frequency calculation on Haswell and earlier
- Fixed issues when applying certain patches
- Added `SSN` (and `HW_SSN`) variable support
- Added onscreen early logging in DEBUG builds for legacy firmware
- Added workaround for firmware not specifying DeviceHandle at bootstrap
- Added support for R/O page tables in `SetupVirtualMap` quirk
- Added OEM preservation for certain Apple SMBIOS tables
- Fixed switching to graphics mode when entering OpenCanopy
- Fixed installing Apple FB Info protocol when no GOP exists
- Fixed abort timeout sound in OpenCanopy on key press
- Added `GopPassThrough` option to support GOP protocol over UGA
- Fixed CPU speed rounding for certain Xeon and Core 2 CPUs
- Removed `KeyMergeThreshold` as it never functioned anyway
- Added `acdtinfo` utility to lookup certain products
- Fixed `FSBFrequency` calculation with fractional multiplier
- Fixed showing core count for some AMD CPUs
- Added `ResetTrafficClass` to reset TCSEL to T0 on legacy HDA
- Fixed default boot entry selection without timeout for builtin picker
- Added ocpasswordgen utility to generate OpenCore password data
- Added `ActivateHpetSupport` quirk to activate HPET support
- Fixed `opencore-version` reporting the incorrect version in rare cases

#### v0.6.6
- Added keyboard and pointer entry scroll support in OpenCanopy
- Added background image support in OpenCanopy
- Fixed selector boot option choice in OpenCanopy
- Relaxed selector dimensions for OpenCanopy
- Added `MaxBIOSVersion` option to `Generic`
- Fixed MLB verification feature in macrecovery
- Replaced `VBoxHfs` driver with `OpenHfsPlus`
- Added audio codec dumping to `SysReport`
- Fixed compatibility with page protection for all binaries
- Fixed crashes in OpenUsbKbDxe when handling unsupported devices
- Removed `HdaCodecDump` application in favor of `SysReport`
- Added `SetApfsTrimTimeout` to tune APFS trim command
- Changed `OpenCore.efi` to application to improve FW compatibility
- Added `DisableSecurityPolicy` UEFI quirk to workaround driver loading
- Added support for ranged widget connections in AudioDxe
- Fixed supplying non-RT `SetVirtualAddressMap` for non-macOS systems
- Fixed using `SystemUuid` from `DataHub` in non-Automatic mode for `SMBIOS`
- Dropped failsafe defaults from `Generic` to match non-Automatic mode
- Replaced `BootProtect` with `LauncherOption` and `LauncherPath`
- Added `OpenPartitionDxe` with Apple Partition Management scheme
- Improved ocvalidate checks in `Misc`, `NVRAM`, and `UEFI` sections
- Fixed multiple flaws in EFI image loading, APFS driver in particular
- Fixed NVRAM `system-id` being accidentally stored in Little Endian format
- Added `UseRawUuidEncoding` to choose SMBIOS UUID encoding style
- Updated builtin firmware versions for SMBIOS and the rest

#### v0.6.5
- Fixed installing OpenDuet on protected volumes
- Updated underlying EDK II package to edk2-stable202011
- Updated builtin firmware versions for SMBIOS and the rest
- Fixed macrecovery server protocol compatibility
- Added basic audio assistant support in OpenCanopy
- Added compiled ACPI samples to the package
- Fixed timer resolution restoration at boot time
- Fixed memory capacity when using custom SMBIOS memory config
- Removed no longer required `DeduplicateBootOrder` quirk
- Fixed macserial crashes when processing invalid serials
- Fixed macserial issues when processing 2021 year serials
- Added advanced error checking in ocvalidate utility
- Added `SetupDelay` to configure audio setup delay
- Reworked LogoutHook.command to support older macOS
- Implemented MP3 audio decoding for audio assistant support
- Added support for `PickerVariant` for more theme variants
- Added `OC_ATTR_HIDE_THEMED_ICONS` `PickerAttribute` for Time Machine
- Fixed OpenUsbKb compatibility with certain keyboards

#### v0.6.4
- Added `BlacklistAppleUpdate` to fix macOS 11 broken update optout
- Dropped HII services from OpenDuet improving size and performance
- Fixed patching of injected kexts in mkext
- Added support for launching from relative paths
- Added direct path passing for tools via `RealPath`
- Allowed launching tools and entries in text mode via `TextMode`
- Updated builtin firmware versions for SMBIOS and the rest
- Fixed ACPI patches not applying if tables are in locked memory
- Fixed `EnableSafeModeSlide` on macOS 11
- Added `AllowRelocationBlock` quirk for older macOS and safe mode
- Fixed CPU frequency calculation on AMD 19h family
- Updated recovery_urls
- Fixed `DisableSingleUser` quirk when Apple Secure Boot is enabled
- Added `BootstrapShort` to workaround buggy Insyde firmware
- Changed `Bootstrap(Short)` to choose dynamic entry (requires NVRAM reset)
- Avoided `Boot` prefix in `RequestBootVarRouting` to workaround AMI issues
- Added bootloader patch support in `Booter` `Patch` section
- Fixed startup hang on firmware that permit timer function re-entrance
- Made pointer control optional for OpenCanopy via `PickerAttributes`
- Added support for `StartupMute` variable in `PlayChime`
- Added support for per-volume icons for APFS on Preboot
- Removed HII dependency from OpenUsbKbDxe driver
- Fixed undefined behavior in OpenDuet causing random crashes and hangs

#### v0.6.3
- Added support for xml comments in plist files
- Updated underlying EDK II package to edk2-stable202008
- Provide fallbacks for NULL memory SMBIOS strings
- Fixed `BOOTx64.efi` and `BOOTIA32.efi` convention
- Fixed SMBIOS handling with multiple memory arrays
- Fixed memory array handle assignment on empty slots
- Fixed CPUID patching on certain versions of macOS 10.4.10 and 10.4.11
- Fixed incorrect core/thread counts on Pentium M processors
- Added `SSDT-UNC.dsl` ACPI sample to resolve X99 issues, thx @RemB
- Updated builtin firmware versions for SMBIOS and the rest
- Increased slide allocation reserve to 200 MB for Big Sur beta 10
- Fixed assert when trying to enable direct renderer on blit-only GOP
- Added support for custom memory properties
- Fixed intermittent 32-bit prelinking failures caused by improper Mach-O expansion
- Fixed failures in cacheless injection dependency resolution
- Fixed detection issues with older Atom CPUs
- Fixed `ScanPolicy` NVMe handling on MacPro5,1
- Fixed I/O issues on platforms incapable of reading over 1MB at once
- Fixed plist-only kext injection in Big Sur
- Add `ForceResolution` option for enabling non-default resolutions
- Fixed Ps2MouseDxe not properly loading under OpenDuetPkg
- Added workaround for read-only errors on some X299 boards
- Added support for `x86legacy` Secure Boot model
- Added missing Secure Boot NVRAM variables required by 11.0
- Added setting of `system-id` NVRAM variable
- Added `ForceSecureBootScheme` quirk for virtual machines
- Fixed kernel and ACPI patches failing to replace last bytes of memory

#### v0.6.2
- Updated builtin firmware versions for SMBIOS and the rest
- Added `ProcessorType` option to `Generic` allowing custom CPU names
- Fixed `UnblockFsConnect` option not working with APFS JumpStart
- Added IA32 binary variant to the release bundles
- Fixed improper handling of cacheless kexts without an Info.plist
- Fixed improper calculation of kext startup address for blocking
- Added mkext 32-bit kext injection (10.4-10.6)
- Added cacheless 32-bit kext injection (10.4-10.7)
- Added 32-bit kernel/kext patching/blocking support
- Fixed issues loading 10.7 EfiBoot
- Added `Type` to `ReservedMemory` to fulfil hibernation hack needs
- Added workaround to displaying `Preboot` instead of `Macintosh HD`
- Added prelinkedkernel 32-bit kext injection (10.6-10.7)
- Added `SystemMemoryStatus` to override memory replacement on some models
- Added older Pentium CPU recognition in SMBIOS
- Added `ExtendBTFeatureFlags` to properly set `FeatureFlags` for Bluetooth (which substitutes BT4LEContinuityFixup)
- Added `MinKernel`/`MaxKernel` to CPUID emulation and `DummyPowerManagement`
- Fixed `-legacy` not being added in `KernelArch` `Auto` mode
- Fixed `i386-user32` not forcing `i386` on macOS 10.7 on X64 firmware
- Fixed `i386-user32` being incorrectly enabled in macOS 10.4, 10.5, and 10.7
- Disabled prelinked boot for macOS 10.4 and 10.5 in `KernelCache` `Auto` mode
- Fixed `macserial` compatibility with iMac20,x serials and other models from 2020
- Added `LegacyCommpage` quirk to improve pre-SSSE3 userspace compatibility
- Fixed legacy SATA HDDs displaying as external drives in the picker

#### v0.6.1
- Improved recognition of early pressed hotkeys, thx @varahash
- Made DMG loading support configurable via `DmgLoading`
- Added iMac20,1 and iMac20,2 model codes
- Fixed display name for older Xeon CPUs like Xeon E5450
- Added Comet Lake-LP HDA device code
- Fixed OS boot selection on SATA controllers with legacy OPROMs
- Fixed RSDP ACPI table checksum recalculation
- Added immutablekernel loading support for 10.13+
- Fixed solving some symbols to zero in 11.0 kext inject
- Reduced OpenCanopy size by restricting boot management access
- Added `BuiltinText` variant for `TextRenderer` for older laptops
- Fixed `SyncRuntimePermissions` creating invalid MAT table
- Added EFI FAT image loading support (macOS 10.8 and earlier)
- Added 64-bit cacheless kext injection and patching support (macOS 10.9 and earlier)
- Added 64-bit mkext kext injection and patching support (macOS 10.6 and earlier)
- Fixed XNU hook matching non-kernel files
- Updated builtin firmware versions for SMBIOS and the rest
- Fixed patching of ACPI tables in low memory
- Fixed macOS 11.0 DMG recovery loading without hotplug
- Fixed `XhciPortLimit` quirk on 10.12.6 and possibly other versions
- Fixed `IncreasePciBarSize` quirk on 10.11.5 and possibly other versions
- Fixed `LapicKernelPanic` quirk on 10.8.5 and possibly other versions
- Fixed hard-lock caused by EHCI SMI in OpenDuetPkg
- Added preview UEFI Secure Boot compatibility
- Added `FuzzyMatch` option to support fuzzy kernelcache matching on 10.6 and earlier
- Added `KernelArch` option to specify architecture preference on older kernels
- Added `KernelCache` option to specify kernel caching preference for older kernels
- Added `Force` section to provide support for injecting drivers in older macOS
- Changed kernel driver injection to happen prior to kernel driver patching
- Added `Arch` filtering option to `Add`, `Block`, `Force`, and `Patch` sections
- Added `DisableLinkeditJettison` quirk to workaround 11.0b5 kernel panics
- Added debugging of missing fields in the configuration

#### v0.6.0
- Fixed sound corruption with AudioDxe
- Fixed icon choice for Apple FW update in OpenCanopy
- Fixed APFS driver loading on Fusion Drive
- Added Comet Lake HDA device code
- Fixed audio stream position reporting on non-Intel platforms
- Added `Firmware` mode to `ResetSystem` to reboot into preferences
- Replaced `BlacklistAppleUpdate` with `run-efi-updater` NVRAM variable
- Fixed reset value and detection in `FadtEnableReset` ACPI quirk
- Fixed freezes during boot option expansion with PXE boot entries
- Updated underlying EDK II package to edk2-stable202005
- Added `ProvideMaxSlide` quirk to improve laptop stability, thx @zhen-zen
- Fixed slide choice on platforms when 0 slide is unavailable, thx @zhen-zen
- Fixed assertions caused by unaligned file path access in DEBUG builds
- Renamed `ConfigValidity` utility to `ocvalidate` for consistency
- Added `GlobalConnect` for APFS loading to workaround older firmware issues
- Added 11.0 support for `AvoidRuntimeDefrag` Booter quirk
- Fixed 11.0 lapic kernel quirk as of DP1
- Improved boot selection scripts for macOS without NVRAM
- Added UGA protocol compatibility in `ProvideConsoleGop` quirk
- Added `UgaPassThrough` option to support UGA protocol over GOP
- Added `AppleFramebufferInfo` protocol implementation and override
- Fixed serial initialisation when file logging is disabled
- Fixed FSBFrequency reporting on Meron and similar CPUs
- Fixed incorrect volume icon dimension requirements in OpenCanopy
- Added preview version of KernelCollection injection code
- Fixed ACPI reset register detection in DxeIpl
- Added MacBookPro16,4 model code
- Updated builtin firmware versions for SMBIOS and the rest
- Fixed OSXSAVE reporting when emulating CPUID on newer CPUs
- Added `SerialInit` option to perform serial initialisation separately
- Fixed OpenDuetPkg booting on Intel G33 with SATA controller in RAID mode
- `PlatformInfo` `Automatic` for all models
- Fixed 32-bit OpenDuetPkg booting on machines with over 4 GBs of RAM
- Fixed delays with OpenDuetPkg booting with certain SATA controllers in IDE mode
- Fixed display name for some high core count i9 CPUs like 7920X
- Fixed SSDT-EC-USBX

#### v0.5.9
- Added full HiDPI support in OpenCanopy
- Improved OpenCanopy font rendering by using CoreText
- Fixed light and custom background font rendering
- Added `Boot####` options support in boot entry listing
- Removed `HideSelf` by pattern recognising `BOOTx64.efi`
- Added `BlacklistAppleUpdate` to avoid Apple FW updates
- Fixed accidental tool and NVRAM reset booting by default
- Fixed unrecognised select `com.apple.recovery.boot` entries
- Changed NVRAM reset not to erase `BootProtect` boot options
- Improved boot performance when picker UI is disabled
- Enforced the use of builtin picker when external fails
- Fixed warnings for empty NVRAM variables (e.g. rtc-blacklist)
- Added `ApplePanic` to store panic logs on ESP root
- Fixed `ReconnectOnResChange` reconnecting even without res change
- Fixed OpenCanopy showing internal icons for external drives
- Fixed OpenCanopy launching Shell with text over it
- Added partial hotkey support to OpenCanopy (e.g. Ctrl+Enter)
- Added builtin text renderer compatibility with Shell page mode
- Fixed `FadtEnableReset` with too small FACP tables and some laptops
- Fixed CPU detection crash with QEMU 5.0 and KVM accelerator
- Removed `RequestBootVarFallback` due to numerous bugs
- Added `DeduplicateBootOrder` UEFI quirk
- Removed `DirectGopCacheMode` due to being ineffective
- Fixed assertions on log exhaustion causing boot failures
- Fixed builtin text renderer failing to provide ConsoleControl
- Fixed compatibility with blit-only GOP (e.g. OVMF Bochs)
- Fixed ignoring `#` in DeviceProperty and NVRAM `Delete`
- Renamed `Block` to `Delete` in `ACPI`,`DeviceProperties`, and `NVRAM`
- Added MacBookPro16,2 and MacBookPro16,3 model codes
- Added PCI device scanning policy support (e.g. VIRTIO)
- Improved playback performance in AudioDxe
- Updated builtin firmware versions for SMBIOS and the rest
- Added improved CPU type detection for newer CPU types
- Added ConfigValidity utility and improved config validation
- Added serial port initialisation for serial debug logging
- Disabled empty debug log file creation to avoid ESP cluttering
- Added `TscSyncTimeout` quirk to workaround debug kernel assertions
- Added first-class Windows support to bless model
- Fixed `LapicKernelPanic` kernel quirk on 10.9
- Added prebuilt version of `CrScreenshotDxe` driver
- Fixed Hyper-V frequency detection compatibility
- Added `SysReport` option for DEBUG builds to dump system info
- Fixed crashes on some AMD firmware when performing keyboard input

#### v0.5.8
- Fixed invalid CPU object reference in SSDT-PLUG
- Fixed incorrect utilities and resources packaging
- Fixed `Custom` `UpdateSMBIOSMode` modifying SMBIOSv3 table
- Updated docs to cover separating SMBIOS via `UpdateSMBIOSMode`
- Fixed rendering macOS installer icons in OpenCanopy
- Added APFS support with Fusion Drive and enhanced security
- Added AppleEvent mouse support in OpenCanopy
- Fixed AppleEvent and OpenCanopy compatibility with OVMF TPL restrictions
- Added mouse drivers to the package as OVMF needs one
- Added memory region reservation support
- Added RtcRw tool to manipulate RTC memory
- Added `PatchAppleRtcChecksum` kernel quirk
- Added `AppleRtcRam` protocol implementation
- Renamed `Protocols` to `ProtocolOverrides` for clarity
- Added ResetSystem tool to allow shutdown/reset actions in the menu
- Added experimental `BootProtect` `Security` option
- Fixed kext injection in 10.8 installer
- Added timeout support to OpenCanopy user interface
- Fixed handling 24-bit screen resolutions
- Added `Ps2KeyboardDxe` driver for DuetPkg
- Updated `BootInstall` DuetPkg version (now opensource)
- Added partial HiDPI support in OpenCanopy
- Update builtin firmware
- Fixed invalid checksum checks when creating vault (thx @dakanji)

#### v0.5.7
- Added TimeMachine detection to picker
- Added early preview version of OpenCanopy
- Fixed FS discovery on NVMe with legacy drivers
- Added `DirectGopCacheMode` option for FB cache policy
- Added `KeyFiltering` option to workaround buggy KB drivers
- Added tool and custom entry separation in audio assistant
- Added `OpenControl` tool to configure full NVRAM access from Shell
- Added `boot.efi` debug protocol support for 10.15.4+
- Added `boot.efi` performance logging for 10.15.4+
- Added `ProtectUefiServices` quirk to fix `DevirtualiseMmio` on Z390
- Replaced `BOOTCAMP Windows` with `Windows` to match the original
- Added bundled `OpenShell` originally available as OpenCoreShell
- Rework `readlabel` utility into `disklabel` with encoding support
- Renamed `FwRuntimeServices` driver to `OpenRuntime`
- Renamed `AppleUsbKbDxe` driver to `OpenUsbKbDxe`
- Update builtin firmware
- Fixed `PowerTimeoutKernelPanic` on 10.15.4
- Fixed 4K section alignment in `OpenRuntime` to fix Linux booting on SKL
- Introduced `SyncRuntimePermissions` to fix multiple memory permission flaws
- Introduced `RebuildAppleMemoryMap` to fix macOS booting on Dell 5490
- Removed `ShrinkMemoryMap` in favour of more advanced `RebuildAppleMemoryMap`
- Marked `EnableWriteUnprotector` as deprecated on modern systems
- Introduced `ProtectMemoryRegions` to fix memory region handling
- Removed `ProtectCsmRegion` in favour of `ProtectMemoryRegions`
- Renamed `PickerAttributes` to `ConsoleAttributes`
- Introduced `PickerAttributes` as a matter of UI configuration

#### v0.5.6
- Various improvements to builtin text renderer
- Fixed locating DMG recovery in APTIO IV firmware on FAT32
- Fixed loading DMG recovery in APTIO IV firmware on FAT32
- Removed `AvoidHighAlloc` quirk due to removed I/O over 4GB
- Moved `ConsoleMode`, `Resolution` options to `Output` section
- Moved console-related UEFI quirks to `Output` section
- Replaced `ConsoleControl` and `BuiltinTextRenderer` with `TextRenderer`
- Removed `ConsoleBehaviourOs` and `ConsoleBehaviourUi`
- Fixed providing ConsoleOutHandle GOP when running from Shell
- Added `PickerAttributes` option to colour picker
- Added `ProtectSecureBoot` option through FwRuntimeServices
- Replaced `RequireVault` and `RequireSignature` with `Vault`
- Added `BootKicker` tool to support launching Apple BootPicker
- Added BootPicker support as an external UI in OC through `PickerMode`
- Added `DirectGopRendering` option to use direct GOP output
- Multiple memory corruption and performance fixes for PNG support
- Fixed `DefaultBackgroundColor` variable handling
- Added `HideAuxiliary` and `Auxiliary` options
- Fixed picker timeout and log timestamps for VMware
- Fixed NULL parent DeviceHandle for launched tools
- Added bundled HiiDatabase driver for very old firmware
- Added SSE2 support in memory intrinsics for better performance
- Improved ACPI PM timer CPU frequency calculation performance
- Improved LapicKernelPanic compatibility with newer macOS versions
- Fixed drivers starting with `#` not being skipped
- Added audio support through AudioDxe with optional boot chime
- Added VoiceOver accessability support in boot.efi for 10.13+
- Added `PickerAudioAssist` option for audio assistance in picker
- Added `HdaCodecDump.efi` tool in default package
- Added legacy AudioDxe and Microsoft namespaces to Reset NVRAM
- Merged `OcSupportPkg` with `OpenCorePkg` for easier bisection
- Disabled warnings in release versions of NVMe and XHCI drivers

#### v0.5.5
- Fixed CPU bus ratio calculation for Nehalem and Westmere
- Fixed CPU package calculation on MacPro5,1 and similar
- Improved OpenCore rerun detection for new versions
- Fixed loading picker on boot failure when it is hidden
- Added PMC ACPI sample for 300-series chipsets
- Improved driver connection performance on APTIO IV
- Fixed boot option saving in LogoutHook.command
- Added support for OEM information in `ExposeSensitiveData`
- Improved `SanitiseClearScreen` to avoid mode switching
- Replaced `SupportsCsm` with `AdviseWindows` enabling UEFI mode
- Fixed issues with default boot path selection on some boards
- Update builtin firmware versions
- Fixed `AdviseWindows` not setting `FirmwareFeatures` in NVRAM
- Added `TakeoffDelay` option for improved action hotkey support
- Added Mac GOP support to `ProvideConsoleGop` quirk
- Added experimental `BuiltinTextRenderer` boot option
- Added `DummyPowerManagement` kernel quirk to disable CPU PM

#### v0.5.4
- Added Enter key handling in boot menu for quick proceed
- Update builtin firmware versions
- Bundled FwRuntimeServices driver with OpenCore
- Allowed writing to non-volatile variables with disabled write
- Fixed microcode reading on Intel CPUs
- Fixed SMBIOS Type4 External Clock values
- Improved Windows compatibility on some setups (acidanthera/bugtracker#614)
- Added `SupportsCsm` and option in `PlatformInfo/Generic`
- Added `OSInfo` protocol support
- Added `SignalAppleOS` `Booter` quirk to enable IGPU on Macs in other OS
- Added `AppleSmcIo`protocol support (replaces `VirtualSmc` UEFI driver)
- Added `AuthRestart` security property for VirtualSMC authenticated restart
- Fixed input protocol initialisation on VMware fusion
- Added arrow key handling in boot menu
- FileVault 2-like key input is now the only supported input in boot menu
- Fixed 5 second delay when exiting Shell to OpenCore Picker
- Added default boot option update and `AllowSetDefault` `Security` option
- Fixed CPU package detection on configurations with multiple CPUs
- Bundled CleanNvram and VerifyMsrE2 tools for debugging
- Added screen clearing after choosing boot entry in picker
- Added `WriteFlash` NVRAM option to enable writing variables in `Add`
- Added `LegacyOverwrite` NVRAM option to allow overwriting variables by nvram.plist
- Added `AppleXcpmForceBoost` kernel quirk to maximise select Xeon performance
- Bundled NvmExpressDxe and XhciDxe drivers for platforms that need them
- Added `IncreasePciBarSize` kernel quirk for select platforms with PCI space issues

#### v0.5.3
- Update builtin firmware versions
- Fixed interpreting letters in boot menu
- Fixed timeout abortion with PollAppleHotKeys quirk
- Fixed rare kext injection failure due to plist-only kext in prelinkedkernel
- Fixed error reporting for dmg loading
- Added various debugging improvements
- Added new crypto stack resulting in vault key format changes
- Added `UnblockFsConnect` UEFI quirk to fix missing filesystems on some laptops
- Added `RequestBootVarFallback` UEFI quirk to circumvent firmware boot option issues
- Added `ThirdPartyDrives` kernel quirk fixing SSD trim and 10.15 SATA hibernation (thx @lvs1974)
- Removed `ThirdPartyTrim` kernel quirk in favour of `ThirdPartyDrives`
- Added Intel Xeon E5 (Broadwell-EP) support (thx @crazyi)
- Switched to edk2-stable201911, which is now the minimum supportd EDK II version

#### v0.5.2
- Fixed `MinKernel` and `MaxKernel` logic (thx @dhinakg, @reitermarkus)
- Fixed ASSERT when booting non-Apple OSes without arguments from the DEBUG version
- Added `MmioWhitelist` configuration option
- Added `PowerTimeoutKernelPanic` kernel quirk
- Fixed erratic cursor appearing in release builds
- Moved `ReconnectOnResChange` to a user-configurable quirk to avoid freezes
- Added OpenCore version to picker ui, configured by `ExposeSensitiveData`
- Added hypervisor CPUID support to work with virtualization (thx @Leoyzen)

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
