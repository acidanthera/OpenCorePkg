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
  PLATFORM_NAME           = OpenCorePkg
  PLATFORM_GUID           = C46F121D-ABC6-42A3-A241-91B09224C357
  PLATFORM_VERSION        = 1.0
  SUPPORTED_ARCHITECTURES = X64|IA32
  BUILD_TARGETS           = RELEASE|DEBUG|NOOPT
  SKUID_IDENTIFIER        = DEFAULT
  DSC_SPECIFICATION       = 0x00010006
  FLASH_DEFINITION        = OpenCorePkg/OpenCorePkg.fdf

  #
  # Network definition
  #
  DEFINE NETWORK_ENABLE                 = TRUE
  DEFINE NETWORK_SNP_ENABLE             = TRUE
  DEFINE NETWORK_IP4_ENABLE             = TRUE
  DEFINE NETWORK_IP6_ENABLE             = FALSE
  DEFINE NETWORK_TLS_ENABLE             = FALSE
  DEFINE NETWORK_HTTP_ENABLE            = TRUE
  DEFINE NETWORK_HTTP_BOOT_ENABLE       = TRUE
  DEFINE NETWORK_ALLOW_HTTP_CONNECTIONS = TRUE
  DEFINE NETWORK_ISCSI_ENABLE           = FALSE
  DEFINE NETWORK_ISCSI_MD5_ENABLE       = FALSE
  DEFINE NETWORK_VLAN_ENABLE            = FALSE

!include NetworkPkg/NetworkDefines.dsc.inc

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  # We cannot use BaseMemoryLibOptDxe since it uses SSE instructions,
  # and some types of firmware fail to properly maintain MMX register contexts
  # across the timers. This results in exceptions when trying to execute
  # primitives like CopyMem in timers (e.g. AIKDataWriteEntry).
  # Reproduced on ASUS M5A97 with AMD FX8320 CPU.
  # REF: https://github.com/acidanthera/bugtracker/issues/754
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  BaseOverflowLib|MdePkg/Library/BaseOverflowLib/BaseOverflowLib.inf
  BaseRngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  BcfgCommandLib|ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  DebugLib|OpenCorePkg/Library/OcDebugLibProtocol/OcDebugLibProtocol.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  MtrrLib|UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  FrameBufferBltLib|MdeModulePkg/Library/FrameBufferBltLib/FrameBufferBltLib.inf
  HandleParsingLib|ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
  OrderedCollectionLib|MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib/BaseOrderedCollectionRedBlackTreeLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  OcAcpiLib|OpenCorePkg/Library/OcAcpiLib/OcAcpiLib.inf
  OcAfterBootCompatLib|OpenCorePkg/Library/OcAfterBootCompatLib/OcAfterBootCompatLib.inf
  OcApfsLib|OpenCorePkg/Library/OcApfsLib/OcApfsLib.inf
  OcAppleBootPolicyLib|OpenCorePkg/Library/OcAppleBootPolicyLib/OcAppleBootPolicyLib.inf
  OcAppleChunklistLib|OpenCorePkg/Library/OcAppleChunklistLib/OcAppleChunklistLib.inf
  OcAppleDiskImageLib|OpenCorePkg/Library/OcAppleDiskImageLib/OcAppleDiskImageLib.inf
  OcAppleEventLib|OpenCorePkg/Library/OcAppleEventLib/OcAppleEventLib.inf
  OcAppleImageConversionLib|OpenCorePkg/Library/OcAppleImageConversionLib/OcAppleImageConversionLib.inf
  OcAppleImg4Lib|OpenCorePkg/Library/OcAppleImg4Lib/OcAppleImg4Lib.inf
  OcAppleKernelLib|OpenCorePkg/Library/OcAppleKernelLib/OcAppleKernelLib.inf
  OcAppleKeyMapLib|OpenCorePkg/Library/OcAppleKeyMapLib/OcAppleKeyMapLib.inf
  OcAppleKeysLib|OpenCorePkg/Library/OcAppleKeysLib/OcAppleKeysLib.inf
  OcAppleRamDiskLib|OpenCorePkg/Library/OcAppleRamDiskLib/OcAppleRamDiskLib.inf
  OcAppleSecureBootLib|OpenCorePkg/Library/OcAppleSecureBootLib/OcAppleSecureBootLib.inf
  OcAppleUserInterfaceThemeLib|OpenCorePkg/Library/OcAppleUserInterfaceThemeLib/OcAppleUserInterfaceThemeLib.inf
  OcAudioLib|OpenCorePkg/Library/OcAudioLib/OcAudioLib.inf
  OcBlitLib|OpenCorePkg/Library/OcBlitLib/OcBlitLib.inf
  OcBootManagementLib|OpenCorePkg/Library/OcBootManagementLib/OcBootManagementLib.inf
  OcBootServicesTableLib|OpenCorePkg/Library/OcBootServicesTableLib/OcBootServicesTableLib.inf
  OcCompressionLib|OpenCorePkg/Library/OcCompressionLib/OcCompressionLib.inf
  OcConfigurationLib|OpenCorePkg/Library/OcConfigurationLib/OcConfigurationLib.inf
  OcConsoleControlEntryModeGenericLib|OpenCorePkg/Library/OcConsoleControlEntryModeLib/OcConsoleControlEntryModeGenericLib.inf
  OcConsoleControlEntryModeLocalLib|OpenCorePkg/Library/OcConsoleControlEntryModeLib/OcConsoleControlEntryModeLocalLib.inf
  OcConsoleLib|OpenCorePkg/Library/OcConsoleLib/OcConsoleLib.inf
  OcCpuLib|OpenCorePkg/Library/OcCpuLib/OcCpuLib.inf
  OcCryptoLib|OpenCorePkg/Library/OcCryptoLib/OcCryptoLib.inf
  OcDataHubLib|OpenCorePkg/Library/OcDataHubLib/OcDataHubLib.inf
  OcDeviceMiscLib|OpenCorePkg/Library/OcDeviceMiscLib/OcDeviceMiscLib.inf
  OcDevicePathLib|OpenCorePkg/Library/OcDevicePathLib/OcDevicePathLib.inf
  OcDevicePropertyLib|OpenCorePkg/Library/OcDevicePropertyLib/OcDevicePropertyLib.inf
  OcDeviceTreeLib|OpenCorePkg/Library/OcDeviceTreeLib/OcDeviceTreeLib.inf
  OcDirectResetLib|OpenCorePkg/Library/OcDirectResetLib/OcDirectResetLib.inf
  OcDriverConnectionLib|OpenCorePkg/Library/OcDriverConnectionLib/OcDriverConnectionLib.inf
  OcFileLib|OpenCorePkg/Library/OcFileLib/OcFileLib.inf
  OcFirmwarePasswordLib|OpenCorePkg/Library/OcFirmwarePasswordLib/OcFirmwarePasswordLib.inf
  OcFirmwareVolumeLib|OpenCorePkg/Library/OcFirmwareVolumeLib/OcFirmwareVolumeLib.inf
  OcFlexArrayLib|OpenCorePkg/Library/OcFlexArrayLib/OcFlexArrayLib.inf
  OcGuardLib|OpenCorePkg/Library/OcGuardLib/OcGuardLib.inf
  OcHashServicesLib|OpenCorePkg/Library/OcHashServicesLib/OcHashServicesLib.inf
  OcHdaDevicesLib|OpenCorePkg/Library/OcHdaDevicesLib/OcHdaDevicesLib.inf
  OcHeciLib|OpenCorePkg/Library/OcHeciLib/OcHeciLib.inf
  OcHiiDatabaseLocalLib|OpenCorePkg/Library/OcHiiDatabaseLib/OcHiiDatabaseLocalLib.inf
  OcInputLib|OpenCorePkg/Library/OcInputLib/OcInputLib.inf
  OcLogAggregatorLib|OpenCorePkg/Library/OcLogAggregatorLib/OcLogAggregatorLib.inf
  OcMachoLib|OpenCorePkg/Library/OcMachoLib/OcMachoLib.inf
  OcMacInfoLib|OpenCorePkg/Library/OcMacInfoLib/OcMacInfoLib.inf
  OcMainLib|OpenCorePkg/Library/OcMainLib/OcMainLib.inf
  OcMemoryLib|OpenCorePkg/Library/OcMemoryLib/OcMemoryLib.inf
  OcMiscLib|OpenCorePkg/Library/OcMiscLib/OcMiscLib.inf
  OcMp3Lib|OpenCorePkg/Library/OcMp3Lib/OcMp3Lib.inf
  OcOSInfoLib|OpenCorePkg/Library/OcOSInfoLib/OcOSInfoLib.inf
  OcPciIoLib|OpenCorePkg/Library/OcPciIoLib/OcPciIoLib.inf
  OcPngLib|OpenCorePkg/Library/OcPngLib/OcPngLib.inf
  OcRngLib|OpenCorePkg/Library/OcRngLib/OcRngLib.inf
  OcRtcLib|OpenCorePkg/Library/OcRtcLib/OcRtcLib.inf
  OcSerializeLib|OpenCorePkg/Library/OcSerializeLib/OcSerializeLib.inf
  OcSmbiosLib|OpenCorePkg/Library/OcSmbiosLib/OcSmbiosLib.inf
  OcSmcLib|OpenCorePkg/Library/OcSmcLib/OcSmcLib.inf
  OcStorageLib|OpenCorePkg/Library/OcStorageLib/OcStorageLib.inf
  OcStringLib|OpenCorePkg/Library/OcStringLib/OcStringLib.inf
  OcTemplateLib|OpenCorePkg/Library/OcTemplateLib/OcTemplateLib.inf
  OcTypingLib|OpenCorePkg/Library/OcTypingLib/OcTypingLib.inf
  TimerLib|OpenCorePkg/Library/OcTimerLib/OcTimerLib.inf
  OcUnicodeCollationEngGenericLib|OpenCorePkg/Library/OcUnicodeCollationEngLib/OcUnicodeCollationEngGenericLib.inf
  OcUnicodeCollationEngLocalLib|OpenCorePkg/Library/OcUnicodeCollationEngLib/OcUnicodeCollationEngLocalLib.inf
  OcVirtualFsLib|OpenCorePkg/Library/OcVirtualFsLib/OcVirtualFsLib.inf
  OcWaveLib|OpenCorePkg/Library/OcWaveLib/OcWaveLib.inf
  OcXmlLib|OpenCorePkg/Library/OcXmlLib/OcXmlLib.inf
  OcPeCoffExtLib|OpenCorePkg/Library/OcPeCoffExtLib/OcPeCoffExtLib.inf
  OcVariableLib|OpenCorePkg/Library/OcVariableLib/OcVariableLib.inf
  OcVariableRuntimeLib|OpenCorePkg/Library/OcVariableRuntimeLib/OcVariableRuntimeLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PeCoffLib2|MdePkg/Library/BasePeCoffLib2/BasePeCoffLib2.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
  ShellCommandLib|ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  UefiApplicationEntryPoint|OpenCorePkg/Library/OcApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  UefiDriverEntryPoint|OpenCorePkg/Library/OcDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  UefiImageExtraActionLib|MdePkg/Library/BaseUefiImageExtraActionLibNull/BaseUefiImageExtraActionLibNull.inf
  UefiImageLib|MdePkg/Library/BaseUefiImageLib/BaseUefiImageLibPeCoff.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
  VariableFlashInfoLib|MdeModulePkg/Library/BaseVariableFlashInfoLib/BaseVariableFlashInfoLib.inf
  ResetSystemLib|OpenCorePkg/Library/OcResetSystemLib/OcResetSystemLib.inf

  !include NetworkPkg/NetworkLibs.dsc.inc

  !include Ext4Pkg/Ext4Defines.dsc.inc
  !include Ext4Pkg/Ext4Libs.dsc.inc

[Components]
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf {
    <LibraryClasses>
      !if $(TARGET) == RELEASE
        DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
      !endif
  }
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf {
    <LibraryClasses>
      !if $(TARGET) == RELEASE
        DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
      !endif
  }
  MdeModulePkg/Bus/Isa/Ps2MouseDxe/Ps2MouseDxe.inf
  MdeModulePkg/Bus/Isa/Ps2KeyboardDxe/Ps2KeyboardDxe.inf
  MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  OpenCorePkg/Application/BootKicker/BootKicker.inf
  OpenCorePkg/Application/Bootstrap/Bootstrap.inf {
    <LibraryClasses>
      !if $(TARGET) != RELEASE
        # Force onscreen visible logging in DEBUG/NOOPT builds.
        NULL|OpenCorePkg/Library/OcConsoleControlEntryModeLib/OcConsoleControlEntryModeGenericLib.inf
      !endif
  }
  OpenCorePkg/Application/ChipTune/ChipTune.inf
  OpenCorePkg/Application/CleanNvram/CleanNvram.inf
  OpenCorePkg/Application/CsrUtil/CsrUtil.inf
  OpenCorePkg/Application/FontTester/FontTester.inf
  OpenCorePkg/Application/GopPerf/GopPerf.inf
  OpenCorePkg/Application/GopStop/GopStop.inf
  OpenCorePkg/Application/KeyTester/KeyTester.inf
  OpenCorePkg/Application/ListPartitions/ListPartitions.inf
  OpenCorePkg/Application/MmapDump/MmapDump.inf
  OpenCorePkg/Application/OpenControl/OpenControl.inf
  OpenCorePkg/Application/OpenCore/OpenCore.inf {
    <LibraryClasses>
      !if $(TARGET) != RELEASE
        # Force onscreen visible logging in DEBUG/NOOPT builds.
        NULL|OpenCorePkg/Library/OcConsoleControlEntryModeLib/OcConsoleControlEntryModeGenericLib.inf
      !endif
  }
  OpenCorePkg/Application/PavpProvision/PavpProvision.inf
  OpenCorePkg/Application/ResetSystem/ResetSystem.inf
  OpenCorePkg/Application/RtcRw/RtcRw.inf
  OpenCorePkg/Application/TpmInfo/TpmInfo.inf
  OpenCorePkg/Application/VerifyMemOpt/VerifyMemOpt.inf {
    <LibraryClasses>
      BaseMemoryLib|MdePkg/Library/BaseMemoryLibOptDxe/BaseMemoryLibOptDxe.inf
  }
  OpenCorePkg/Application/ControlMsrE2/ControlMsrE2.inf
  OpenCorePkg/Debug/GdbSyms/GdbSyms.inf
  OpenCorePkg/Library/OcAcpiLib/OcAcpiLib.inf
  OpenCorePkg/Library/OcAfterBootCompatLib/OcAfterBootCompatLib.inf
  OpenCorePkg/Library/OcApfsLib/OcApfsLib.inf
  OpenCorePkg/Library/OcAppleBootPolicyLib/OcAppleBootPolicyLib.inf
  OpenCorePkg/Library/OcAppleChunklistLib/OcAppleChunklistLib.inf
  OpenCorePkg/Library/OcAppleDiskImageLib/OcAppleDiskImageLib.inf
  OpenCorePkg/Library/OcAppleEventLib/OcAppleEventLib.inf
  OpenCorePkg/Library/OcAppleImageConversionLib/OcAppleImageConversionLib.inf
  OpenCorePkg/Library/OcAppleImg4Lib/OcAppleImg4Lib.inf
  OpenCorePkg/Library/OcAppleKernelLib/OcAppleKernelLib.inf
  OpenCorePkg/Library/OcAppleKeyMapLib/OcAppleKeyMapLib.inf
  OpenCorePkg/Library/OcAppleRamDiskLib/OcAppleRamDiskLib.inf
  OpenCorePkg/Library/OcAppleSecureBootLib/OcAppleSecureBootLib.inf
  OpenCorePkg/Library/OcAppleUserInterfaceThemeLib/OcAppleUserInterfaceThemeLib.inf
  OpenCorePkg/Library/OcAudioLib/OcAudioLib.inf
  OpenCorePkg/Library/OcBlitLib/OcBlitLib.inf
  OpenCorePkg/Library/OcBootManagementLib/OcBootManagementLib.inf
  OpenCorePkg/Library/OcBootServicesTableLib/OcBootServicesTableLib.inf
  OpenCorePkg/Library/OcCompilerIntrinsicsLib/OcCompilerIntrinsicsLib.inf
  OpenCorePkg/Library/OcCompressionLib/OcCompressionLib.inf
  OpenCorePkg/Library/OcConfigurationLib/OcConfigurationLib.inf
  OpenCorePkg/Library/OcConsoleControlEntryModeLib/OcConsoleControlEntryModeGenericLib.inf
  OpenCorePkg/Library/OcConsoleControlEntryModeLib/OcConsoleControlEntryModeLocalLib.inf
  OpenCorePkg/Library/OcConsoleLib/OcConsoleLib.inf
  OpenCorePkg/Library/OcCpuLib/OcCpuLib.inf
  OpenCorePkg/Library/OcCryptoLib/OcCryptoLib.inf
  OpenCorePkg/Library/OcDataHubLib/OcDataHubLib.inf
  OpenCorePkg/Library/OcDebugLibSerial/OcDebugLibSerial.inf
  OpenCorePkg/Library/OcDebugLibProtocol/OcDebugLibProtocol.inf
  OpenCorePkg/Library/OcDebugLibNull/OcDebugLibNull.inf
  OpenCorePkg/Library/OcDeviceMiscLib/OcDeviceMiscLib.inf
  OpenCorePkg/Library/OcDevicePathLib/OcDevicePathLib.inf
  OpenCorePkg/Library/OcDevicePropertyLib/OcDevicePropertyLib.inf
  OpenCorePkg/Library/OcDeviceTreeLib/OcDeviceTreeLib.inf
  OpenCorePkg/Library/OcDirectResetLib/OcDirectResetLib.inf
  OpenCorePkg/Library/OcDriverConnectionLib/OcDriverConnectionLib.inf
  OpenCorePkg/Library/OcFileLib/OcFileLib.inf
  OpenCorePkg/Library/OcFirmwarePasswordLib/OcFirmwarePasswordLib.inf
  OpenCorePkg/Library/OcFirmwareVolumeLib/OcFirmwareVolumeLib.inf
  OpenCorePkg/Library/OcFlexArrayLib/OcFlexArrayLib.inf
  OpenCorePkg/Library/OcGuardLib/OcGuardLib.inf
  OpenCorePkg/Library/OcHashServicesLib/OcHashServicesLib.inf
  OpenCorePkg/Library/OcHdaDevicesLib/OcHdaDevicesLib.inf
  OpenCorePkg/Library/OcHeciLib/OcHeciLib.inf
  OpenCorePkg/Library/OcHiiDatabaseLib/OcHiiDatabaseLocalLib.inf
  OpenCorePkg/Library/OcInputLib/OcInputLib.inf
  OpenCorePkg/Library/OcLogAggregatorLib/OcLogAggregatorLib.inf
  OpenCorePkg/Library/OcLogAggregatorLibNull/OcLogAggregatorLibNull.inf
  OpenCorePkg/Library/OcMachoLib/OcMachoLib.inf
  OpenCorePkg/Library/OcMainLib/OcMainLib.inf
  OpenCorePkg/Library/OcMemoryLib/OcMemoryLib.inf
  OpenCorePkg/Library/OcMiscLib/OcMiscLib.inf
  OpenCorePkg/Library/OcMp3Lib/OcMp3Lib.inf
  OpenCorePkg/Library/OcOSInfoLib/OcOSInfoLib.inf
  OpenCorePkg/Library/OcPeCoffExtLib/OcPeCoffExtLib.inf
  OpenCorePkg/Library/OcPngLib/OcPngLib.inf
  OpenCorePkg/Library/OcRngLib/OcRngLib.inf
  OpenCorePkg/Library/OcSerializeLib/OcSerializeLib.inf
  OpenCorePkg/Library/OcSmbiosLib/OcSmbiosLib.inf
  OpenCorePkg/Library/OcSmcLib/OcSmcLib.inf
  OpenCorePkg/Library/OcStorageLib/OcStorageLib.inf
  OpenCorePkg/Library/OcStringLib/OcStringLib.inf
  OpenCorePkg/Library/OcTemplateLib/OcTemplateLib.inf
  OpenCorePkg/Library/OcTimerLib/OcTimerLib.inf
  OpenCorePkg/Library/OcUnicodeCollationEngLib/OcUnicodeCollationEngGenericLib.inf
  OpenCorePkg/Library/OcUnicodeCollationEngLib/OcUnicodeCollationEngLocalLib.inf
  OpenCorePkg/Library/OcPciIoLib/OcPciIoLib.inf
  OpenCorePkg/Library/OcVirtualFsLib/OcVirtualFsLib.inf
  OpenCorePkg/Library/OcWaveLib/OcWaveLib.inf
  OpenCorePkg/Library/OcXmlLib/OcXmlLib.inf
  OpenCorePkg/Legacy/BootPlatform/BiosVideo/BiosVideo.inf
  OpenCorePkg/Platform/CrScreenshotDxe/CrScreenshotDxe.inf
  OpenCorePkg/Platform/OpenCanopy/OpenCanopy.inf
  OpenCorePkg/Platform/OpenLinuxBoot/OpenLinuxBoot.inf
  OpenCorePkg/Platform/OpenNtfsDxe/OpenNtfsDxe.inf
  OpenCorePkg/Platform/OpenPartitionDxe/PartitionDxe.inf
  OpenCorePkg/Platform/OpenRuntime/OpenRuntime.inf
  OpenCorePkg/Platform/OpenUsbKbDxe/UsbKbDxe.inf
  OpenCorePkg/Platform/OpenVariableRuntimeDxe/VariableRuntimeDxe.inf {
    <PcdsFixedAtBuild>
      gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|TRUE
      gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x10000
      gEfiMdeModulePkgTokenSpaceGuid.PcdVariableStoreSize|0x10000
    <LibraryClasses>
      UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
      SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
      AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
      TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
      VarCheckLib|MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
      VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLibRuntimeDxe.inf
      TimerLib|OpenCorePkg/Library/DuetTimerLib/DuetTimerLib.inf
      DebugLib|OpenCorePkg/Library/OcDebugLibNull/OcDebugLibNull.inf
      NULL|OpenCorePkg/Library/OcVariableRuntimeLib/OcVariableRuntimeLib.inf
  }
  OpenCorePkg/Platform/ResetNvramEntry/ResetNvramEntry.inf
  OpenCorePkg/Platform/ToggleSipEntry/ToggleSipEntry.inf
  OpenCorePkg/Staging/AudioDxe/AudioDxe.inf
  OpenCorePkg/Staging/EnableGop/EnableGop.inf {
    <LibraryClasses>
      DebugLib|OpenCorePkg/Library/OcDebugLibNull/OcDebugLibNull.inf
  }
  OpenCorePkg/Staging/EnableGop/EnableGopDirect.inf {
    <LibraryClasses>
      DebugLib|OpenCorePkg/Library/OcDebugLibNull/OcDebugLibNull.inf
  }
  OpenCorePkg/Staging/OpenHfsPlus/OpenHfsPlus.inf
  OpenCorePkg/Tests/AcpiTest/AcpiTest.inf
  OpenCorePkg/Tests/AcpiTest/AcpiTestApp.inf
  OpenCorePkg/Tests/CryptoTest/CryptoTest.inf
  OpenCorePkg/Tests/CryptoTest/CryptoTestApp.inf
  OpenCorePkg/Tests/DataHubTest/DataHubTest.inf
  OpenCorePkg/Tests/DataHubTest/DataHubTestApp.inf
  OpenCorePkg/Tests/PropertyTest/PropertyTest.inf
  OpenCorePkg/Tests/PropertyTest/PropertyTestApp.inf
  OpenCorePkg/Tests/SmbiosTest/SmbiosTest.inf
  OpenCorePkg/Tests/SmbiosTest/SmbiosTestApp.inf
  # UEFI Shell with all commands integrated
  ShellPkg/Application/Shell/Shell.inf {
   <Defines>
      FILE_GUID = EA4BB293-2D7F-4456-A681-1F22F42CD0BC
    <PcdsFixedAtBuild>
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
      # For some reason ShellPkg overrides this, so do we.
      gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize|16000
    <LibraryClasses>
      # Use custom BootServicesTable
      UefiBootServicesTableLib|OpenCorePkg/Library/OcBootServicesTableLib/UefiBootServicesTableLib.inf
      # Add the original commands.
      NULL|ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellNetwork1CommandsLib/UefiShellNetwork1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellNetwork2CommandsLib/UefiShellNetwork2CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellAcpiViewCommandLib/UefiShellAcpiViewCommandLib.inf
  }

  # Ext4 driver
  Ext4Pkg/Ext4Dxe/Ext4Dxe.inf

  #
  # Network Support
  #
  !include NetworkPkg/NetworkComponents.dsc.inc

[LibraryClasses]
  NULL|OpenCorePkg/Library/OcCompilerIntrinsicsLib/OcCompilerIntrinsicsLib.inf

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
  gOpenCorePkgTokenSpaceGuid.PcdCanaryAllowRdtscFallback|TRUE

  # ImageLoader settings
  gEfiMdePkgTokenSpaceGuid.PcdImageLoaderRtRelocAllowTargetMismatch|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdImageLoaderHashProhibitOverlap|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdImageLoaderLoadHeader|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdImageLoaderDebugSupport|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdImageLoaderAllowMisalignedOffset|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdImageLoaderRemoveXForWX|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdImageLoaderWXorX|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdImageLoaderAlignmentPolicy|0xFFFFFFFF
  gEfiMdePkgTokenSpaceGuid.PcdImageLoaderRelocTypePolicy|0xFFFFFFFF

[PcdsPatchableInModule]
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterAccessWidth|8
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseHardwareFlowControl|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialDetectCable|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x03F8
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialBaudRate|115200
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialLineControl|0x03
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialFifoControl|0x07
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialClockRate|1843200
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialPciDeviceInfo|{0xFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialExtendedTxFifoSize|64
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterStride|1
  #
  # Network Pcds
  #
  !include NetworkPkg/NetworkPcds.dsc.inc

[BuildOptions]
  # While there are no PCDs as of now, there at least are some custom macros.
  DEFINE OCPKG_BUILD_OPTIONS_GEN = -D DISABLE_NEW_DEPRECATED_INTERFACES $(OCPKG_BUILD_OPTIONS) -D OC_TARGET_$(TARGET)=1
  DEFINE OCPKG_ANAL_OPTIONS_GEN = "-DANALYZER_UNREACHABLE=__builtin_unreachable" "-DANALYZER_NORETURN=__attribute__((noreturn))"

  GCC:DEBUG_*_*_CC_FLAGS        = $(OCPKG_BUILD_OPTIONS_GEN) $(OCPKG_ANAL_OPTIONS_GEN) -fstack-protector-strong -mstack-protector-guard=global -Wuninitialized
  GCC:NOOPT_*_*_CC_FLAGS        = $(OCPKG_BUILD_OPTIONS_GEN) $(OCPKG_ANAL_OPTIONS_GEN) -fstack-protector-strong -mstack-protector-guard=global -Wuninitialized
  GCC:RELEASE_*_*_CC_FLAGS      = $(OCPKG_BUILD_OPTIONS_GEN) $(OCPKG_ANAL_OPTIONS_GEN) -fstack-protector-strong -mstack-protector-guard=global -Wuninitialized
  CLANGPDB:DEBUG_*_*_CC_FLAGS   = $(OCPKG_BUILD_OPTIONS_GEN) $(OCPKG_ANAL_OPTIONS_GEN) -fstack-protector-strong -mstack-protector-guard=global -ftrivial-auto-var-init=pattern
  CLANGPDB:NOOPT_*_*_CC_FLAGS   = $(OCPKG_BUILD_OPTIONS_GEN) $(OCPKG_ANAL_OPTIONS_GEN) -fstack-protector-strong -mstack-protector-guard=global -ftrivial-auto-var-init=pattern
  CLANGPDB:RELEASE_*_*_CC_FLAGS = $(OCPKG_BUILD_OPTIONS_GEN) $(OCPKG_ANAL_OPTIONS_GEN) -fstack-protector-strong -mstack-protector-guard=global -ftrivial-auto-var-init=pattern
  CLANGGCC:DEBUG_*_*_CC_FLAGS   = $(OCPKG_BUILD_OPTIONS_GEN) $(OCPKG_ANAL_OPTIONS_GEN) -fstack-protector-strong -mstack-protector-guard=global -ftrivial-auto-var-init=pattern
  CLANGGCC:NOOPT_*_*_CC_FLAGS   = $(OCPKG_BUILD_OPTIONS_GEN) $(OCPKG_ANAL_OPTIONS_GEN) -fstack-protector-strong -mstack-protector-guard=global -ftrivial-auto-var-init=pattern
  CLANGGCC:RELEASE_*_*_CC_FLAGS = $(OCPKG_BUILD_OPTIONS_GEN) $(OCPKG_ANAL_OPTIONS_GEN) -fstack-protector-strong -mstack-protector-guard=global -ftrivial-auto-var-init=pattern
  MSFT:DEBUG_*_*_CC_FLAGS       = $(OCPKG_BUILD_OPTIONS_GEN) /wd4723 /GS /kernel
  MSFT:NOOPT_*_*_CC_FLAGS       = $(OCPKG_BUILD_OPTIONS_GEN) /wd4723 /GS /kernel
  MSFT:RELEASE_*_*_CC_FLAGS     = $(OCPKG_BUILD_OPTIONS_GEN) /wd4723 /GS /kernel
  XCODE:DEBUG_*_*_CC_FLAGS      = $(OCPKG_BUILD_OPTIONS_GEN) $(OCPKG_ANAL_OPTIONS_GEN) -fstack-protector-strong -ftrivial-auto-var-init=pattern
  XCODE:NOOPT_*_*_CC_FLAGS      = $(OCPKG_BUILD_OPTIONS_GEN) $(OCPKG_ANAL_OPTIONS_GEN) -fstack-protector-strong -ftrivial-auto-var-init=pattern
  XCODE:RELEASE_*_*_CC_FLAGS    = $(OCPKG_BUILD_OPTIONS_GEN) $(OCPKG_ANAL_OPTIONS_GEN) -Oz -flto -fstack-protector-strong -ftrivial-auto-var-init=pattern

  !include NetworkPkg/NetworkBuildOptions.dsc.inc
