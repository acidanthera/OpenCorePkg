/** @file
  Copyright (C) 2017-2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcSmcLib.h>
#include <Library/OcRtcLib.h>

#include "OcSmcLibInternal.h"

STATIC
EFI_EVENT
mAuthenticationKeyEraseEvent;

STATIC
VIRTUALSMC_KEY_VALUE
mVirtualSmcKeyValue[6] = {
  { SMC_KEY_KEY, SmcKeyTypeUint32, 4, SMC_KEY_ATTRIBUTE_READ, {0, 0, 0, ARRAY_SIZE (mVirtualSmcKeyValue)} },
  { SMC_KEY_RMde, SmcKeyTypeChar, 1, SMC_KEY_ATTRIBUTE_READ, {SMC_MODE_APPCODE} },
  //
  // Requested yet unused (battery inside, causes missing battery in UI).
  //
  //{ SMC_KEY_BBIN, SmcKeyTypeFlag, 1, SMC_KEY_ATTRIBUTE_READ, {0} },
  { SMC_KEY_BRSC, SmcKeyTypeUint16, 2, SMC_KEY_ATTRIBUTE_READ, {0} },
  { SMC_KEY_MSLD, SmcKeyTypeUint8, 1, SMC_KEY_ATTRIBUTE_READ, {0} },
  { SMC_KEY_BATP, SmcKeyTypeFlag, 1, SMC_KEY_ATTRIBUTE_READ, {0} },
  //
  // HBKP must always be the last key in the list (see mAuthenticationKeyIndex).
  //
  { SMC_KEY_HBKP, SmcKeyTypeCh8s, 32, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE, {0} }
};

STATIC
CONST UINT8
mAuthenticationKeyIndex = ARRAY_SIZE (mVirtualSmcKeyValue) - 1;

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcReadValue (
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY                Key,
  IN  SMC_DATA_SIZE          Size,
  OUT SMC_DATA               *Value
  )
{
  UINTN      Index;

  DEBUG ((DEBUG_INFO, "OCSMC: SmcReadValue Key %X Size %d\n", Key, Size));

  if (Value == NULL) {
    return EFI_SMC_BAD_PARAMETER;
  }

  for (Index = 0; Index < ARRAY_SIZE (mVirtualSmcKeyValue); Index++) {
    if (mVirtualSmcKeyValue[Index].Key == Key) {
      if (mVirtualSmcKeyValue[Index].Size != Size) {
        return EFI_SMC_KEY_MISMATCH;
      }

      CopyMem (Value, mVirtualSmcKeyValue[Index].Data, Size);

      return EFI_SMC_SUCCESS;
    }
  }

  return EFI_SMC_NOT_FOUND;
}

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcWriteValue (
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY                Key,
  IN  SMC_DATA_SIZE          Size,
  IN  SMC_DATA               *Value
  )
{
  UINTN       Index;

  DEBUG ((DEBUG_INFO, "OCSMC: SmcWriteValue Key %X Size %d\n", Key, Size));

  if (!Value || Size == 0) {
    return EFI_SMC_BAD_PARAMETER;
  }

  //
  // Handle HBKP separately to let boot.efi erase its contents as early as it wants.
  //
  if (Key == SMC_KEY_HBKP && Size <= SMC_HBKP_SIZE) {
    SecureZeroMem (mVirtualSmcKeyValue[mAuthenticationKeyIndex].Data, SMC_HBKP_SIZE);
    CopyMem (mVirtualSmcKeyValue[mAuthenticationKeyIndex].Data, Value, Size);
    for (Index = 0; Index < SMC_HBKP_SIZE; Index++) {
      if (mVirtualSmcKeyValue[mAuthenticationKeyIndex].Data[Index] != 0) {
        DEBUG ((DEBUG_INFO, "OCSMC: Not updating key with non-zero data\n"));
        break;
      }
    }

    return EFI_SUCCESS;
  }
  return EFI_SMC_NOT_WRITABLE;
}

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcMakeKey (
  IN  CHAR8    *Name,
  OUT SMC_KEY  *Key
  )
{
  UINTN      Index;

  DEBUG ((DEBUG_VERBOSE, "OCSMC: SmcIoVirtualSmcMakeKey\n"));

  if (Name != NULL && Key != NULL) {
    *Key  = 0;
    Index = 0;

    do {
      if (SMC_KEY_IS_VALID_CHAR (Name[Index])) {
        *Key <<= 8;
        *Key  |= Name[Index];
        ++Index;
      } else {
        *Key = 0;
        return EFI_SMC_BAD_PARAMETER;
      }
    } while (Index < sizeof (*Key) / sizeof (*Name));

    return EFI_SMC_SUCCESS;
  }

  return EFI_SMC_BAD_PARAMETER;
}

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcGetKeyCount (
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  OUT UINT32                 *Count
  )
{
  DEBUG ((
    DEBUG_VERBOSE,
    "OCSMC: SmcIoVirtualSmcGetKeyCount %u\n",
    (UINT32) ARRAY_SIZE (mVirtualSmcKeyValue)
    ));

  if (Count == NULL) {
    return EFI_SMC_BAD_PARAMETER;
  }

  *Count = SwapBytes32 (ARRAY_SIZE (mVirtualSmcKeyValue));

  return EFI_SMC_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcGetKeyFromIndex (
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY_INDEX          Index,
  OUT SMC_KEY                *Key
  )
{
  DEBUG ((DEBUG_VERBOSE, "OCSMC: SmcIoVirtualSmcGetKeyFromIndex\n"));

  if (Key == NULL) {
    return EFI_SMC_BAD_PARAMETER;
  }

  if (Index < ARRAY_SIZE (mVirtualSmcKeyValue)) {
    *Key = mVirtualSmcKeyValue[Index].Key;
    return EFI_SMC_SUCCESS;
  }

  return EFI_SMC_NOT_FOUND;
}

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcGetKeyInfo (
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY                Key,
  OUT SMC_DATA_SIZE          *Size,
  OUT SMC_KEY_TYPE           *Type,
  OUT SMC_KEY_ATTRIBUTES     *Attributes
  )
{
  UINTN Index;

  DEBUG ((DEBUG_VERBOSE, "OCSMC: SmcIoVirtualSmcGetKeyFromIndex %X\n", Key));

  if (Size == NULL || Type == NULL || Attributes == NULL) {
    return EFI_SMC_BAD_PARAMETER;
  }

  for (Index = 0; Index < ARRAY_SIZE (mVirtualSmcKeyValue); Index++) {
    if (mVirtualSmcKeyValue[Index].Key == Key) {
      *Size = mVirtualSmcKeyValue[Index].Size;
      *Type = mVirtualSmcKeyValue[Index].Type;
      *Attributes = mVirtualSmcKeyValue[Index].Attributes;
      return EFI_SMC_SUCCESS;
    }
  }

  return EFI_SMC_NOT_FOUND;
}

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcReset (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINT32                 Mode
  )
{
  DEBUG ((DEBUG_VERBOSE, "OCSMC: SmcIoVirtualSmcReset %X\n", Mode));

  return EFI_SMC_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcFlashType (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN SMC_FLASH_TYPE         Type
  )
{
  DEBUG ((DEBUG_VERBOSE, "OCSMC: SmcIoVirtualSmcFlashType %X\n", Type));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnsupported (
  VOID
  )
{
  DEBUG ((DEBUG_VERBOSE, "OCSMC: SmcIoVirtualSmcUnsupported\n"));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcFlashWrite (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINT32                 Unknown,
  IN SMC_FLASH_SIZE         Size,
  IN SMC_DATA               *Data
  )
{
  DEBUG ((DEBUG_VERBOSE, "OCSMC: SmcIoVirtualSmcFlashWrite %d\n", Size));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcFlashAuth (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN SMC_FLASH_SIZE         Size,
  IN SMC_DATA               *Data
  )
{
  DEBUG ((DEBUG_VERBOSE, "OCSMC: SmcIoVirtualSmcFlashAuth %d\n", Size));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnknown1 (
  VOID
  )
{
  DEBUG ((DEBUG_VERBOSE, "OCSMC: SmcIoVirtualSmcUnknown1\n"));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnknown2 (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINTN                  Ukn1,
  IN UINTN                  Ukn2
  )
{
  DEBUG ((DEBUG_VERBOSE, "OCSMC: SmcIoVirtualSmcUnknown2\n"));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnknown3 (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINTN                  Ukn1,
  IN UINTN                  Ukn2
  )
{
  DEBUG ((DEBUG_VERBOSE, "OCSMC: SmcIoVirtualSmcUnknown3\n"));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnknown4 (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINTN                  Ukn1
  )
{
  DEBUG ((DEBUG_VERBOSE, "OCSMC: SmcIoVirtualSmcUnknown4\n"));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

STATIC
EFI_STATUS
EFIAPI
SmcIoVirtualSmcUnknown5 (
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINT8                  *Data
  )
{
  DEBUG ((DEBUG_VERBOSE, "OCSMC: SmcIoVirtualSmcUnknown5\n"));

  return EFI_SMC_UNSUPPORTED_FEATURE;
}

STATIC APPLE_SMC_IO_PROTOCOL mSmcIoProtocol = {
  APPLE_SMC_IO_PROTOCOL_REVISION,
  SmcIoVirtualSmcReadValue,
  SmcIoVirtualSmcWriteValue,
  SmcIoVirtualSmcGetKeyCount,
  SmcIoVirtualSmcMakeKey,
  SmcIoVirtualSmcGetKeyFromIndex,
  SmcIoVirtualSmcGetKeyInfo,
  SmcIoVirtualSmcReset,
  SmcIoVirtualSmcFlashType,
  SmcIoVirtualSmcUnsupported,
  SmcIoVirtualSmcFlashWrite,
  SmcIoVirtualSmcFlashAuth,
  0,
  SMC_PORT_BASE,
  FALSE,
  SmcIoVirtualSmcUnknown1,
  SmcIoVirtualSmcUnknown2,
  SmcIoVirtualSmcUnknown3,
  SmcIoVirtualSmcUnknown4,
  SmcIoVirtualSmcUnknown5
};

STATIC
VOID
EFIAPI
EraseAuthenticationKey (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  SecureZeroMem (mVirtualSmcKeyValue[mAuthenticationKeyIndex].Data, SMC_HBKP_SIZE);
}

STATIC
BOOLEAN
ExtractAuthentificationKey (
  UINT8   *Buffer,
  UINT32  Size
  )
{
  UINT8           Index;
  AES_CONTEXT     Context;
  UINT8           EncryptKey[CONFIG_AES_KEY_SIZE];
  CONST UINT8     *InitVector;
  UINT8           *Payload;
  UINT32          PayloadSize;
  UINT32          RealSize;

  if (Size < sizeof (UINT32) + SMC_HBKP_SIZE) {
    DEBUG ((DEBUG_INFO, "OCSMC: Invalid key length - %u\n", (UINT32) Size));
    return FALSE;
  }

  if (Buffer[0] == 'V' && Buffer[1] == 'S' && Buffer[2] == 'P' && Buffer[3] == 'T') {
    //
    // Perform an as-is copy of stored contents.
    //
    CopyMem (mVirtualSmcKeyValue[mAuthenticationKeyIndex].Data, Buffer + sizeof (UINT32), SMC_HBKP_SIZE);
  } else if (Buffer[0] == 'V' && Buffer[1] == 'S' && Buffer[2] == 'E' && Buffer[3] == 'N') {
    //
    // The magic is followed by an IV and at least one AES block containing at least SMC_HBKP_SIZE bytes.
    //
    if (Size < sizeof (UINT32) + AES_BLOCK_SIZE + SMC_HBKP_SIZE || (Size - sizeof (UINT32)) % AES_BLOCK_SIZE != 0) {
      DEBUG ((DEBUG_INFO, "OCSMC: Invalid encrypted key length - %u\n", (UINT32) Size));
      return FALSE;
    }

    //
    // Read and erase the temporary encryption key from CMOS memory.
    //
    for (Index = 0; Index < sizeof (EncryptKey); Index++) {
      EncryptKey[Index] = OcRtcRead (0xD0 + Index);
      OcRtcWrite (0xD0 + Index, 0);
    }

    //
    // Perform the decryption.
    //
    InitVector   = Buffer + sizeof (UINT32);
    Payload      = Buffer + sizeof (UINT32) + AES_BLOCK_SIZE;
    PayloadSize  = Size - (sizeof (UINT32) + AES_BLOCK_SIZE);
    AesInitCtxIv (&Context, EncryptKey, InitVector);
    AesCbcDecryptBuffer (&Context, Payload, PayloadSize);
    SecureZeroMem (&Context, sizeof (Context));
    SecureZeroMem (EncryptKey, sizeof (EncryptKey));
    RealSize = *(const UINT32 *)Payload;

    //
    // Ensure the size matches SMC_HBKP_SIZE.
    //
    if (RealSize != SMC_HBKP_SIZE) {
      DEBUG ((DEBUG_INFO, "OCSMC: Invalid decrypted key length - %d\n", RealSize));
      return FALSE;
    }

    //
    // Copy the decrypted contents.
    //
    CopyMem (mVirtualSmcKeyValue[mAuthenticationKeyIndex].Data, Payload + sizeof (UINT32), SMC_HBKP_SIZE);
  } else {
    DEBUG ((DEBUG_INFO, "OCSMC: Invalid key magic - %02X %02X %02X %02X\n", Buffer[0], Buffer[1], Buffer[2], Buffer[3]));
    return FALSE;
  }

  return TRUE;
}

STATIC
VOID
LoadAuthenticationKey (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *Buffer = NULL;
  UINT32      Attributes = 0;
  UINTN       Size = 0;

  //
  // Load encryption key contents.
  //
  Status = gRT->GetVariable (VIRTUALSMC_ENCRYPTION_KEY, &gOcWriteOnlyVariableGuid, &Attributes, &Size, NULL);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Buffer = AllocateZeroPool (Size);
    if (Buffer != NULL) {
      Status = gRT->GetVariable (VIRTUALSMC_ENCRYPTION_KEY, &gOcWriteOnlyVariableGuid, &Attributes, &Size, Buffer);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OCSMC: Layer key (%u, %X) obtain failure - %r\n", (UINT32) Size, Attributes, Status));
      }
    } else {
      DEBUG ((DEBUG_INFO, "OCSMC: Key buffer (%u) allocation failure - %r\n", (UINT32) Size, Status));
      Status = EFI_OUT_OF_RESOURCES;
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCSMC: Initial key obtain failure - %r\n", Status));
  }

  //
  // Nullify or (at least) remove existing vsmc-key variable.
  //
  if (Buffer != NULL) {
    //
    // Parse encryption contents if any.
    //
    if (!EFI_ERROR (Status)) {
      ExtractAuthentificationKey (Buffer, (UINT32)Size);
    }

    SecureZeroMem (Buffer, Size);
    Status = gRT->SetVariable (VIRTUALSMC_ENCRYPTION_KEY, &gOcWriteOnlyVariableGuid, Attributes, Size, Buffer);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCSMC: Failed to zero key - %r\n", Status));
      Status = gRT->SetVariable (VIRTUALSMC_ENCRYPTION_KEY, &gOcWriteOnlyVariableGuid, 0, 0, NULL);
    }
    gBS->FreePool (Buffer);
    Buffer = NULL;
  } else {
    Status = gRT->SetVariable (VIRTUALSMC_ENCRYPTION_KEY, &gOcWriteOnlyVariableGuid, 0, 0, NULL);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCSMC: Failed to remove key - %r\n", Status));
  }

  //
  // Erase local HBKP key copy at exit boot services.
  //
  Status = gBS->CreateEvent (
    EVT_SIGNAL_EXIT_BOOT_SERVICES,
    TPL_NOTIFY,
    EraseAuthenticationKey,
    NULL,
    &mAuthenticationKeyEraseEvent
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCSMC: Failed to create exit bs event for hbkp erase\n"));
  }
}

STATIC
VOID
ExportStatusKey (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT32      StatusBuffer[2];
  UINT8       *StatusBufferMagic;

  StatusBufferMagic = (UINT8 *) &StatusBuffer[0];

  //
  // Build the structure.
  //
  StatusBufferMagic[0] = 'V';
  StatusBufferMagic[1] = 'S';
  StatusBufferMagic[2] = 'M';
  StatusBufferMagic[3] = 'C';
  StatusBuffer[1] = 1;

  //
  // Set status key for kext frontend.
  //
  Status = gRT->SetVariable (
    VIRTUALSMC_STATUS_KEY,
    &gOcReadOnlyVariableGuid,
    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
    sizeof (StatusBuffer),
    StatusBuffer
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCSMC: Failed to create status - %r\n", Status));
  }
}

APPLE_SMC_IO_PROTOCOL *
OcSmcIoInstallProtocol (
  IN BOOLEAN  Reinstall,
  IN BOOLEAN  AuthRestart
  )
{
  EFI_STATUS             Status;
  APPLE_SMC_IO_PROTOCOL  *Protocol;

  if (AuthRestart) {
    LoadAuthenticationKey ();
    ExportStatusKey ();
  }

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gAppleSmcIoProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCSMC: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
      &gAppleSmcIoProtocolGuid,
      NULL,
      (VOID *) &Protocol
      );

    if (!EFI_ERROR (Status)) {
      return Protocol;
    }
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
     &gImageHandle,
     &gAppleSmcIoProtocolGuid,
     (VOID *) &mSmcIoProtocol,
     NULL
     );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &mSmcIoProtocol;
}
