#### Library status

All libraries have several documentation and codestyle issues. These are not
listed here.

- **Functional** state implies that the library is being used.
- **In progress** state implies that the library is incomplete for usage.
- **Legacy** state implies that the library is abandoned.


* OcAcpiLib  
    **Status**: functional  
    **Issues**: none
* OcAppleBootPolicyLib  
    **Status**: functional  
    **Issues**:
    1. No proper interface for OS detection.
    1. No dmg boot detection.
* OcAppleChunklistLib  
    **Status**: in progress  
    **Issues**:
    1. No signature verification.
* OcAppleImageVerificationLib  
    **Status**: functional  
    **Issues**:
    1. Has potential security flaws.
* OcCpuLib  
    **Status**: functional  
    **Issues**:
    1. No package count detection.
    1. No AMD CPU support.
    1. Apple processor type detection is incomplete.
* OcCryptoLib  
    **Status**: functional  
    **Issues**: none
* OcDebugLogLib  
    **Status**: functional  
    **Issues**:
    1. No open-source log protocol implementation.
* OcDevicePathLib  
    **Status**: legacy  
    **Issues**:
    1. Subject for removal if no use.
* OcDevicePropertyLib  
    **Status**: functional  
    **Issues**:
    1. No research done on Apple Thunderbolt protocol.
    1. NVRAM property loading is untested and needs auditing.
    1. Device path conversion is not verified
* OcFileLib  
    **Status**: functional  
    **Issues**: none
* OcFirmwarePasswordLib  
    **Status**: functional  
    **Issues**:
    1. No research done on Apple Firmware Password protocol.
* OcGuardLib  
    **Status**: functional  
    **Issues**: none
* OcMachoLib  
    **Status**: in progress  
    **Issues**:
* OcMiscLib  
    **Status**: legacy  
    **Issues**:
    1. Subject for refactoring except Base64Decode, DataPatcher, LegacyRegion, NullTextOutput.
* OcPngLib  
    **Status**: functional  
    **Issues**: none
* OcProtocolLib  
    **Status**: legacy  
    **Issues**:
    1. Subject for removal if no use.
* OcRtcLib  
    **Status**: functional  
    **Issues**: none
* OcSerializeLib  
    **Status**: functional  
    **Issues**: none
* OcSmbiosLib  
    **Status**: functional  
    **Issues**: none
    1. Potentially reports incorrect memory on some boards.
    1. No SMC information table is provided.
* OcStringLib  
    **Status**: legacy  
    **Issues**:
    1. Subject for removal if no use.
* OcTemplateLib  
    **Status**: functional  
    **Issues**: none
* OcTimerLib  
    **Status**: functional  
    **Issues**:
    1. No AMD CPU support.
* OcXmlLib  
    **Status**: functional  
    **Issues**: none
