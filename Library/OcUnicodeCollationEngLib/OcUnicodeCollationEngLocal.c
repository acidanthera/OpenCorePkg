/** @file
  Local library constructor routines.

  Copyright (c) 2020, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "OcUnicodeCollationEngInternal.h"
#include <Library/OcBootServicesTableLib.h>

EFI_STATUS
EFIAPI
OcUnicodeCollationEngLocalLibConstructor (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  OcUnicodeCollationUpdatePlatformLanguage ();
  OcUnicodeCollationInitializeMappingTables ();
  OcRegisterBootServicesProtocol (
    &gEfiUnicodeCollation2ProtocolGuid,
    &gInternalUnicode2Eng,
    FALSE
    );

  return EFI_SUCCESS;
}
