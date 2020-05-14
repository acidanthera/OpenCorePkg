/** @file
  Normal library constructor routines.

  Copyright (c) 2020, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "OcUnicodeCollationEngInternal.h"
#include <Library/OcUnicodeCollationEngGenericLib.h>

EFI_UNICODE_COLLATION_PROTOCOL *
OcUnicodeCollationEngInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS                      Status;
  EFI_UNICODE_COLLATION_PROTOCOL  *Existing;
  EFI_HANDLE                      NewHandle;

  OcUnicodeCollationUpdatePlatformLanguage ();
  OcUnicodeCollationInitializeMappingTables ();

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gEfiUnicodeCollation2ProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCUC: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
      &gEfiUnicodeCollation2ProtocolGuid,
      NULL,
      (VOID **) &Existing
      );

    if (!EFI_ERROR (Status)) {
      //
      // We do not need to install an existing collation.
      //
      return Existing;
    }
  }

  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &NewHandle,
    &gEfiUnicodeCollation2ProtocolGuid,
    &gInternalUnicode2Eng,
    NULL
    );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &gInternalUnicode2Eng;
}
