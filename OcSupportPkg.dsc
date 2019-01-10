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
  PLATFORM_GUID           = 6B1D3AB4-5C85-462D-9DC5-480F8B17D5CB
  PLATFORM_VERSION        = 1.0
  SUPPORTED_ARCHITECTURES = X64
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
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf

[Components]
  OcSupportPkg/Library/OcAppleImageVerificationLib/OcAppleImageVerificationLib.inf
  OcSupportPkg/Library/OcCryptoLib/OcCryptoLib.inf
  OcSupportPkg/Library/OcGuardLib/OcGuardLib.inf
  OcSupportPkg/Library/OcPngLib/OcPngLib.inf
  OcSupportPkg/Library/OcAcpiLib/OcAcpiLib.inf
  OcSupportPkg/Library/OcCpuLib/OcCpuLib.inf
  OcSupportPkg/Library/OcDebugLogLib/OcDebugLogLib.inf
  OcSupportPkg/Library/OcDevicePathLib/OcDevicePathLib.inf
  OcSupportPkg/Library/OcFileLib/OcFileLib.inf
  OcSupportPkg/Library/OcMiscLib/OcMiscLib.inf
  OcSupportPkg/Library/OcProtocolLib/OcProtocolLib.inf
  OcSupportPkg/Library/OcStringLib/OcStringLib.inf
  OcSupportPkg/Library/OcTimerLib/OcTimerLib.inf
  OcSupportPkg/Library/OcVariableLib/OcVariableLib.inf
  OcSupportPkg/Library/OcDevicePropertyLib/OcDevicePropertyLib.inf
  OcSupportPkg/Library/OcFirmwarePasswordLib/OcFirmwarePasswordLib.inf
  OcSupportPkg/Library/OcMachoLib/OcMachoLib.inf
  OcSupportPkg/Library/OcSerializeLib/OcSerializeLib.inf
  OcSupportPkg/Library/OcTemplateLib/OcTemplateLib.inf
  OcSupportPkg/Library/OcXmlLib/OcXmlLib.inf
  OcSupportPkg/Debug/GdbSyms/GdbSyms.inf

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdMaximumAsciiStringLength|0
!if $(TARGET) == DEBUG
  # DEBUG_ASSERT_ENABLED | DEBUG_PRINT_ENABLED | DEBUG_CODE_ENABLED | CLEAR_MEMORY_ENABLED | ASSERT_DEADLOOP_ENABLED
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2f
  # DEBUG_ERROR | DEBUG_WARN | DEBUG_INFO
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000042
  gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel|0x80000042
!else
  # DEBUG_PRINT_ENABLED
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|2
  # DEBUG_ERROR | DEBUG_WARN
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000002
  gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel|0x80000002
!endif

[BuildOptions]
  # While there are no PCDs as of now, there at least are some custom macros.
  DEFINE OCSUPPORTPKG_BUILD_OPTIONS_GEN = -D DISABLE_NEW_DEPRECATED_INTERFACES $(OCSUPPORTPKG_BUILD_OPTIONS)

  GCC:DEBUG_*_*_CC_FLAGS     = $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  GCC:NOOPT_*_*_CC_FLAGS     = $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  GCC:RELEASE_*_*_CC_FLAGS   = $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  MSFT:DEBUG_*_*_CC_FLAGS    = $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  MSFT:NOOPT_*_*_CC_FLAGS    = $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  MSFT:RELEASE_*_*_CC_FLAGS  = $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  XCODE:DEBUG_*_*_CC_FLAGS   = $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  XCODE:NOOPT_*_*_CC_FLAGS   = $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
  XCODE:RELEASE_*_*_CC_FLAGS = -flto $(OCSUPPORTPKG_BUILD_OPTIONS_GEN)
