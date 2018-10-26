/** @file
  Copyright (C) 2016 - 2018, The HermitCrabs Lab. All rights reserved.

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
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Macros.h>

// SetVariable
/** Sets an EFI Variable.

  @param[in] Name            A Null-terminated Unicode string that is the name of the vendor’s variable. Each Name is
                             unique for each VendorGuid.
                             Name must contain 1 or more Unicode characters. If Name is an empty Unicode string, then
                             EFI_INVALID_PARAMETER is returned.
  @param[in] VendorGuid      A unique identifier for the vendor.
  @param[in] Attributes      Attributes bitmask to set for the variable. Refer to the GetVariable() function
                             description.
  @param[in] DataSize        The size in bytes of the Data buffer. A size of zero causes the variable to be deleted.
  @param[in] Data            The contents for the variable.
  @param[in] OverideDefault  A boolean flag which enables updating a previously set value.

  @retval EFI_SUCCESS  The variable was set successfully.
**/
EFI_STATUS
SetVariable (
  IN CHAR8     *Name,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data,
  IN BOOLEAN   OverideDefault
  )
{
  EFI_STATUS Status;

  UINTN      Size;
  CHAR16     *VariableName;

  Status        = EFI_OUT_OF_RESOURCES;
  VariableName  = AllocateZeroPool (AsciiStrSize (Name) * sizeof (CHAR16));

  if (VariableName != NULL) {

    OcAsciiStrToUnicode (Name, VariableName, 0);

    Size   = 0;
    Status = gRT->GetVariable (
                    VariableName,
                    VendorGuid,
                    NULL,
                    &Size,
                    NULL
                    );

    if (Status == EFI_BUFFER_TOO_SMALL && !OverideDefault) {
      Status = EFI_WRITE_PROTECTED;
    } else {
      CHAR8   *DataString;
      CHAR8   *AttributeString;
      UINTN   AttributeStringSize;

      Status = gRT->SetVariable (
                      VariableName,
                      VendorGuid,
                      Attributes,
                      DataSize,
                      Data
                      );

      AttributeStringSize = 32;
      AttributeString     = AllocateZeroPool (AttributeStringSize);

      if (AttributeString != NULL) {
        // Convert Attributes into text string

        if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
          AsciiStrCatS (AttributeString, AttributeStringSize, "+NV");
        }

        if ((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) != 0) {
          AsciiStrCatS (AttributeString, AttributeStringSize, "+RT+BS");
        } else if ((Attributes & EFI_VARIABLE_BOOTSERVICE_ACCESS) != 0) {
          AsciiStrCatS (AttributeString, AttributeStringSize, "+BS");
        }

        if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) {
          AsciiStrCatS (AttributeString, AttributeStringSize, "+HR");
        }

        if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
          AsciiStrCatS (AttributeString, AttributeStringSize, "+AW");
        }

        if ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) {
          AsciiStrCatS (AttributeString, AttributeStringSize, "+AT");
        }

        // Remove leading +
        if (AttributeString[0] == '+') {
          CopyMem (AttributeString, (AttributeString + 1), AsciiStrSize (AttributeString + 1));
        }

        DataString = ConvertDataToString (Data, DataSize);

        if (DataString != NULL) {
          DEBUG ((
            DEBUG_VARIABLE,
            "Setting %a %g:%s = %a (%d)\n",
            AttributeString,
            VendorGuid,
            VariableName,
            DataString,
            DataSize
            ));

          FreePool ((VOID *)DataString);
        } else {
          DEBUG ((
            DEBUG_VARIABLE,
            "Setting %a %g:%s (%d)\n",
            AttributeString,
            VendorGuid,
            VariableName,
            DataSize
            ));
        }

        FreePool ((VOID *)AttributeString);
      } else {
        Status = EFI_OUT_OF_RESOURCES;
      }

    } 

  }

  return Status;
}
