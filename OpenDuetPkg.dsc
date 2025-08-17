## @file
#  An EFI/Framework Emulation Platform with UEFI HII interface supported.
#
#  Developer's UEFI Emulation. DUET provides an EFI/UEFI IA32/X64 environment on legacy BIOS,
#  to help developing and debugging native EFI/UEFI drivers.
#
#  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution. The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

[Defines]
  PLATFORM_NAME                  = OpenDuetPkg
  PLATFORM_GUID                  = 199E24E0-0989-42AA-87F2-611A8C397E72
  PLATFORM_VERSION               = 0.92
  DSC_SPECIFICATION              = 0x00010006
  OUTPUT_DIRECTORY               = Build/OpenDuetPkg
  SUPPORTED_ARCHITECTURES        = X64|IA32
  BUILD_TARGETS                  = RELEASE|DEBUG|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = OpenCorePkg/OpenDuetPkg.fdf

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  #
  # Entry point
  #
  PeimEntryPoint|OpenCorePkg/Library/OcPeimEntryPoint/PeimEntryPoint.inf
  DxeCoreEntryPoint|OpenCorePkg/Library/OcDxeCoreEntryPoint/DxeCoreEntryPoint.inf
  UefiDriverEntryPoint|OpenCorePkg/Library/OcDriverEntryPoint/UefiDriverEntryPoint.inf
  #
  # Basic
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  !if $(ARCH) == X64
    BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  !else
    BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  !endif
  BaseOverflowLib|MdePkg/Library/BaseOverflowLib/BaseOverflowLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  LzmaDecompressLib|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  #
  # UEFI & PI
  #
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiHiiServicesLib|OpenCorePkg/Library/OcHiiServicesLib/OcHiiServicesLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  VarCheckLib|MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
  VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
  VariableFlashInfoLib|MdeModulePkg/Library/BaseVariableFlashInfoLib/BaseVariableFlashInfoLib.inf
  UefiImageExtraActionLib|MdePkg/Library/BaseUefiImageExtraActionLibNull/BaseUefiImageExtraActionLibNull.inf
  PeCoffLib2|MdePkg/Library/BasePeCoffLib2/BasePeCoffLib2.inf
  ImagePropertiesRecordLib|MdeModulePkg/Library/ImagePropertiesRecordLib/ImagePropertiesRecordLib.inf
  #
  # Generic Modules
  #
  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  #
  # Platform
  #
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  DuetBdsLib|OpenCorePkg/Library/DuetBdsLib/DuetBdsLib.inf
  TimerLib|OpenCorePkg/Library/DuetTimerLib/DuetTimerLib.inf
  #
  # Misc
  #
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  FrameBufferBltLib|MdeModulePkg/Library/FrameBufferBltLib/FrameBufferBltLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  MtrrLib|UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  CpuExceptionHandlerLib|MdeModulePkg/Library/CpuExceptionHandlerLibNull/CpuExceptionHandlerLibNull.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  OcDevicePathLib|OpenCorePkg/Library/OcDevicePathLib/OcDevicePathLib.inf
  OcFileLib|OpenCorePkg/Library/OcFileLib/OcFileLib.inf
  OcGuardLib|OpenCorePkg/Library/OcGuardLib/OcGuardLib.inf
  OcLegacyThunkLib|OpenCorePkg/Library/OcLegacyThunkLib/OcLegacyThunkLib.inf
  OcMemoryLib|OpenCorePkg/Library/OcMemoryLib/OcMemoryLib.inf
  OcMiscLib|OpenCorePkg/Library/OcMiscLib/OcMiscLib.inf
  OcStringLib|OpenCorePkg/Library/OcStringLib/OcStringLib.inf
  OcTextInputLib|OpenCorePkg/Library/OcTextInputLib/OcTextInputLib.inf
  #
  # To save size, use NULL library for DebugLib and ReportStatusCodeLib.
  # If need status code output, do library instance override.
  #
!if ($(TARGET) == RELEASE)
  DebugLib|OpenCorePkg/Library/OcDebugLibNull/OcDebugLibNull.inf
!else
  #
  # Equivalent to DEBUG_ON_SERIAL_PORT in OvmfPkg.
  #
  DebugLib|OpenCorePkg/Library/OcDebugLibSerial/OcDebugLibSerial.inf
!endif
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf

[LibraryClasses.common.DXE_CORE]
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  CpuArchLib|UefiCpuPkg/Library/CpuArchLib/CpuArchLibNull.inf

[LibraryClasses.common.DXE_DRIVER]
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  NestedInterruptTplLib|OvmfPkg/Library/NestedInterruptTplLib/NestedInterruptTplLib.inf
  
[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLibRuntimeDxe.inf

[Components]
  OpenCorePkg/Legacy/BootPlatform/DxeIpl/DxeIpl.inf
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
      DxeServicesTableLib|MdeModulePkg/Library/DxeCoreDxeServicesTableLib/DxeCoreDxeServicesTableLib.inf
      UefiBootServicesTableLib|MdeModulePkg/Library/DxeCoreUefiBootServicesTableLib/DxeCoreUefiBootServicesTableLib.inf
      UefiRuntimeServicesTableLib|MdeModulePkg/Library/DxeCoreUefiRuntimeServicesTableLib/DxeCoreUefiRuntimeServicesTableLib.inf
  }

  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <LibraryClasses>
      OcDirectResetLib|OpenCorePkg/Library/OcDirectResetLib/OcDirectResetLib.inf
      OcFlexArrayLib|OpenCorePkg/Library/OcFlexArrayLib/OcFlexArrayLib.inf
      OcSerializeLib|OpenCorePkg/Library/OcSerializeLib/OcSerializeLib.inf
      OcTemplateLib|OpenCorePkg/Library/OcTemplateLib/OcTemplateLib.inf
      OcVariableLib|OpenCorePkg/Library/OcVariableLib/OcVariableLib.inf
      OcXmlLib|OpenCorePkg/Library/OcXmlLib/OcXmlLib.inf
      NULL|OpenCorePkg/Library/OcVariableRuntimeLib/OcVariableRuntimeLib.inf
  }

  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
    <PcdsPatchableInModule>
      gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|0
      gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|0
  }
  OpenCorePkg/Legacy/BootPlatform/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf {
    <LibraryClasses>
      DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  }
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
  OpenCorePkg/Legacy/BootPlatform/SmbiosGenDxe/SmbiosGen.inf

!if $(TARGET) != NOOPT
  # We will reuse DEBUG EfiLdr in NOOPT build to keep within allotted 0x10000-0x20000 space.
  OpenCorePkg/Legacy/BootPlatform/EfiLdr/EfiLdr.inf
!endif
  OpenCorePkg/Legacy/BootPlatform/BdsDxe/BdsDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }
  UefiCpuPkg/CpuIo2Dxe/CpuIo2Dxe.inf
  OpenCorePkg/Legacy/BootPlatform/CpuDxe/CpuDxe.inf
  OpenCorePkg/Legacy/BootPlatform/8259InterruptControllerDxe/8259.inf {
    <PcdsFixedAtBuild>
      gOpenCorePkgTokenSpaceGuid.Pcd8259LegacyModeMask|0xFFFC
  }
  OpenCorePkg/Legacy/BootPlatform/AcpiResetDxe/Reset.inf
  MdeModulePkg/Universal/Metronome/Metronome.inf

  # Chipset
  PcAtChipsetPkg/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf
  OpenCorePkg/Legacy/BootPlatform/8254TimerDxe/8254Timer.inf
  OpenCorePkg/Legacy/BootPlatform/PciRootBridgeDxe/PciRootBridgeNoEnumeration.inf
  OpenCorePkg/Legacy/BootPlatform/PciBusNoEnumerationDxe/PciBusNoEnumeration.inf
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf

  # Foreign file system support
  FatPkg/EnhancedFatDxe/Fat.inf

  # IDE/AHCI Support
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf

  # Usb Support
  MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
  MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf

  # BiosBootBlock Support
  OpenCorePkg/Legacy/BootPlatform/BlockIoDxe/BlockIoDxe.inf

  # ISA Support
  OvmfPkg/SioBusDxe/SioBusDxe.inf
  MdeModulePkg/Bus/Isa/Ps2KeyboardDxe/Ps2KeyboardDxe.inf
  MdeModulePkg/Bus/Isa/Ps2MouseDxe/Ps2MouseDxe.inf
  OpenCorePkg/Platform/OcTextInputDxe/OcTextInputDxe.inf

  # ACPI Support
  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf

  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }
  OpenCorePkg/Platform/OpenPartitionDxe/PartitionDxe.inf

  # Bios Thunk
  OpenCorePkg/Legacy/BootPlatform/LegacyRegion2Dxe/LegacyRegion2Dxe.inf
  OpenCorePkg/Legacy/BootPlatform/BiosVideo/BiosVideo.inf

[LibraryClasses]
  NULL|MdePkg/Library/IntrinsicLib/IntrinsicLib.inf

[PcdsFeatureFlag]
  gEfiMdeModulePkgTokenSpaceGuid.PcdSupportHiiImageProtocol|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdHiiOsRuntimeSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutUgaSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDevicePathSupportDevicePathFromText|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDevicePathSupportDevicePathToText|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdTurnOffUsbLegacySupport|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdUgaConsumeSupport|FALSE

[PcdsFixedAtBuild]
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVendor|L"Acidanthera"
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareRevision|0x00010010
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x10000
  gEfiMdeModulePkgTokenSpaceGuid.PcdVariableStoreSize|0x10000
!if ($(TARGET) == RELEASE)
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x0
  gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel|0x0
!else
  # DEBUG_ASSERT_ENABLED | DEBUG_PRINT_ENABLED | DEBUG_CODE_ENABLED | CLEAR_MEMORY_ENABLED | ASSERT_DEADLOOP_ENABLED
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2f
  # DEBUG_ERROR | DEBUG_WARN | DEBUG_INFO
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000042
  gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel|0x80000042
!endif
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x0
  gOpenCorePkgTokenSpaceGuid.PcdCanaryAllowRdtscFallback|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdUefiImageFormatSupportFv|0x02
  #
  # Policy required to allow legacy, non-aligned, non-signed Apple images
  # such as HfsPlusLegacy.efi.
  #
  gEfiMdePkgTokenSpaceGuid.PcdImageProtectionPolicy|0x00000003
  #
  # PcdDxeNxMemoryProtectionPolicy and PcdImageLoaderAllowMisalignedOffset
  # settings for Linux EFI stub (same as OvmfPkg LINUX_LOADER settings).
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeNxMemoryProtectionPolicy|0xFFFFFFFFFFFFFF40
  gEfiMdePkgTokenSpaceGuid.PcdImageLoaderAllowMisalignedOffset|TRUE

[BuildOptions]
  MSFT:NOOPT_*_*_CC_FLAGS    = -D OC_TARGET_RELEASE=1 /FAcs -Dinline=__inline /GS /kernel
  MSFT:DEBUG_*_*_CC_FLAGS    = -D OC_TARGET_RELEASE=1 /FAcs -Dinline=__inline /GS /kernel
  MSFT:RELEASE_*_*_CC_FLAGS  = -D OC_TARGET_RELEASE=1 /FAcs -Dinline=__inline -DMDEPKG_NDEBUG /GS /kernel

  XCODE:NOOPT_*_*_CC_FLAGS   = -D OC_TARGET_RELEASE=1 -fno-unwind-tables -O0 -fstack-protector-strong -ftrivial-auto-var-init=pattern
  XCODE:DEBUG_*_*_CC_FLAGS   = -D OC_TARGET_RELEASE=1 -fno-unwind-tables -flto -Os -fstack-protector-strong -ftrivial-auto-var-init=pattern
  XCODE:RELEASE_*_*_CC_FLAGS = -D OC_TARGET_RELEASE=1 -fno-unwind-tables -flto -Os -DMDEPKG_NDEBUG -fstack-protector-strong -ftrivial-auto-var-init=pattern

  GCC:NOOPT_*_*_CC_FLAGS     = -D OC_TARGET_RELEASE=1 -Wno-unused-but-set-variable -fstack-protector-strong -mstack-protector-guard=global -Wuninitialized
  GCC:DEBUG_*_*_CC_FLAGS     = -D OC_TARGET_RELEASE=1 -Wno-unused-but-set-variable -fstack-protector-strong -mstack-protector-guard=global -Wuninitialized
  GCC:RELEASE_*_*_CC_FLAGS   = -D OC_TARGET_RELEASE=1 -DMDEPKG_NDEBUG -Wno-unused-but-set-variable -fstack-protector-strong -mstack-protector-guard=global -Wuninitialized

  CLANGGCC:NOOPT_*_*_CC_FLAGS     = -D OC_TARGET_RELEASE=1 -Wno-unused-but-set-variable -fstack-protector-strong -mstack-protector-guard=global -Wuninitialized
  CLANGGCC:DEBUG_*_*_CC_FLAGS     = -D OC_TARGET_RELEASE=1 -Wno-unused-but-set-variable -fstack-protector-strong -mstack-protector-guard=global -Wuninitialized
  CLANGGCC:RELEASE_*_*_CC_FLAGS   = -D OC_TARGET_RELEASE=1 -DMDEPKG_NDEBUG -Wno-unused-but-set-variable -fstack-protector-strong -mstack-protector-guard=global -Wuninitialized
