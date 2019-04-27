#### Library status

All libraries have several documentation and codestyle issues. These are not
listed here.

- **Functional** state implies that the library is being used.
- **In progress** state implies that the library is incomplete for usage.
- **Legacy** state implies that the library is abandoned.


* OcAcpiLib  
    **Summary**: ACPI injector and patcher  
    **Status**: functional  
    **Issues**: none
* OcAppleBootPolicyLib  
    **Summary**: Apple bless protocol implementation  
    **Status**: functional  
    **Issues**: none
* OcAppleKernelLib  
    **Summary**: Apple kernelspace injector and patcher  
    **Status**: functional  
    **Issues**:
    1. Booting without caches on 10.9 or earlier will bypass kext injection.
* OcCompressionLib  
    **Summary**: Misc compression and decompression (LZSS, LZVN, ZLIB)  
    **Status**: functional  
    **Issues**: none
* OcAppleChunklistLib  
    **Summary**: Apple chunklist (e.g. for dmg hashes) handling library  
    **Status**: in progress  
    **Issues**:
    1. No signature verification.
* OcAppleImageVerificationLib  
    **Summary**: Apple EFI image signature verification lib  
    **Status**: in progress  
    **Issues**:
    1. Has potential security flaws.
* OcBootManagementLib
    **Summary**: Simple blessed-based boot management with UI  
    **Status**: functional  
    **Issues**:
    1. No proper interface for OS detection.
    1. No dmg boot detection.
    1. No preferred entry detection from nvram BootOrder
* OcCpuLib  
    **Summary**: CPU feature scanning  
    **Status**: functional  
    **Issues**:
    1. No package count detection.
    1. No AMD CPU support.
    1. Apple processor type detection is incomplete.
* OcCryptoLib  
    **Summary**: Misc cryptographic primitives (AES, RSA, MD5, SHA-1, SHA-256)  
    **Status**: functional  
    **Issues**: none
* OcDataHubLib  
    **Summary**: Apple-specific DataHub data configuration  
    **Status**: functional  
    **Issues**: none
* OcAppleDiskImageLib  
    **Summary**: Expose DMG as an UEFI RAM disk  
    **Status**: in progress  
    **Issues**:
* OcConfigurationLib  
    **Summary**: Deserialize OpenCore configuration  
    **Status**: functional  
    **Issues**:
* OcDebugLogLib  
    **Summary**: Debug output redirection through OC Log protocol  
    **Status**: functional  
    **Issues**:
    1. No file logging.
* OcDevicePathLib  
    **Summary**: Device path management and transformation  
    **Status**: legacy  
    **Issues**:
    1. Subject for removal if no use.
* OcDevicePropertyLib  
    **Summary**: Apple device property protocol implementation  
    **Status**: functional  
    **Issues**:
    1. No research done on Apple Thunderbolt protocol.
    1. NVRAM property loading is untested and needs auditing.
    1. Device path conversion is not verified
* OcFileLib  
    **Summary**: Supplemental file I/O  
    **Status**: functional  
    **Issues**: none
* OcFirmwarePasswordLib  
    **Summary**: Apple firmware password protocol implementation  
    **Status**: functional  
    **Issues**:
    1. No research done on Apple Firmware Password protocol.
* OcGuardLib  
    **Summary**: Basic sanity checking (static assertions, overflow maths)  
    **Status**: functional  
    **Issues**:
    1. Stack canary has no runtime support (e.g. via rdrand)
    1. Stack canary does not work with LTO
* OcMachoLib  
    **Summary**: Mach-O image handling and transformation  
    **Status**: functional  
    **Issues**: none
* OcMiscLib  
    **Summary**: Miscellaneous stuff not fitting elsewhere  
    **Status**: legacy  
    **Issues**:
    1. Subject for refactoring except Base64Decode, DataPatcher, LegacyRegion, NullTextOutput.
* OcPngLib  
    **Summary**: PNG image decoding  
    **Status**: functional  
    **Issues**: none
* OcRtcLib  
    **Summary**: CMOS memory access  
    **Status**: functional  
    **Issues**: none
* OcSerializeLib  
    **Summary**: PLIST document deserialization  
    **Status**: functional  
    **Issues**: none
* OcSmbiosLib  
    **Summary**: Apple-specific SMBIOS data configuration  
    **Status**: functional  
    **Issues**: none
    1. Potentially reports incorrect memory on some boards.
    1. No SMC information table is provided.
* OcStorageLib
    **Summary**: Resource storage abstraction (from e.g. FS I/O)  
    **Status**: in progress  
    **Issues**: none
    1. Does not support signature verification.
    1. Does not detect file removal from signed vault.
* OcStringLib  
    **Summary**: String handling and management  
    **Status**: functional  
    **Issues**: none
* OcTemplateLib  
    **Summary**: Data description and resource management  
    **Status**: functional  
    **Issues**: none
* OcTimerLib  
    **Summary**: EDK II timer library based on TSC  
    **Status**: functional  
    **Issues**:
    1. No AMD CPU support.
* OcVirtualFsLib  
    **Summary**: UEFI file system interception  
    **Status**: functional  
    **Issues**:
    1. Does not support directory iteration with virtualised files.
* OcXmlLib  
    **Summary**: XML and PLIST reading and transformation  
    **Status**: functional  
    **Issues**: none
