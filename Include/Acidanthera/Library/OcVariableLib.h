/** @file
  OpenCore Variable library.

  Copyright (c) 2016-2022, Vitaly Cheptsov, Marvin Haeuser, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_VARIABLE_LIB_H
#define OC_VARIABLE_LIB_H

#include <Uefi.h>
#include <Library/OcConfigurationLib.h>
#include <Library/OcStorageLib.h>
#include <Protocol/OcFirmwareRuntime.h>

#define OPEN_CORE_NVRAM_ROOT_PATH  L"NVRAM"

#define OPEN_CORE_NVRAM_FILENAME  L"nvram.plist"

#define OPEN_CORE_NVRAM_FALLBACK_FILENAME  L"nvram.fallback"

#define OPEN_CORE_NVRAM_USED_FILENAME  L"nvram.used"

#define OPEN_CORE_NVRAM_ATTR  (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

#define OPEN_CORE_NVRAM_NV_ATTR  (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE)

#define OPEN_CORE_INT_NVRAM_ATTR  EFI_VARIABLE_BOOTSERVICE_ACCESS

/**
  Initialize the OpenCore variable library. No other function may be called
  before this function has returned.

  @param[in] ForceOcWriteFlash  Whether OC system variables should be forced to
                                be written to flash.
**/
VOID
OcVariableInit (
  IN BOOLEAN  ForceOcWriteFlash
  );

/**
  Sets the value of an OpenCore system variable to the OpenCore vendor GUID.
  If the write is to be performed non-volatile, this function guarantees to not
  request a write if the existing data is identical to the data requested to be
  written.

  @param[in]  VariableName       A Null-terminated string that is the name of the vendor's variable.
                                 Each VariableName is unique for each VendorGuid. VariableName must
                                 contain 1 or more characters. If VariableName is an empty string,
                                 then EFI_INVALID_PARAMETER is returned.
  @param[in]  Attributes         Attributes bitmask to set for the variable.
  @param[in]  DataSize           The size in bytes of the Data buffer. Unless the EFI_VARIABLE_APPEND_WRITE or
                                 EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute is set, a size of zero
                                 causes the variable to be deleted. When the EFI_VARIABLE_APPEND_WRITE attribute is
                                 set, then a SetVariable() call with a DataSize of zero will not cause any change to
                                 the variable value (the timestamp associated with the variable may be updated however
                                 even if no new data value is provided,see the description of the
                                 EFI_VARIABLE_AUTHENTICATION_2 descriptor below. In this case the DataSize will not
                                 be zero since the EFI_VARIABLE_AUTHENTICATION_2 descriptor will be populated).
  @param[in]  Data               The contents for the variable.
  @param[in]  VendorGuid         Variable GUID, defaults to gOcVendorVariableGuid if NULL.

  @retval EFI_SUCCESS            The firmware has successfully stored the variable and its data as
                                 defined by the Attributes.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits, name, and GUID was supplied, or the
                                 DataSize exceeds the maximum allowed.
  @retval EFI_INVALID_PARAMETER  VariableName is an empty string.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
  @retval EFI_WRITE_PROTECTED    The variable in question is read-only.
  @retval EFI_WRITE_PROTECTED    The variable in question cannot be deleted.
  @retval EFI_SECURITY_VIOLATION The variable could not be written due to EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACESS being set,
                                 but the AuthInfo does NOT pass the validation check carried out by the firmware.

  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.
**/
EFI_STATUS
OcSetSystemVariable (
  IN CHAR16    *VariableName,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data,
  IN EFI_GUID  *VendorGuid  OPTIONAL
  );

/**
  Test NVRAM GUID against legacy schema.

  @param[in]  AsciiVariableGuid   Guid to test in ASCII format.
  @param[out] VariableGuid        On success AsciiVariableGuid converted to GUID format.
  @param[in]  Schema              Schema to test against.
  @param[out] SchemaEntry         On success list of allowed variable names for this GUID.

  @result EFI_SUCCESS If at least some variables are allowed under this GUID.
**/
EFI_STATUS
OcProcessVariableGuid (
  IN  CONST CHAR8            *AsciiVariableGuid,
  OUT GUID                   *VariableGuid,
  IN  OC_NVRAM_LEGACY_MAP    *Schema  OPTIONAL,
  OUT OC_NVRAM_LEGACY_ENTRY  **SchemaEntry  OPTIONAL
  );

/**
  Test NVRAM variable name against legacy schema.

  @param[in]  SchemaEntry         List of allowed names.
  @param[in]  VariableGuid        Variable GUID (optional, for debug output only).
  @param[in]  VariableName        Variable name.
  @param[in]  StringFormat        Is VariableName Ascii or Unicode?
**/
BOOLEAN
OcVariableIsAllowedBySchemaEntry (
  IN OC_NVRAM_LEGACY_ENTRY  *SchemaEntry,
  IN EFI_GUID               *VariableGuid OPTIONAL,
  IN CONST VOID             *VariableName,
  IN OC_STRING_FORMAT       StringFormat
  );

/**
  Set NVRAM variable - for internal use at NVRAM setup only.

  @param[in]  AsciiVariableName   Variable name.
  @param[in]  VariableGuid        Variably Guid.
  @param[in]  Attributes          Attributes.
  @param[in]  VariableSize        Data size.
  @param[in]  VariableData        Data.
  @param[in]  SchemaEntry         Optional schema to filter by.
  @param[in]  Overwrite           If TRUE pre-existing variables can be overwritten.
**/
VOID
OcSetNvramVariable (
  IN CONST CHAR8            *AsciiVariableName,
  IN EFI_GUID               *VariableGuid,
  IN UINT32                 Attributes,
  IN UINT32                 VariableSize,
  IN VOID                   *VariableData,
  IN OC_NVRAM_LEGACY_ENTRY  *SchemaEntry OPTIONAL,
  IN BOOLEAN                Overwrite
  );

/**
  Get EFI boot option from specified namespace.

  @param[out] OptionSize      Boot option size.
  @param[in] BootOption       Which boot option to return.
  @param[in] BootGuid         Boot namespace to use (OC or default).

  @retval EFI boot option data.
**/
EFI_LOAD_OPTION *
OcGetBootOptionData (
  OUT UINTN           *OptionSize,
  IN  UINT16          BootOption,
  IN  CONST EFI_GUID  *BootGuid
  );

/**
  Resets selected NVRAM variables and reboots the system.

  @param[in]     PreserveBoot       Should reset preserve Boot### entries.

  @retval EFI_SUCCESS, or error returned by called code.
**/
EFI_STATUS
OcResetNvram (
  IN     BOOLEAN  PreserveBoot
  );

/**
  When compatible protocol is found, disable OpenRuntime NVRAM protection then
  return relevant protocol for subsequent restore, else return NULL.
  Always call OcRestoreNvramProtection to restore normal OpenRuntime operation
  before booting anything, after disabling with this call.

  @retval     Compatible protocol if found and firmware runtime was disabled,
              NULL otherwise.
**/
OC_FIRMWARE_RUNTIME_PROTOCOL *
OcDisableNvramProtection (
  VOID
  );

/**
  Restore OpenRuntime NVRAM protection if it was disabled by a previous call to
  OcDisableNvramProtection.
  Noop when FwRuntime argument is NULL.

  @param[in]     FwRuntime          Firmware runtime protocol or NULL, from previous call to
                                    OcDisableNvramProtection.
**/
VOID
OcRestoreNvramProtection (
  IN OC_FIRMWARE_RUNTIME_PROTOCOL  *FwRuntime
  );

/**
  Perform NVRAM UEFI variable deletion.
**/
VOID
OcDeleteVariables (
  IN BOOLEAN  PreserveBoot
  );

/**
  Process variable result.
**/
typedef enum _OC_PROCESS_VARIABLE_RESULT {
  OcProcessVariableContinue,
  OcProcessVariableRestart,
  OcProcessVariableAbort
} OC_PROCESS_VARIABLE_RESULT;

/**
  Process variable during OcScanVariables.
  Any filtering of which variables to use is done within this function.

  @param[in]     Guid               Variable GUID.
  @param[in]     Name               Variable Name.
  @param[in]     Context            Caller-provided context.

  @retval Indicates whether the scan should continue, restart or abort.
**/
typedef
OC_PROCESS_VARIABLE_RESULT
(EFIAPI *OC_PROCESS_VARIABLE)(
  IN EFI_GUID           *Guid,
  IN CHAR16             *Name,
  IN VOID               *Context OPTIONAL
  );

/**
  Apply function to each NVRAM variable.

  @param[in]     ProcessVariable    Function to apply.
  @param[in]     Context            Caller-provided context.
**/
VOID
OcScanVariables (
  IN OC_PROCESS_VARIABLE  ProcessVariable,
  IN VOID                 *Context
  );

/**
  Get current SIP setting.

  @param[out]     CsrActiveConfig    Returned csr-active-config variable; uninitialised if variable
                                     not found, or other error.
  @param[out]     Attributes         If not NULL, a pointer to the memory location to return the
                                     attributes bitmask for the variable; uninitialised if variable
                                     not found, or other error.

  @retval EFI_SUCCESS, EFI_NOT_FOUND, or other error returned by called code.
**/
EFI_STATUS
OcGetSip (
  OUT UINT32  *CsrActiveConfig,
  OUT UINT32  *Attributes          OPTIONAL
  );

/**
  Set current SIP setting.

  @param[in]      CsrActiveConfig    csr-active-config value to set, or NULL to clear the variable.
  @param[in]      Attributes         Attributes to apply.

  @retval EFI_SUCCESS, EFI_NOT_FOUND, or other error returned by called code.
**/
EFI_STATUS
OcSetSip (
  IN  UINT32  *CsrActiveConfig,
  IN  UINT32  Attributes
  );

/**
  Is SIP enabled?

  @param[in]      GetStatus          Return status from previous OcGetSip or gRT->GetVariable call.
  @param[in]      CsrActiveConfig    csr-active-config value from previous OcGetSip or gRT->GetVariable call.
                                     This value is never used unless GetStatus is EFI_SUCCESS.

  @retval TRUE if SIP should be considered enabled based on the passed values.
**/
BOOLEAN
OcIsSipEnabled (
  IN  EFI_STATUS  GetStatus,
  IN  UINT32      CsrActiveConfig
  );

/**
  Toggle SIP.

  @param[in]      CsrActiveConfig    The csr-active-config value to use to disable SIP, if it was previously enabled.

  @retval TRUE on successful operation.
**/
EFI_STATUS
OcToggleSip (
  IN  UINT32  CsrActiveConfig
  );

/**
  Load emulated NVRAM using installed protocol when present.

  @param[in]  Storage                 OpenCore storage.
  @param[in]  LegacyMap               OpenCore legacy NVRAM map, stating which variables are allowed to be read/written.
  @param[in]  LegacyOverwrite         Whether to overwrite any pre-existing variables found in emulated NVRAM.
  @param[in]  RequestBootVarRouting   Whether OpenCore boot variable routing is enabled.
**/
VOID
OcLoadLegacyNvram (
  IN OC_STORAGE_CONTEXT   *Storage,
  IN OC_NVRAM_LEGACY_MAP  *LegacyMap,
  IN BOOLEAN              LegacyOverwrite,
  IN BOOLEAN              RequestBootVarRouting
  );

/**
  Save to emulated NVRAM using installed protocol when present.
**/
VOID
EFIAPI
OcSaveLegacyNvram (
  VOID
  );

/**
  Reset emulated NVRAM using installed protocol when present.
  If protocol is present, does not return and restarts system.
**/
VOID
EFIAPI
OcResetLegacyNvram (
  VOID
  );

/**
  Switch to fallback emulated NVRAM using installed protocol when present.
**/
VOID
EFIAPI
OcSwitchToFallbackLegacyNvram (
  VOID
  );

/**
  If Required is TRUE set volatile BS-only ShimRetainProtocol variable to 1.

  @param[in]  Required        Is ShimRetainProtocol required.
**/
EFI_STATUS
OcShimRetainProtocol (
  IN BOOLEAN  Required
  );

#endif // OC_VARIABLE_LIB_H
