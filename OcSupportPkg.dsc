## @file
# Copyright (C) 2018, vit9696.  All rights reserved.<BR>
# Copyright (C) 2018, Download-Fritz.  All rights reserved.<BR>
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
##

[Defines]
  PLATFORM_NAME           = OcSupportPkg
  PLATFORM_GUID           = C9F9C1B2-67ED-4496-97B6-3B24F083A2E6
  PLATFORM_VERSION        = 1.0
  SUPPORTED_ARCHITECTURES = X64|IA32
  BUILD_TARGETS           = RELEASE|DEBUG|NOOPT
  SKUID_IDENTIFIER        = DEFAULT
  DSC_SPECIFICATION       = 0x00010006

[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseRngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  DebugLib|OcSupportPkg/Library/OcDebugLogLib/OcDebugLogLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  OcAcpiLib|OcSupportPkg/Library/OcAcpiLib/OcAcpiLib.inf
  OcAppleBootCompatLib|OcSupportPkg/Library/OcAppleBootCompatLib/OcAppleBootCompatLib.inf
  OcAppleBootPolicyLib|OcSupportPkg/Library/OcAppleBootPolicyLib/OcAppleBootPolicyLib.inf
  OcAppleChunklistLib|OcSupportPkg/Library/OcAppleChunklistLib/OcAppleChunklistLib.inf
  OcAppleImg4Lib|OcSupportPkg/Library/OcAppleImg4Lib/OcAppleImg4Lib.inf
  OcAppleDiskImageLib|OcSupportPkg/Library/OcAppleDiskImageLib/OcAppleDiskImageLib.inf
  OcAppleEventLib|OcSupportPkg/Library/OcAppleEventLib/OcAppleEventLib.inf
  OcInputLib|OcSupportPkg/Library/OcInputLib/OcInputLib.inf
  OcAppleImageConversionLib|OcSupportPkg/Library/OcAppleImageConversionLib/OcAppleImageConversionLib.inf
  OcAppleImageVerificationLib|OcSupportPkg/Library/OcAppleImageVerificationLib/OcAppleImageVerificationLib.inf
  OcAppleKernelLib|OcSupportPkg/Library/OcAppleKernelLib/OcAppleKernelLib.inf
  OcAppleKeyMapLib|OcSupportPkg/Library/OcAppleKeyMapLib/OcAppleKeyMapLib.inf
  OcAppleKeysLib|OcSupportPkg/Library/OcAppleKeysLib/OcAppleKeysLib.inf
  OcAppleRamDiskLib|OcSupportPkg/Library/OcAppleRamDiskLib/OcAppleRamDiskLib.inf
  OcAppleSecureBootLib|OcSupportPkg/Library/OcAppleSecureBootLib/OcAppleSecureBootLib.inf
  OcAppleUserInterfaceThemeLib|OcSupportPkg/Library/OcAppleUserInterfaceThemeLib/OcAppleUserInterfaceThemeLib.inf
  OcBootManagementLib|OcSupportPkg/Library/OcBootManagementLib/OcBootManagementLib.inf
  OcConsoleLib|OcSupportPkg/Library/OcConsoleLib/OcConsoleLib.inf
  OcConsoleControlEntryModeLib|OcSupportPkg/Library/OcConsoleControlEntryModeLib/OcConsoleControlEntryModeLib.inf
  OcCpuLib|OcSupportPkg/Library/OcCpuLib/OcCpuLib.inf
  OcCryptoLib|OcSupportPkg/Library/OcCryptoLib/OcCryptoLib.inf
  OcCompressionLib|OcSupportPkg/Library/OcCompressionLib/OcCompressionLib.inf
  OcDataHubLib|OcSupportPkg/Library/OcDataHubLib/OcDataHubLib.inf
  OcDebugLogLib|OcSupportPkg/Library/OcDebugLogLib/OcDebugLogLib.inf
  OcDevicePathLib|OcSupportPkg/Library/OcDevicePathLib/OcDevicePathLib.inf
  OcDevicePropertyLib|OcSupportPkg/Library/OcDevicePropertyLib/OcDevicePropertyLib.inf
  OcFileLib|OcSupportPkg/Library/OcFileLib/OcFileLib.inf
  OcFirmwarePasswordLib|OcSupportPkg/Library/OcFirmwarePasswordLib/OcFirmwarePasswordLib.inf
  OcFirmwareVolumeLib|OcSupportPkg/Library/OcFirmwareVolumeLib/OcFirmwareVolumeLib.inf
  OcGuardLib|OcSupportPkg/Library/OcGuardLib/OcGuardLib.inf
  OcHashServicesLib|OcSupportPkg/Library/OcHashServicesLib/OcHashServicesLib.inf
  OcMachoLib|OcSupportPkg/Library/OcMachoLib/OcMachoLib.inf
  OcMemoryLib|OcSupportPkg/Library/OcMemoryLib/OcMemoryLib.inf
  OcMiscLib|OcSupportPkg/Library/OcMiscLib/OcMiscLib.inf
  OcPngLib|OcSupportPkg/Library/OcPngLib/OcPngLib.inf
  OcRngLib|OcSupportPkg/Library/OcRngLib/OcRngLib.inf
  OcRtcLib|OcSupportPkg/Library/OcRtcLib/OcRtcLib.inf
  OcSerializeLib|OcSupportPkg/Library/OcSerializeLib/OcSerializeLib.inf
  OcSmbiosLib|OcSupportPkg/Library/OcSmbiosLib/OcSmbiosLib.inf
  OcStringLib|OcSupportPkg/Library/OcStringLib/OcStringLib.inf
  OcTemplateLib|OcSupportPkg/Library/OcTemplateLib/OcTemplateLib.inf
  OcTimerLib|OcSupportPkg/Library/OcTimerLib/OcTimerLib.inf
  OcUnicodeCollationEngLib|OcSupportPkg/Library/OcUnicodeCollationEngLib/OcUnicodeCollationEngLib.inf
  OcVirtualFsLib|OcSupportPkg/Library/OcVirtualFsLib/OcVirtualFsLib.inf
  OcXmlLib|OcSupportPkg/Library/OcXmlLib/OcXmlLib.inf

[Components]
  OcSupportPkg/Library/OcAcpiLib/OcAcpiLib.inf
  OcSupportPkg/Library/OcAppleBootCompatLib/OcAppleBootCompatLib.inf
  OcSupportPkg/Library/OcAppleBootPolicyLib/OcAppleBootPolicyLib.inf
  OcSupportPkg/Library/OcAppleChunklistLib/OcAppleChunklistLib.inf
  OcSupportPkg/Library/OcAppleImg4Lib/OcAppleImg4Lib.inf
  OcSupportPkg/Library/OcAppleDiskImageLib/OcAppleDiskImageLib.inf
  OcSupportPkg/Library/OcAppleEventLib/OcAppleEventLib.inf
  OcSupportPkg/Library/OcInputLib/OcInputLib.inf
  OcSupportPkg/Library/OcAppleImageConversionLib/OcAppleImageConversionLib.inf
  OcSupportPkg/Library/OcAppleImageVerificationLib/OcAppleImageVerificationLib.inf
  OcSupportPkg/Library/OcAppleKernelLib/OcAppleKernelLib.inf
  OcSupportPkg/Library/OcAppleKeyMapLib/OcAppleKeyMapLib.inf
  OcSupportPkg/Library/OcAppleRamDiskLib/OcAppleRamDiskLib.inf
  OcSupportPkg/Library/OcAppleSecureBootLib/OcAppleSecureBootLib.inf
  OcSupportPkg/Library/OcAppleUserInterfaceThemeLib/OcAppleUserInterfaceThemeLib.inf
  OcSupportPkg/Library/OcBootManagementLib/OcBootManagementLib.inf
  OcSupportPkg/Library/OcConfigurationLib/OcConfigurationLib.inf
  OcSupportPkg/Library/OcConsoleLib/OcConsoleLib.inf
  OcSupportPkg/Library/OcConsoleControlEntryModeLib/OcConsoleControlEntryModeLib.inf
  OcSupportPkg/Library/OcCpuLib/OcCpuLib.inf
  OcSupportPkg/Library/OcCryptoLib/OcCryptoLib.inf
  OcSupportPkg/Library/OcCompressionLib/OcCompressionLib.inf
  OcSupportPkg/Library/OcDataHubLib/OcDataHubLib.inf
  OcSupportPkg/Library/OcDebugLogLib/OcDebugLogLib.inf
  OcSupportPkg/Library/OcDevicePathLib/OcDevicePathLib.inf
  OcSupportPkg/Library/OcDevicePropertyLib/OcDevicePropertyLib.inf
  OcSupportPkg/Library/OcDeviceTreeLib/OcDeviceTreeLib.inf
  OcSupportPkg/Library/OcFileLib/OcFileLib.inf
  OcSupportPkg/Library/OcFirmwarePasswordLib/OcFirmwarePasswordLib.inf
  OcSupportPkg/Library/OcFirmwareVolumeLib/OcFirmwareVolumeLib.inf
  OcSupportPkg/Library/OcGuardLib/OcGuardLib.inf
  OcSupportPkg/Library/OcHashServicesLib/OcHashServicesLib.inf
  OcSupportPkg/Library/OcMachoLib/OcMachoLib.inf
  OcSupportPkg/Library/OcMemoryLib/OcMemoryLib.inf
  OcSupportPkg/Library/OcMiscLib/OcMiscLib.inf
  OcSupportPkg/Library/OcPngLib/OcPngLib.inf
  OcSupportPkg/Library/OcRngLib/OcRngLib.inf
  OcSupportPkg/Library/OcSerializeLib/OcSerializeLib.inf
  OcSupportPkg/Library/OcSmbiosLib/OcSmbiosLib.inf
  OcSupportPkg/Library/OcStorageLib/OcStorageLib.inf
  OcSupportPkg/Library/OcStringLib/OcStringLib.inf
  OcSupportPkg/Library/OcTemplateLib/OcTemplateLib.inf
  OcSupportPkg/Library/OcTimerLib/OcTimerLib.inf
  OcSupportPkg/Library/OcUnicodeCollationEngLib/OcUnicodeCollationEngLib.inf
  OcSupportPkg/Library/OcVirtualFsLib/OcVirtualFsLib.inf
  OcSupportPkg/Library/OcXmlLib/OcXmlLib.inf
  OcSupportPkg/Debug/GdbSyms/GdbSyms.inf
  OcSupportPkg/Tests/CryptoTest/CryptoTest.inf
  OcSupportPkg/Tests/CryptoTest/CryptoTestApp.inf
  OcSupportPkg/Tests/AcpiTest/AcpiTest.inf
  OcSupportPkg/Tests/AcpiTest/AcpiTestApp.inf
  OcSupportPkg/Tests/BlessTest/BlessTest.inf
  OcSupportPkg/Tests/BlessTest/BlessTestApp.inf
  OcSupportPkg/Tests/DataHubTest/DataHubTest.inf
  OcSupportPkg/Tests/DataHubTest/DataHubTestApp.inf
  OcSupportPkg/Tests/ExternalUi/ExternalUi.inf
  OcSupportPkg/Tests/KernelTest/KernelTest.inf
  OcSupportPkg/Tests/KernelTest/KernelTestApp.inf
  OcSupportPkg/Tests/PropertyTest/PropertyTest.inf
  OcSupportPkg/Tests/PropertyTest/PropertyTestApp.inf
  OcSupportPkg/Tests/SmbiosTest/SmbiosTest.inf
  OcSupportPkg/Tests/SmbiosTest/SmbiosTestApp.inf

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdMaximumAsciiStringLength|0
!if $(TARGET) == RELEASE
  # DEBUG_PRINT_ENABLED
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|2
  # DEBUG_ERROR | DEBUG_WARN
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000002
  gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel|0x80000002
!else
  # DEBUG_ASSERT_ENABLED | DEBUG_PRINT_ENABLED | DEBUG_CODE_ENABLED | CLEAR_MEMORY_ENABLED | ASSERT_DEADLOOP_ENABLED
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2f
  # DEBUG_ERROR | DEBUG_WARN | DEBUG_INFO
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000042
  gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel|0x80000042
!endif

[BuildOptions]
  # While there are no PCDs as of now, there at least are some custom macros.
  DEFINE OCSUPPORTPKG_BUILD_OPTIONS_GEN = -D DISABLE_NEW_DEPRECATED_INTERFACES $(OCSUPPORTPKG_BUILD_OPTIONS)

  GCC:DEBUG_*_*_CC_FLAGS     = -D OC_TARGET_DEBUG=1 $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  GCC:NOOPT_*_*_CC_FLAGS     = -D OC_TARGET_NOOPT=1 $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  GCC:RELEASE_*_*_CC_FLAGS   = -D OC_TARGET_RELEASE=1 $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  MSFT:DEBUG_*_*_CC_FLAGS    = -D OC_TARGET_DEBUG=1 $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  MSFT:NOOPT_*_*_CC_FLAGS    = -D OC_TARGET_NOOPT=1 $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  MSFT:RELEASE_*_*_CC_FLAGS  = -D OC_TARGET_RELEASE=1 $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  XCODE:DEBUG_*_*_CC_FLAGS   = -D OC_TARGET_DEBUG=1 $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  XCODE:NOOPT_*_*_CC_FLAGS   = -D OC_TARGET_NOOPT=1 $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  XCODE:RELEASE_*_*_CC_FLAGS = -D OC_TARGET_RELEASE=1 -Oz -flto $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
