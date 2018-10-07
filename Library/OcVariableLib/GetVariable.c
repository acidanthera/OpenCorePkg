/** @file
  Copyright (C) 2016 - 2017, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/OcStringLib.h>

// OcGetVariable
/** Return the variables value in buffer.

  @param[in] Name           A Null-terminated Ascii string that is the name of the vendor’s variable. Each Name is
                            unique for each VendorGuid.
                            Name must contain 1 or more Ascii characters. If Name is an empty Ascii string, then
                            EFI_INVALID_PARAMETER is returned.
  @param[in] VendorGuid     A unique identifier for the vendor.
  @param[in,out] BufferPtr  Pointer to store the buffer.
  @param[in,out] BufferSize Size of the variables data buffer.

  @retval  A pointer to the variables data
**/
EFI_STATUS
OcGetVariable (
  IN  CHAR8       *Name,
  IN  EFI_GUID    *VendorGuid,
  IN OUT VOID     **BufferPtr,
  IN OUT UINTN    *BufferSize
  )
{
  EFI_STATUS Status;
  VOID       *TempBuffer;
  CHAR16     *VariableName;

  Status = EFI_INVALID_PARAMETER;

  if (Name == NULL || VendorGuid == NULL || BufferPtr == NULL || BufferSize == NULL) {
    return Status;
  }

  VariableName  = AllocateZeroPool (AsciiStrSize (Name) * sizeof(CHAR16));

  if (VariableName) {

    OcAsciiStrToUnicode (Name, VariableName, 0);

    Status = gRT->GetVariable (
                    VariableName,
                    VendorGuid,
                    NULL,
                    BufferSize,
                    *BufferPtr
                    );

    if (EFI_ERROR (Status)) {

      if ((Status == EFI_BUFFER_TOO_SMALL) && (*BufferPtr == NULL)) {

        Status     = EFI_OUT_OF_RESOURCES;
        TempBuffer = AllocateZeroPool (*BufferSize);

        if (TempBuffer != NULL) {

          Status  = gRT->GetVariable (
                           VariableName,
                           VendorGuid,
                           NULL,
                           BufferSize,
                           TempBuffer
                           );

          if (!EFI_ERROR (Status)) {
            *BufferPtr  = TempBuffer;
          } else {
            FreePool (TempBuffer);
          }
        }
      }
    }

    FreePool (VariableName);
  }

  return Status;
}


// GetVariableAndSize
/** Return the variables value in buffer.

  @param[in] Name           A Null-terminated Unicode string that is the name of the vendor’s variable. Each Name is
                            unique for each VendorGuid.
                            Name must contain 1 or more Unicode characters. If Name is an empty Unicode string, then
                            EFI_INVALID_PARAMETER is returned.
  @param[in] VendorGuid     A unique identifier for the vendor.
  @param[out] VariableSize  Size of the variables data buffer.

  @retval  A pointer to the variables data
**/
