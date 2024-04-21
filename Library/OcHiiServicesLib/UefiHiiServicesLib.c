/** @file
  This library retrieves pointers to the UEFI HII Protocol instances in the
  library's constructor.  All of the UEFI HII related protocols are optional,
  so the consumers of this library class must verify that the global variable
  pointers are not NULL before use.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

#include <Protocol/HiiFont.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiImage.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiConfigRouting.h>

///
/// Pointer to the UEFI HII Font Protocol
///
EFI_HII_FONT_PROTOCOL  *gHiiFont = NULL;

///
/// Pointer to the UEFI HII String Protocol
///
EFI_HII_STRING_PROTOCOL  *gHiiString = NULL;

///
/// Pointer to the UEFI HII Image Protocol
///
EFI_HII_IMAGE_PROTOCOL  *gHiiImage = NULL;

///
/// Pointer to the UEFI HII Database Protocol
///
EFI_HII_DATABASE_PROTOCOL  *gHiiDatabase = NULL;

///
/// Pointer to the UEFI HII Config Rounting Protocol
///
EFI_HII_CONFIG_ROUTING_PROTOCOL  *gHiiConfigRouting = NULL;

/**
  The constructor function retrieves pointers to the UEFI HII protocol instances

  The constructor function retrieves pointers to the four UEFI HII protocols from the
  handle database.  These include the UEFI HII Font Protocol, the UEFI HII String
  Protocol, the UEFI HII Image Protocol, the UEFI HII Database Protocol, and the
  UEFI HII Config Routing Protocol.  This function always return EFI_SUCCESS.
  All of these protocols are optional if the platform does not support configuration
  and the UEFI HII Image Protocol and the UEFI HII Font Protocol are optional if
  the platform does not support a graphical console.  As a result, the consumers
  of this library much check the protocol pointers againt NULL before using them,
  or use dependency expressions to guarantee that some of them are present before
  assuming they are not NULL.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
UefiHiiServicesLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Retrieve the pointer to the UEFI HII String Protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiStringProtocolGuid, NULL, (VOID **)&gHiiString);
  DEBUG ((DEBUG_INFO, "Locate UEFI HII String Protocol - %r\n", Status));

  //
  // Retrieve the pointer to the UEFI HII Database Protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **)&gHiiDatabase);
  DEBUG ((DEBUG_INFO, "Locate UEFI HII Database Protocol - %r\n", Status));

  //
  // Retrieve the pointer to the UEFI HII Config Routing Protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **)&gHiiConfigRouting);
  DEBUG ((DEBUG_INFO, "Locate UEFI HII Config Routing Protocol - %r\n", Status));

  //
  // Retrieve the pointer to the optional UEFI HII Font Protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiFontProtocolGuid, NULL, (VOID **)&gHiiFont);
  DEBUG ((DEBUG_INFO, "Locate UEFI HII Font Protocol - %r\n", Status));

  //
  // Retrieve the pointer to the optional UEFI HII Image Protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiImageProtocolGuid, NULL, (VOID **)&gHiiImage);
  DEBUG ((DEBUG_INFO, "Locate UEFI HII Image Protocol - %r\n", Status));

  return EFI_SUCCESS;
}
