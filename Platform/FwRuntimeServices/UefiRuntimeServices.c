/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "FwRuntimeServicesPrivate.h"

#include <Guid/OcVariables.h>
#include <Guid/GlobalVariable.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

/**
  Runtime accessible variables (all read only).
**/
STATIC EFI_GET_TIME                  mStoredGetTime;
STATIC EFI_SET_TIME                  mStoredSetTime;
STATIC EFI_GET_WAKEUP_TIME           mStoredGetWakeupTime;
STATIC EFI_SET_WAKEUP_TIME           mStoredSetWakeupTime;
STATIC EFI_GET_VARIABLE              mStoredGetVariable;
STATIC EFI_GET_NEXT_VARIABLE_NAME    mStoredGetNextVariableName;
STATIC EFI_SET_VARIABLE              mStoredSetVariable;
STATIC EFI_GET_NEXT_HIGH_MONO_COUNT  mStoredGetNextHighMonotonicCount;
STATIC EFI_RESET_SYSTEM              mStoredResetSystem;

/**
  Configurations to use.
**/
OC_FWRT_CONFIG  gMainConfig;
OC_FWRT_CONFIG  gOverrideConfig;
OC_FWRT_CONFIG  *gCurrentConfig;

/**
  Boot phase accessible variables.
**/
STATIC EFI_EVENT                     mTranslateEvent;
STATIC EFI_GET_VARIABLE              mCustomGetVariable;
STATIC BOOLEAN                       mKernelStarted;

/**
  Current boot order before updating.
**/
STATIC UINT16                        mTmpBootOrder[16];
STATIC UINTN                         mTmpBootOrderSize;
STATIC INT32                         mOcBootOrderMap[16];
STATIC UINT8                         mTmpBootOption[512];
STATIC UINTN                         mTmpBootOptionSize;

STATIC
VOID
WriteUnprotectorPrologue (
  OUT BOOLEAN  *Ints,
  OUT BOOLEAN  *Wp
  )
{
  IA32_CR0  Cr0;

  if (gCurrentConfig->WriteUnprotector) {
    *Ints     = SaveAndDisableInterrupts ();
    Cr0.UintN = AsmReadCr0 ();
    if (Cr0.Bits.WP == 1) {
      *Wp = TRUE;
      Cr0.Bits.WP = 0;
      AsmWriteCr0 (Cr0.UintN);
    } else {
      *Wp = FALSE;
    }
  } else {
    *Ints = FALSE;
    *Wp   = FALSE;
  }
}

STATIC
VOID
WriteUnprotectorEpilogue (
  IN BOOLEAN  Ints,
  IN BOOLEAN  Wp
  )
{
  IA32_CR0  Cr0;

  if (gCurrentConfig->WriteUnprotector) {
    if (Wp) {
      Cr0.UintN   = AsmReadCr0 ();
      Cr0.Bits.WP = 1;
      AsmWriteCr0 (Cr0.UintN);
    }

    if (Ints) {
      EnableInterrupts ();
    }
  }
}

STATIC
BOOLEAN
IsEfiBootVar (
  IN   CHAR16    *VariableName,
  IN   EFI_GUID  *VendorGuid,
  OUT  BOOLEAN   *IsBootOrder  OPTIONAL,
  OUT  INT32     *BootNum      OPTIONAL
  )
{
  UINTN   Index;
  CHAR16  Digit;
  INT32   TmpNum;

  if (!CompareGuid (VendorGuid, &gEfiGlobalVariableGuid)) {
    return FALSE;
  }

  if (StrnCmp (L"Boot", VariableName, L_STR_LEN (L"Boot")) != 0) {
    return FALSE;
  }

  if (IsBootOrder != NULL && StrCmp (VariableName, L"BootOrder") == 0) {
    *IsBootOrder = TRUE;
    return TRUE;
  }

  if (BootNum != NULL
    && StrLen (VariableName) == L_STR_LEN (L"Boot####")) {
    TmpNum = 0;

    for (Index = 0; Index < 4; ++Index) {
      Digit = VariableName[L_STR_LEN (L"Boot") + Index];
      if (Digit >= '0' && Digit <= '9') {
        TmpNum = 16 * TmpNum + (INT32) (Digit - '0');
      } else if (Digit >= 'A' && Digit <= 'F') {
        TmpNum = 16 * TmpNum + (INT32) (Digit - 'A') + 10;
      } else {
        return TRUE;
      }
    }

    *BootNum = TmpNum;
  }

  return TRUE;
}

STATIC
BOOLEAN
IsOcBootVar (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid
  )
{
  if (!CompareGuid (VendorGuid, &gOcVendorVariableGuid)) {
    return FALSE;
  }

  if (StrnCmp (L"Boot", VariableName, L_STR_LEN (L"Boot")) != 0) {
    return FALSE;
  }

  return TRUE;
}

STATIC
EFI_STATUS
EFIAPI
WrapGetTime (
  OUT  EFI_TIME               *Time,
  OUT  EFI_TIME_CAPABILITIES  *Capabilities OPTIONAL
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Ints;
  BOOLEAN     Wp;

  WriteUnprotectorPrologue (&Ints, &Wp);

  Status = mStoredGetTime (
    Time,
    Capabilities
    );

  if (!EFI_ERROR (Status)) {
    //
    // On old AMI firmwares (like the one found in GA-Z87X-UD4H) there is a chance
    // of getting 2047 (EFI_UNSPECIFIED_TIMEZONE) from GetTime. This is valid,
    // yet is disliked by some software including but not limited to UEFI Shell.
    // See the patch: https://lists.01.org/pipermail/edk2-devel/2018-May/024534.html
    // As a workaround we make sure this does not happen at all.
    //
    if (Time->TimeZone == EFI_UNSPECIFIED_TIMEZONE) {
      Time->TimeZone = 0;
    }
  }

  WriteUnprotectorEpilogue (Ints, Wp);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
WrapSetTime (
  IN  EFI_TIME  *Time
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Ints;
  BOOLEAN     Wp;

  WriteUnprotectorPrologue (&Ints, &Wp);

  Status = mStoredSetTime (
    Time
    );

  WriteUnprotectorEpilogue (Ints, Wp);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
WrapGetWakeupTime (
  OUT BOOLEAN    *Enabled,
  OUT BOOLEAN    *Pending,
  OUT EFI_TIME   *Time
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Ints;
  BOOLEAN     Wp;

  WriteUnprotectorPrologue (&Ints, &Wp);

  Status = mStoredGetWakeupTime (
    Enabled,
    Pending,
    Time
    );

  WriteUnprotectorEpilogue (Ints, Wp);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
WrapSetWakeupTime (
  IN  BOOLEAN   Enable,
  IN  EFI_TIME  *Time  OPTIONAL
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Ints;
  BOOLEAN     Wp;

  WriteUnprotectorPrologue (&Ints, &Wp);

  Status = mStoredSetWakeupTime (
    Enable,
    Time
    );

  WriteUnprotectorEpilogue (Ints, Wp);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
WrapGetVariable (
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  OUT    UINT32    *Attributes OPTIONAL,
  IN OUT UINTN     *DataSize,
  OUT    VOID      *Data  OPTIONAL
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Ints;
  BOOLEAN     Wp;

  //
  // Perform early checks for speedup.
  //
  if (VariableName == NULL
    || VendorGuid == NULL
    || DataSize == NULL
    || (Data == NULL && *DataSize != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Abort access to write-only variables.
  //
  if (gCurrentConfig->RestrictedVariables
    && mKernelStarted
    && CompareGuid (VendorGuid, &gOcWriteOnlyVariableGuid)) {
    return EFI_SECURITY_VIOLATION;
  }

  //
  // Redirect Boot-prefixed variables to our own GUID.
  //
  if (gCurrentConfig->BootVariableRedirect && IsEfiBootVar (VariableName, VendorGuid, NULL, NULL)) {
    VendorGuid = &gOcVendorVariableGuid;
  }

  WriteUnprotectorPrologue (&Ints, &Wp);

  Status = (mCustomGetVariable != NULL ? mCustomGetVariable : mStoredGetVariable) (
    VariableName,
    VendorGuid,
    Attributes,
    DataSize,
    Data
    );

  WriteUnprotectorEpilogue (Ints, Wp);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
WrapGetNextVariableName (
  IN OUT UINTN     *VariableNameSize,
  IN OUT CHAR16    *VariableName,
  IN OUT EFI_GUID  *VendorGuid
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINTN       Size;
  CHAR16      TempName[256];
  EFI_GUID    TempGuid;
  BOOLEAN     StartBootVar;
  BOOLEAN     Ints;
  BOOLEAN     Wp;

  //
  // Perform initial checks as per spec. Last check is part of:
  // Null-terminator is not found in the first VariableNameSize
  // bytes of the input VariableName buffer.
  //
  if (VariableNameSize == NULL || VariableName == NULL || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Checking that VariableName never exceeds *VariableNameSize and is always null-terminated.
  //
  Size = 0;
  for (Index = 0; Index < *VariableNameSize; ++Index) {
    if (VariableName[Index] == L'\0') {
      Size = (Index + 1) * sizeof (CHAR16);
      break;
    }
  }

  //
  // Also assume that too large variables do not exist, as we cannot work with them anyway.
  //
  if (Size == 0 || Size > sizeof (TempName)) {
    return EFI_INVALID_PARAMETER;
  }

  WriteUnprotectorPrologue (&Ints, &Wp);

  //
  // In case we do not redirect, simply do nothing.
  //
  if (!gCurrentConfig->BootVariableRedirect) {
    Status = mStoredGetNextVariableName (VariableNameSize, VariableName, VendorGuid);
    WriteUnprotectorEpilogue (Ints, Wp);
    return Status;
  }

  //
  // Copy vendor and variable name to internal buffer.
  //
  CopyGuid (&TempGuid, VendorGuid);
  StrnCpyS (TempName, ARRAY_SIZE (TempName), VariableName, StrLen (VariableName));

  StartBootVar = FALSE;

  //
  // In case we are not yet iterating EfiBoot variables,
  // then go through the whole variable list and return
  // variables except EfiBoot.
  //
  if (!IsEfiBootVar (TempName, &TempGuid, NULL, NULL)) {
    while (TRUE) {
      //
      // Request for variables.
      //
      Size   = sizeof (TempName);
      Status = mStoredGetNextVariableName (&Size, TempName, &TempGuid);

      if (!EFI_ERROR (Status)) {
        if (!IsEfiBootVar (TempName, &TempGuid, NULL, NULL)) {
          Size = StrSize (TempName); ///< Not guaranteed to be updated with EFI_SUCCESS.

          if (*VariableNameSize >= Size) {
            //
            // Return this variable.
            //
            CopyGuid (VendorGuid, &TempGuid);
            StrnCpyS (
              VariableName,
              *VariableNameSize / sizeof (CHAR16),
              TempName,
              StrLen (TempName)
              );
            *VariableNameSize = Size; ///< This is NOT explicitly required by the spec.
            Status            = EFI_SUCCESS;
          } else {
            //
            // Request more space.
            //
            *VariableNameSize = Size;
            Status            = EFI_BUFFER_TOO_SMALL;
          }

          WriteUnprotectorEpilogue (Ints, Wp);
          return Status;
        } else {
          //
          // EfiBoot variables are handled outside of this loop.
          //
        }
      } else if (Status == EFI_BUFFER_TOO_SMALL) {
        //
        // This means sizeof (TempName) is too small for this system.
        // At this step we cannot do anything, but let's replace error
        // with something sensible.
        //
        WriteUnprotectorEpilogue (Ints, Wp);
        return EFI_DEVICE_ERROR;
      } else if (Status == EFI_NOT_FOUND) {
        //
        // End of normal variable list.
        // This means it is the time for boot variables to be searched
        // from the beginning.
        //
        StartBootVar = TRUE;
        break;
      } else {
        //
        // We got EFI_UNSUPPORTED, EFI_DEVICE_ERROR or EFI_INVALID_PARAMETER.
        // Return as is.
        //
        WriteUnprotectorEpilogue (Ints, Wp);
        return Status;
      }
    }
  }

  //
  // Handle EfiBoot variables now.
  // When StartBootVar is TRUE we start from the beginning.
  // Otherwise we have boot variable in TempGuid/TempName.
  //

  if (StartBootVar) {
    //
    // It is not required to zero guid, but let's do it just in case.
    //
    TempName[0] = L'\0';
    ZeroMem (&TempGuid, sizeof (TempGuid));
  } else {
    //
    // Switch to real GUID as stored in variable storage.
    //
    CopyGuid (&TempGuid, &gOcVendorVariableGuid);
  }

  //
  // Find next EfiBoot variable.
  //
  while (TRUE) {
    Size   = sizeof (TempName);
    Status = mStoredGetNextVariableName (&Size, TempName, &TempGuid);

    if (!EFI_ERROR (Status)) {
      if (IsOcBootVar (TempName, &TempGuid)) {
        Size = StrSize (TempName); ///< Not guaranteed to be updated with EFI_SUCCESS.

        if (*VariableNameSize >= Size) {
          //
          // Return this variable.
          //
          CopyGuid (VendorGuid, &gEfiGlobalVariableGuid);
          StrnCpyS (
            VariableName,
            *VariableNameSize / sizeof (CHAR16),
            TempName,
            StrLen (TempName)
            );
          *VariableNameSize = Size; ///< This is NOT explicitly required by the spec.
          Status = EFI_SUCCESS;
        } else {
          //
          // Request more space.
          //
          *VariableNameSize = Size;
          Status = EFI_BUFFER_TOO_SMALL;
        }

        WriteUnprotectorEpilogue (Ints, Wp);
        return Status;
      }
    } else {
      //
      // This is some error, but let us not care which and just exit cleanly.
      //
      break;
    }
  }

  //
  // Report this is the end.
  //
  WriteUnprotectorEpilogue (Ints, Wp);
  return EFI_NOT_FOUND;
}

STATIC
VOID
HandleBootOrderFallback (
  UINT16  *NewBootOrder,
  UINTN   NewBootOrderCount,
  UINT16  *OldBootOrder,
  UINTN   OldBootOrderCount
  )
{
  UINTN       Index;
  UINTN       Index2;
  EFI_STATUS  Status;
  INT32       BootNum;

  //
  // Look for BootOrder entry addition, assume only one exists if at all.
  // Ignore deletions, as we can handle them later.
  //
  BootNum = -1;
  for (Index = 0; Index < NewBootOrderCount; ++Index) {
    for (Index2 = 0; Index2 < OldBootOrderCount; ++Index2) {
      if (NewBootOrder[Index] == OldBootOrder[Index2]) {
        break;
      }
    }

    if (Index2 == OldBootOrderCount) {
      BootNum = NewBootOrder[Index];
      break;
    }
  }

  if (BootNum < 0) {
    return;
  }

  //
  // BootNum now contains the variable added to OcVendorVariable BootOrder.
  // Check our pending mapping and submit it to EfiGlobalVariable BootOrder if present.
  //

  for (Index = 0; Index < ARRAY_SIZE (mOcBootOrderMap); ++Index) {
    if (mOcBootOrderMap[Index] >= 0 && mOcBootOrderMap[Index] == BootNum) {
      //
      // Erase from pending order.
      //
      mOcBootOrderMap[Index] = -1;
      BootNum = (INT32) (Index + OC_GL_BOOT_OPTION_START);
      break;
    }
  }

  if (Index == ARRAY_SIZE (mOcBootOrderMap)) {
    //
    // We do not know how to handle this variable, just ignore.
    // Most likely we have already wrote it.
    //
    return;
  }

  //
  // BootNum now contains the variable we want to add EfiGlobalVariable BootOrder.
  // Add it, reusing mTmpBootOrder array.
  //
  mTmpBootOrderSize = sizeof (mTmpBootOrder) - sizeof (mTmpBootOrder[0]);
  Status = mStoredGetVariable (
    L"BootOrder",
    &gEfiGlobalVariableGuid,
    NULL,
    &mTmpBootOrderSize,
    mTmpBootOrder
    );

  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      mTmpBootOrderSize = 0;
    } else {
      //
      // We did not fit BootOrder into our list, just ignore.
      // This should not happen.
      //
      return;
    }
  }

  mTmpBootOrder[mTmpBootOrderSize / sizeof (mTmpBootOrder[0])] = (UINT16) BootNum;
  mTmpBootOrderSize += sizeof (mTmpBootOrder[0]);

  mStoredSetVariable (
    L"BootOrder",
    &gEfiGlobalVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
    mTmpBootOrderSize,
    mTmpBootOrder
    );
}

STATIC
EFI_DEVICE_PATH *
GetBootEntryDevicePath (
  EFI_LOAD_OPTION  *LoadOption,
  UINTN            LoadOptionSize,
  BOOLEAN          ForFallback
  )
{
  UINT8                     *LoadOptionPtr;
  CONST CHAR16              *Description;
  UINTN                     DescriptionSize;
  UINT16                    FilePathListSize;
  EFI_DEVICE_PATH_PROTOCOL  *FilePathList;
  EFI_DEVICE_PATH_PROTOCOL  *CurrNode;
  FILEPATH_DEVICE_PATH      *LastNode;
  UINTN                     Index;
  UINTN                     Index2;
  UINTN                     PathLen;
  UINTN                     CurrLen;

  if (LoadOptionSize < sizeof (*LoadOption)
    || (LoadOption->Attributes & LOAD_OPTION_ACTIVE) == 0
    || (LoadOption->Attributes & LOAD_OPTION_CATEGORY) != LOAD_OPTION_CATEGORY_BOOT) {
    return NULL;
  }

  FilePathListSize = LoadOption->FilePathListLength;

  LoadOptionPtr   = (UINT8 *) (LoadOption + 1);
  LoadOptionSize -= sizeof (*LoadOption);

  if (LoadOption->FilePathListLength > LoadOptionSize) {
    return NULL;
  }

  LoadOptionSize -= FilePathListSize;

  Description     = (CHAR16 *) LoadOptionPtr;
  DescriptionSize = StrnSizeS (Description, (LoadOptionSize / sizeof (CHAR16)));
  if (DescriptionSize > LoadOptionSize) {
    return NULL;
  }

  //
  // Just discard all macOS entries.
  //
  if (ForFallback && StrCmp (Description, L"Mac OS X") == 0) {
    return NULL;
  }

  LoadOptionPtr  += DescriptionSize;
  LoadOptionSize -= DescriptionSize;

  FilePathList = (EFI_DEVICE_PATH_PROTOCOL *)LoadOptionPtr;
  if (!IsDevicePathValid (FilePathList, FilePathListSize)) {
    return NULL;
  }

  //
  // Ignore checks for already stored paths.
  //
  if (!ForFallback) {
    return FilePathList;
  }

  LastNode = NULL;

  for (CurrNode = FilePathList; !IsDevicePathEnd (CurrNode); CurrNode = NextDevicePathNode (CurrNode)) {
    if (DevicePathType (CurrNode) == MEDIA_DEVICE_PATH && DevicePathSubType (CurrNode) == MEDIA_FILEPATH_DP) {
      LastNode = (FILEPATH_DEVICE_PATH *) CurrNode;
    }
  }

  //
  // Also discard entries that do not point to files (or maybe folders).
  //
  if (LastNode == NULL) {
    return NULL;
  }

  //
  // In addition discard several predefined names that are known to be macOS related.
  // There obviously are more, but we are unlikely to see them.
  //
  STATIC CONST CHAR16 *mAppleBootloaders[] = {
    L"boot.efi",
    L"ThorUtil.efi",
    L"MultiUpdater.efi"
  };

  PathLen = OcFileDevicePathNameLen (LastNode);
  for (Index = 0; Index < ARRAY_SIZE (mAppleBootloaders); ++Index) {
    CurrLen = StrLen (mAppleBootloaders[Index]);
    if (PathLen >= CurrLen) {
      Index2 = PathLen - CurrLen;
      if ((Index2 == 0 || LastNode->PathName[Index2 - 1] == L'\\')
        && CompareMem (&LastNode->PathName[Index2], mAppleBootloaders[Index], CurrLen) == 0) {
        return NULL;
      }
    }
  }

  return FilePathList;
}

STATIC
VOID
HandleBootEntryFallback (
  UINT16           OcBootNum,
  EFI_LOAD_OPTION  *LoadOption,
  UINTN            LoadOptionSize
  )
{
  EFI_STATUS       Status;
  UINTN            Index;
  UINTN            Index2;
  CHAR16           BootOption[L_STR_LEN (L"Boot####") + 1];
  INT32            BootNum;
  EFI_DEVICE_PATH  *SelfPath;
  EFI_DEVICE_PATH  *OtherPath;

  //
  // Check if this device path is eligible for fallback entry.
  //
  SelfPath = GetBootEntryDevicePath (LoadOption, LoadOptionSize, TRUE);
  if (SelfPath == NULL) {
    return;
  }

  //
  // Check if this boot entry is not already present.
  //
  mTmpBootOrderSize = sizeof (mTmpBootOrder);
  Status = mStoredGetVariable (
    L"BootOrder",
    &gEfiGlobalVariableGuid,
    NULL,
    &mTmpBootOrderSize,
    mTmpBootOrder
    );

  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      mTmpBootOrderSize = 0;
    } else {
      //
      // We cannot handle this boot order. Should not happen.
      //
      return;
    }
  }

  BootOption[0] = 'B';
  BootOption[1] = 'o';
  BootOption[2] = 'o';
  BootOption[3] = 't';
  BootOption[8] = '\0';

  for (Index = 0; Index < mTmpBootOrderSize / sizeof (mTmpBootOrder[0]); ++Index) {
    BootOption[4] = "0123456789ABCDEF"[((UINT32) mTmpBootOrder[Index] >> 12U) & 0xFU];
    BootOption[5] = "0123456789ABCDEF"[((UINT32) mTmpBootOrder[Index] >>  8U) & 0xFU];
    BootOption[6] = "0123456789ABCDEF"[((UINT32) mTmpBootOrder[Index] >>  4U) & 0xFU];
    BootOption[7] = "0123456789ABCDEF"[((UINT32) mTmpBootOrder[Index]       ) & 0xFU];

    mTmpBootOptionSize = sizeof (mTmpBootOption);
    Status = mStoredGetVariable (
      BootOption,
      &gEfiGlobalVariableGuid,
      NULL,
      &mTmpBootOptionSize,
      mTmpBootOption
      );

    if (EFI_ERROR (Status)) {
      //
      // Assume orphan entry or whatever it is.
      // It can technically not fit, but this is not expected.
      //
      continue;
    }

    OtherPath = GetBootEntryDevicePath ((EFI_LOAD_OPTION *) &mTmpBootOption[0], mTmpBootOptionSize, FALSE);
    if (OtherPath == NULL) {
      //
      // Assume invalid entry or whatever it is, just ignore.
      //
      continue;
    }

    //
    // Check option device paths for equality.
    // In case they match, just ignore this option.
    //
    if (IsDevicePathEqual (SelfPath, OtherPath)) {
      return;
    }
  }

  //
  // We now want to fallback this option, and need to decide where.
  // Check if this boot entry is not written already.
  //
  BootNum = -1;
  for (Index = 0; Index < ARRAY_SIZE (mOcBootOrderMap); ++Index) {
    if (mOcBootOrderMap[Index] >= 0) {
      if ((UINT16) mOcBootOrderMap[Index] == OcBootNum) {
        BootNum = (INT32) (OC_GL_BOOT_OPTION_START + Index);
        break;
      }
    } else if (BootNum < 0) {
      for (Index2 = 0; Index2 < mTmpBootOrderSize / sizeof (mTmpBootOrder[0]); ++Index2) {
        if (mTmpBootOrder[Index2] == OC_GL_BOOT_OPTION_START + Index) {
          break;
        }
      }
      if (Index2 == mTmpBootOrderSize / sizeof (mTmpBootOrder[0])) {
        BootNum = (INT32) (OC_GL_BOOT_OPTION_START + Index);
      }
    }
  }

  //
  // Unless zero or more we cannot write this entry. Should not happen.
  //
  if (BootNum < 0) {
    return;
  }

  //
  // Store the variable.
  //
  mOcBootOrderMap[BootNum - OC_GL_BOOT_OPTION_START] = OcBootNum;
  BootOption[4] = "0123456789ABCDEF"[((UINT32) BootNum >> 12U) & 0xFU];
  BootOption[5] = "0123456789ABCDEF"[((UINT32) BootNum >>  8U) & 0xFU];
  BootOption[6] = "0123456789ABCDEF"[((UINT32) BootNum >>  4U) & 0xFU];
  BootOption[7] = "0123456789ABCDEF"[((UINT32) BootNum       ) & 0xFU];
  mStoredSetVariable (
    BootOption,
    &gEfiGlobalVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
    LoadOptionSize,
    LoadOption
    );
}

STATIC
EFI_STATUS
EFIAPI
WrapSetVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
  )
{
  EFI_STATUS  Status;
  INT32       BootNum;
  BOOLEAN     Ints;
  BOOLEAN     Wp;
  BOOLEAN     IsBootOrder;
  BOOLEAN     DoFallback;

  //
  // Abort access when running with read-only NVRAM.
  //
  if (gCurrentConfig->WriteProtection && (Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
    return EFI_SECURITY_VIOLATION;
  }

  //
  // Abort access to read-only variables.
  //
  if (gCurrentConfig->RestrictedVariables
    && mKernelStarted
    && CompareGuid (VendorGuid, &gOcReadOnlyVariableGuid)) {
    return EFI_SECURITY_VIOLATION;
  }

  //
  // Redirect Boot-prefixed variables to our own GUID.
  //
  DoFallback  = FALSE;
  IsBootOrder = FALSE;
  BootNum     = -1;
  if (gCurrentConfig->BootVariableRedirect
    && IsEfiBootVar (VariableName, VendorGuid, &IsBootOrder, &BootNum)) {
    VendorGuid = &gOcVendorVariableGuid;
    DoFallback = gCurrentConfig->BootVariableFallback;
  }

  WriteUnprotectorPrologue (&Ints, &Wp);

  //
  // Preserve current BootOrder.
  //
  if (DoFallback && IsBootOrder) {
    mTmpBootOrderSize = sizeof (mTmpBootOrder);
    Status = mStoredGetVariable (
      VariableName,
      VendorGuid,
      NULL,
      &mTmpBootOrderSize,
      mTmpBootOrder
      );

    //
    // This should never happen, but if it does, just ignore this BootOrder update
    // as we simply cannot track the difference between current and next.
    //
    if (EFI_ERROR (Status)) {
      if (Status == EFI_NOT_FOUND) {
        mTmpBootOrderSize = 0;
      } else {
        IsBootOrder = FALSE;
      }
    }
  }

  Status = mStoredSetVariable (
    VariableName,
    VendorGuid,
    Attributes,
    DataSize,
    Data
    );

  if (!EFI_ERROR (Status) && DoFallback && Data != NULL && DataSize > 0) {
    //
    // So, we need to give back our changes.
    //
    if (IsBootOrder && OC_TYPE_ALIGNED (UINT16, Data)) {
      HandleBootOrderFallback (
        Data,
        DataSize / sizeof (UINT16),
        mTmpBootOrder,
        mTmpBootOrderSize / sizeof (UINT16)
        );
    } else if (BootNum >= 0) {
      HandleBootEntryFallback (
        (UINT16) BootNum,
        Data,
        DataSize
        );
    }
  }

  WriteUnprotectorEpilogue (Ints, Wp);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
WrapGetNextHighMonotonicCount (
  OUT UINT32  *Count
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Ints;
  BOOLEAN     Wp;

  WriteUnprotectorPrologue (&Ints, &Wp);

  Status = mStoredGetNextHighMonotonicCount (
    Count
    );

  WriteUnprotectorEpilogue (Ints, Wp);

  return Status;
}

STATIC
VOID
EFIAPI
WrapResetSystem (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  )
{
  BOOLEAN  Ints;
  BOOLEAN  Wp;

  WriteUnprotectorPrologue (&Ints, &Wp);

  mStoredResetSystem (
    ResetType,
    ResetStatus,
    DataSize,
    ResetData
    );

  WriteUnprotectorEpilogue (Ints, Wp);
}

EFI_STATUS
EFIAPI
FwOnGetVariable (
  IN  EFI_GET_VARIABLE  GetVariable,
  OUT EFI_GET_VARIABLE  *OrgGetVariable  OPTIONAL
  )
{
  if (mCustomGetVariable != NULL) {
    return EFI_ALREADY_STARTED;
  }

  mCustomGetVariable = GetVariable;

  if (OrgGetVariable != NULL) {
    *OrgGetVariable = mStoredGetVariable;
  }

  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
TranslateAddressesHandler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  gRT->ConvertPointer (0, (VOID **) &mStoredGetTime);
  gRT->ConvertPointer (0, (VOID **) &mStoredSetTime);
  gRT->ConvertPointer (0, (VOID **) &mStoredGetWakeupTime);
  gRT->ConvertPointer (0, (VOID **) &mStoredSetWakeupTime);
  gRT->ConvertPointer (0, (VOID **) &mStoredGetVariable);
  gRT->ConvertPointer (0, (VOID **) &mStoredGetNextVariableName);
  gRT->ConvertPointer (0, (VOID **) &mStoredSetVariable);
  gRT->ConvertPointer (0, (VOID **) &mStoredGetNextHighMonotonicCount);
  gRT->ConvertPointer (0, (VOID **) &mStoredResetSystem);

  gRT->ConvertPointer (0, (VOID **) &gCurrentConfig);
  mCustomGetVariable = NULL;

  //
  // Ideally we do that from ExitBootServices, but VirtualAddressChange is fine as well.
  //
  mKernelStarted     = TRUE;
}

VOID
RedirectRuntimeServices (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  for (Index = 0; Index < ARRAY_SIZE (mOcBootOrderMap); ++Index) {
    mOcBootOrderMap[Index] = -1;
  }

  mStoredGetTime                   = gRT->GetTime;
  mStoredSetTime                   = gRT->SetTime;
  mStoredGetWakeupTime             = gRT->GetWakeupTime;
  mStoredSetWakeupTime             = gRT->SetWakeupTime;
  mStoredGetVariable               = gRT->GetVariable;
  mStoredGetNextVariableName       = gRT->GetNextVariableName;
  mStoredSetVariable               = gRT->SetVariable;
  mStoredGetNextHighMonotonicCount = gRT->GetNextHighMonotonicCount;
  mStoredResetSystem               = gRT->ResetSystem;

  gRT->GetTime                     = WrapGetTime;
  gRT->SetTime                     = WrapSetTime;
  gRT->GetWakeupTime               = WrapGetWakeupTime;
  gRT->SetWakeupTime               = WrapSetWakeupTime;
  gRT->GetVariable                 = WrapGetVariable;
  gRT->GetNextVariableName         = WrapGetNextVariableName;
  gRT->SetVariable                 = WrapSetVariable;
  gRT->GetNextHighMonotonicCount   = WrapGetNextHighMonotonicCount;
  gRT->ResetSystem                 = WrapResetSystem;

  gRT->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 (gRT, gRT->Hdr.HeaderSize, &gRT->Hdr.CRC32);

  Status = gBS->CreateEvent (
    EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
    TPL_CALLBACK,
    TranslateAddressesHandler,
    NULL,
    &mTranslateEvent
    );

  ASSERT_EFI_ERROR (Status);
}
