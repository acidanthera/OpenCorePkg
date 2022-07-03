/** @file
  Manage Apple SIP variable csr-active-config.

  Copyright (C) 2022, mikebeaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <IndustryStandard/AppleCsrConfig.h>
#include <Guid/AppleVariable.h>
#include <Library/OcVariableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

EFI_STATUS
OcGetSip (
  OUT UINT32  *CsrActiveConfig,
  OUT UINT32  *Attributes          OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       DataSize;

  ASSERT (CsrActiveConfig != NULL);

  DataSize = sizeof (*CsrActiveConfig);

  Status = gRT->GetVariable (
                  L"csr-active-config",
                  &gAppleBootVariableGuid,
                  Attributes,
                  &DataSize,
                  CsrActiveConfig
                  );

  return Status;
}

EFI_STATUS
OcSetSip (
  IN  UINT32  *CsrActiveConfig,
  IN  UINT32  Attributes
  )
{
  EFI_STATUS  Status;

  Status = gRT->SetVariable (
                  L"csr-active-config",
                  &gAppleBootVariableGuid,
                  Attributes,
                  CsrActiveConfig == NULL ? 0 : sizeof (*CsrActiveConfig),
                  CsrActiveConfig
                  );

  return Status;
}

BOOLEAN
OcIsSipEnabled (
  IN  EFI_STATUS  GetStatus,
  IN  UINT32      CsrActiveConfig
  )
{
  ASSERT (GetStatus == EFI_NOT_FOUND || !EFI_ERROR (GetStatus));

  //
  // CSR_ALLOW_APPLE_INTERNAL with no other bits set reports as SIP enabled
  // (and is used as the enable value in some Apl setups)
  //
  return GetStatus == EFI_NOT_FOUND || (CsrActiveConfig & ~CSR_ALLOW_APPLE_INTERNAL) == 0;
}

EFI_STATUS
OcToggleSip (
  IN  UINT32  CsrActiveConfig
  )
{
  EFI_STATUS  Status;
  UINT32      Attributes;
  UINT32      CsrConfig;

  //
  // Use existing attributes where present (e.g. keep changes made while WriteFlash = false as volatile only)
  //
  Status = OcGetSip (&CsrConfig, &Attributes);

  if ((Status != EFI_NOT_FOUND) && EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCVAR: Error getting SIP status - %r\n", Status));
  } else {
    if (Status == EFI_NOT_FOUND) {
      Attributes = CSR_APPLE_SIP_NVRAM_NV_ATTR;
    } else {
      Attributes &= CSR_APPLE_SIP_NVRAM_NV_ATTR;
    }

    if (OcIsSipEnabled (Status, CsrConfig)) {
      CsrConfig = CsrActiveConfig;
      Status    = OcSetSip (&CsrConfig, Attributes);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "OCVAR: Error disabling SIP - r\n", Status));
      }
    } else {
      CsrConfig = 0;
      Status    = OcSetSip (&CsrConfig, Attributes);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "OCVAR: Error enabling SIP - r\n", Status));
      }
    }
  }

  return Status;
}
