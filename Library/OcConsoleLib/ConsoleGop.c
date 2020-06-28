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

#include "OcConsoleLibInternal.h"

#include <Protocol/ConsoleControl.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextOut.h>

#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/FrameBufferBltLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MtrrLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcGuardLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC
EFI_HANDLE_PROTOCOL
mOriginalHandleProtocol;

STATIC
EFI_GRAPHICS_OUTPUT_PROTOCOL *
mConsoleGraphicsOutput;

STATIC
FRAME_BUFFER_CONFIGURE *
mFramebufferContext;

STATIC
EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE
mOriginalGopSetMode;

STATIC
INT32
mCachePolicy;

STATIC
EFI_STATUS
EFIAPI
ConsoleHandleProtocol (
  IN  EFI_HANDLE        Handle,
  IN  EFI_GUID          *Protocol,
  OUT VOID              **Interface
  )
{
  EFI_STATUS  Status;

  Status = mOriginalHandleProtocol (Handle, Protocol, Interface);

  if (Status != EFI_UNSUPPORTED) {
    return Status;
  }

  if (CompareGuid (&gEfiGraphicsOutputProtocolGuid, Protocol)) {
    if (mConsoleGraphicsOutput != NULL) {
      *Interface = mConsoleGraphicsOutput;
      return EFI_SUCCESS;
    }
  } else if (CompareGuid (&gEfiUgaDrawProtocolGuid, Protocol)) {
    //
    // EfiBoot from 10.4 can only use UgaDraw protocol.
    //
    Status = gBS->LocateProtocol (
      &gEfiUgaDrawProtocolGuid,
      NULL,
      Interface
      );
    if (!EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }
  }

  return EFI_UNSUPPORTED;
}

VOID
OcProvideConsoleGop (
  IN BOOLEAN  Route
  )
{
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *OriginalGop;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop;
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         Index;

  //
  // Shell may replace gST->ConsoleOutHandle, so we have to ensure
  // that HandleProtocol always reports valid chosen GOP.
  //
  if (Route) {
    mOriginalHandleProtocol  = gBS->HandleProtocol;
    gBS->HandleProtocol      = ConsoleHandleProtocol;
    gBS->Hdr.CRC32           = 0;
    gBS->CalculateCrc32 (gBS, gBS->Hdr.HeaderSize, &gBS->Hdr.CRC32);
  }

  OriginalGop = NULL;
  Status = gBS->HandleProtocol (
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &OriginalGop
    );

  if (!EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCC: GOP exists on ConsoleOutHandle and has %u modes\n",
      (UINT32) OriginalGop->Mode->MaxMode
      ));

    //
    // This is not the case on MacPro5,1 with Mac EFI incompatible GPU.
    // Here we need to uninstall ConOut GOP in favour of GPU GOP.
    //
    if (OriginalGop->Mode->MaxMode > 0) {
      mConsoleGraphicsOutput = OriginalGop;
      return;
    }

    DEBUG ((
      DEBUG_INFO,
      "OCC: Looking for GOP replacement due to invalid mode count\n"
      ));

    Status = gBS->LocateHandleBuffer (
      ByProtocol,
      &gEfiGraphicsOutputProtocolGuid,
      NULL,
      &HandleCount,
      &HandleBuffer
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCC: No handles with GOP protocol - %r\n", Status));
      return;
    }

    Status = EFI_NOT_FOUND;
    for (Index = 0; Index < HandleCount; ++Index) {
      if (HandleBuffer[Index] != gST->ConsoleOutHandle) {
        Status = gBS->HandleProtocol (
          HandleBuffer[Index],
          &gEfiGraphicsOutputProtocolGuid,
          (VOID **) &Gop
          );
        break;
      }
    }

    DEBUG ((DEBUG_INFO, "OCC: Alternative GOP status is - %r\n", Status));
    FreePool (HandleBuffer);

    if (!EFI_ERROR (Status)) {
      gBS->UninstallProtocolInterface (
        gST->ConsoleOutHandle,
        &gEfiGraphicsOutputProtocolGuid,
        OriginalGop
        );
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCC: Installing GOP (%r) on ConsoleOutHandle...\n", Status));
    Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **) &Gop);
  }

  if (!EFI_ERROR (Status)) {
    Status = gBS->InstallMultipleProtocolInterfaces (
      &gST->ConsoleOutHandle,
      &gEfiGraphicsOutputProtocolGuid,
      Gop,
      NULL
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCC: Failed to install GOP on ConsoleOutHandle - %r\n", Status));
    }

    mConsoleGraphicsOutput = Gop;
  } else {
    DEBUG ((DEBUG_WARN, "OCC: Missing compatible GOP - %r\n", Status));
  }
}

STATIC
FRAME_BUFFER_CONFIGURE *
EFIAPI
DirectGopFromTarget (
  IN EFI_PHYSICAL_ADDRESS                  FramebufferBase,
  IN EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info
  )
{
  EFI_STATUS                    Status;
  UINTN                         ConfigureSize;
  FRAME_BUFFER_CONFIGURE        *Context;

  ConfigureSize = 0;
  Status = FrameBufferBltConfigure (
    (VOID *)(UINTN) FramebufferBase,
    Info,
    NULL,
    &ConfigureSize
    );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return NULL;
  }

  Context = AllocatePool (ConfigureSize);
  if (Context == NULL) {
    return NULL;
  }

  Status = FrameBufferBltConfigure (
    (VOID *)(UINTN) FramebufferBase,
    Info,
    Context,
    &ConfigureSize
    );
  if (EFI_ERROR (Status)) {
    FreePool (Context);
    return NULL;
  }

  return Context;
}

STATIC
EFI_STATUS
EFIAPI
DirectGopSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
  IN  UINT32                       ModeNumber
  )
{
  EFI_STATUS              Status;
  EFI_TPL                 OldTpl;
  FRAME_BUFFER_CONFIGURE  *Original;

  if (ModeNumber == This->Mode->Mode) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Protect from invalid Blt calls during SetMode
  //
  Original = mFramebufferContext;
  mFramebufferContext = NULL;

  Status = mOriginalGopSetMode (This, ModeNumber);
  if (EFI_ERROR (Status)) {
    mFramebufferContext = Original;
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  if (Original != NULL) {
    FreePool (Original);
  }

  mFramebufferContext = DirectGopFromTarget (This->Mode->FrameBufferBase, This->Mode->Info);
  if (mFramebufferContext == NULL) {
    gBS->RestoreTPL (OldTpl);
    return EFI_DEVICE_ERROR;
  }

  if (mCachePolicy >= 0) {
    MtrrSetMemoryAttribute (
      This->Mode->FrameBufferBase,
      This->Mode->FrameBufferSize,
      mCachePolicy
      );
  }

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
DirectGopBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL            *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL           *BltBuffer   OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION       BltOperation,
  IN  UINTN                                   SourceX,
  IN  UINTN                                   SourceY,
  IN  UINTN                                   DestinationX,
  IN  UINTN                                   DestinationY,
  IN  UINTN                                   Width,
  IN  UINTN                                   Height,
  IN  UINTN                                   Delta         OPTIONAL
  )
{
  if (mFramebufferContext != NULL) {
    return FrameBufferBlt (
      mFramebufferContext,
      BltBuffer,
      BltOperation,
      SourceX,
      SourceY,
      DestinationX,
      DestinationY,
      Width,
      Height,
      Delta
      );
  }

  return EFI_DEVICE_ERROR;
}

VOID
OcReconnectConsole (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Index;

  //
  // On some firmwares When we change mode on GOP, we need to reconnect the drivers
  // which produce simple text out. Otherwise, they won't produce text based on the
  // new resolution.
  //
  // Needy reports that boot.efi seems to work fine without this block of code.
  // However, I believe that UEFI specification does not provide any standard way
  // to inform TextOut protocol about resolution change, which means the firmware
  // may not be aware of the change, especially when custom GOP is used.
  // We can move this to quirks if it causes problems, but I believe the code below
  // is legit.
  //
  // Note: on APTIO IV boards this block of code may result in black screen when launching
  // OpenCore from Shell, thus it is optional.
  //

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiSimpleTextOutProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; ++Index) {
      gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);
    }

    for (Index = 0; Index < HandleCount; ++Index) {
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }

    FreePool (HandleBuffer);

    //
    // It is implementation defined, which console mode is used by ConOut.
    // Assume the implementation chooses most sensible value based on GOP resolution.
    // If it does not, there is a separate ConsoleMode param, which expands to SetConsoleMode.
    //
  } else {
    DEBUG ((DEBUG_WARN, "OCC: Failed to find any text output handles\n"));
  }
}

EFI_STATUS
OcUseDirectGop (
  IN INT32  CacheType
  )
{
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop;

  DEBUG ((DEBUG_INFO, "OCC: Switching to direct GOP renderer...\n"));

  Status = gBS->HandleProtocol (
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &Gop
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCC: Cannot find console GOP for direct GOP - %r\n", Status));
    return Status;
  }

  mFramebufferContext = DirectGopFromTarget (Gop->Mode->FrameBufferBase, Gop->Mode->Info);
  if (mFramebufferContext == NULL) {
    DEBUG ((DEBUG_INFO, "OCC: Delaying direct GOP configuration...\n"));
    //
    // This is possible at the start.
    //
  }

  mOriginalGopSetMode = Gop->SetMode;
  Gop->SetMode = DirectGopSetMode;
  Gop->Blt = DirectGopBlt;
  mCachePolicy = -1;

  if (CacheType >= 0) {
    Status = MtrrSetMemoryAttribute (
      Gop->Mode->FrameBufferBase,
      Gop->Mode->FrameBufferSize,
      CacheType
      );
    DEBUG ((
      DEBUG_INFO,
      "OCC: FB (%Lx, %Lx) MTRR (%x) - %r\n",
      (UINT64) Gop->Mode->FrameBufferBase,
      (UINT64) Gop->Mode->FrameBufferSize,
      CacheType,
      Status
      ));
    if (!EFI_ERROR (Status)) {
      mCachePolicy = CacheType;
    }
  }

  return EFI_SUCCESS;
}
