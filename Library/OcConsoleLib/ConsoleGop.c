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
#include "ConsoleGopInternal.h"

#include <Protocol/AppleEg2Info.h>
#include <Protocol/ConsoleControl.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextOut.h>

#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcBlitLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MtrrLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcGuardLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC CONSOLE_GOP_CONTEXT mGop;

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

  Status = mGop.OriginalHandleProtocol (Handle, Protocol, Interface);

  if (Status != EFI_UNSUPPORTED) {
    return Status;
  }

  if (CompareGuid (&gEfiGraphicsOutputProtocolGuid, Protocol)) {
    if (mGop.ConsoleGop != NULL) {
      *Interface = mGop.ConsoleGop;
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

EFI_STATUS
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
    mGop.OriginalHandleProtocol  = gBS->HandleProtocol;
    gBS->HandleProtocol          = ConsoleHandleProtocol;
    gBS->Hdr.CRC32               = 0;
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
      mGop.ConsoleGop = OriginalGop;
      return EFI_ALREADY_STARTED;
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
      return EFI_UNSUPPORTED;
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

    mGop.ConsoleGop = Gop;
  } else {
    DEBUG ((DEBUG_WARN, "OCC: Missing compatible GOP - %r\n", Status));
  }
  return Status;
}

/**
  Update current GOP mode to represent either custom (rotated)
  or original mode. In general custom mode is used, but to
  call the original functions it is safer to switch to original.

  @param[in,out]  This      GOP protocol to update.
  @param[in]      Source    Source mode.
**/
STATIC
VOID
SwitchMode (
  IN OUT EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN     BOOLEAN                               UseCustom
  )
{
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Source;

  ASSERT (This != NULL);
  ASSERT (This->Mode != NULL);
  ASSERT (This->Mode->Info != NULL);

  if (UseCustom) {
    Source = &mGop.CustomModeInfo;
    This->Mode->FrameBufferBase  = 0;
    This->Mode->FrameBufferSize  = 0;
  } else {
    Source = &mGop.OriginalModeInfo;
    This->Mode->FrameBufferBase = mGop.OriginalFrameBufferBase;
    This->Mode->FrameBufferSize = mGop.OriginalFrameBufferSize;
  }

  This->Mode->Info->VerticalResolution   = Source->VerticalResolution;
  This->Mode->Info->HorizontalResolution = Source->HorizontalResolution;
  This->Mode->Info->PixelsPerScanLine    = Source->PixelsPerScanLine;
}

/**
  Translate current GOP mode to custom (rotated) mode.
  Both original and custom modes are saved.

  @param[in,out]  This      GOP protocol to update.
  @param[in]      Rotation  Rotation angle.
**/
STATIC
VOID
RotateMode (
  IN OUT EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
  IN     UINT32                        Rotation
  )
{
  ASSERT (This != NULL);
  ASSERT (This->Mode != NULL);
  ASSERT (This->Mode->Info != NULL);

  CopyMem (&mGop.OriginalModeInfo, This->Mode->Info, sizeof (mGop.OriginalModeInfo));

  if (Rotation == 90 || Rotation == 270) {
    This->Mode->Info->HorizontalResolution = mGop.OriginalModeInfo.VerticalResolution;
    This->Mode->Info->VerticalResolution   = mGop.OriginalModeInfo.HorizontalResolution;
    This->Mode->Info->PixelsPerScanLine    = This->Mode->Info->HorizontalResolution;
  }

  mGop.OriginalFrameBufferBase = This->Mode->FrameBufferBase;
  mGop.OriginalFrameBufferSize = This->Mode->FrameBufferSize;

  if (Rotation != 0) {
    //
    // macOS requires FrameBufferBase to be 0 for rotation to work, which
    // forces it inspect the AppleFramebufferInfo protocol.
    // It also requires PixelFormat to be <= PixelBitMask, otherwise Apple
    // logo will not show.
    // REF: https://github.com/acidanthera/bugtracker/issues/1498#issuecomment-782822654
    //
    // Windows and Linux bootloaders only draw directly to the framebuffer.
    //
    This->Mode->FrameBufferBase   = 0;
    This->Mode->FrameBufferSize   = 0;
  }

  CopyMem (&mGop.CustomModeInfo, This->Mode->Info, sizeof (mGop.CustomModeInfo));
}

STATIC
OC_BLIT_CONFIGURE *
EFIAPI
DirectGopFromTarget (
  IN  EFI_PHYSICAL_ADDRESS                  FramebufferBase,
  IN  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info,
  OUT UINTN                                 *PageCount
  )
{
  EFI_STATUS                    Status;
  UINTN                         ConfigureSize;
  OC_BLIT_CONFIGURE             *Context;

  ConfigureSize = 0;
  Status = OcBlitConfigure (
    (VOID *)(UINTN) FramebufferBase,
    Info,
    mGop.Rotation,
    NULL,
    &ConfigureSize
    );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return NULL;
  }

  *PageCount = EFI_SIZE_TO_PAGES (ConfigureSize);
  Context = AllocatePages (*PageCount);
  if (Context == NULL) {
    return NULL;
  }

  Status = OcBlitConfigure (
    (VOID *)(UINTN) FramebufferBase,
    Info,
    mGop.Rotation,
    Context,
    &ConfigureSize
    );
  if (EFI_ERROR (Status)) {
    FreePages (Context, *PageCount);
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
  EFI_STATUS                            Status;
  EFI_TPL                               OldTpl;
  OC_BLIT_CONFIGURE                     *Original;

  if (ModeNumber == This->Mode->Mode) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Protect from invalid Blt calls during SetMode.
  //
  Original = mGop.FramebufferContext;
  mGop.FramebufferContext = NULL;

  //
  // Protect from mishandling of rotated info.
  //
  SwitchMode (This, FALSE);

  Status = mGop.OriginalGopSetMode (This, ModeNumber);
  if (EFI_ERROR (Status)) {
    SwitchMode (This, TRUE);
    mGop.FramebufferContext = Original;
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  if (Original != NULL) {
    FreePages (Original, mGop.FramebufferContextPageCount);
  }

  RotateMode (This, mGop.Rotation);

  mGop.FramebufferContext = DirectGopFromTarget (
    mGop.OriginalFrameBufferBase,
    &mGop.OriginalModeInfo,
    &mGop.FramebufferContextPageCount
    );
  if (mGop.FramebufferContext == NULL) {
    gBS->RestoreTPL (OldTpl);
    return EFI_DEVICE_ERROR;
  }

  if (mGop.CachePolicy >= 0) {
    MtrrSetMemoryAttribute (
      mGop.OriginalFrameBufferBase,
      mGop.OriginalFrameBufferSize,
      mGop.CachePolicy
      );
  }

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
DirectQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
{
  EFI_STATUS  Status;
  UINT32      HorizontalResolution;

  SwitchMode (This, FALSE);
  Status = mGop.OriginalGopQueryMode (This, ModeNumber, SizeOfInfo, Info);
  if (EFI_ERROR (Status)) {
    SwitchMode (This, TRUE);
    return Status;
  }

  if (mGop.Rotation == 90 || mGop.Rotation == 270) {
    HorizontalResolution          = (*Info)->HorizontalResolution;
    (*Info)->HorizontalResolution = (*Info)->VerticalResolution;
    (*Info)->VerticalResolution   = HorizontalResolution;
    (*Info)->PixelsPerScanLine    = (*Info)->HorizontalResolution;
  }

  return Status;
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
  if (mGop.FramebufferContext != NULL) {
    return OcBlitRender (
      mGop.FramebufferContext,
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
  // When we change the GOP mode on some types of firmware, we need to reconnect the
  // drivers that produce simple text out as otherwise, they will not produce text
  // at the new resolution.
  //
  // Needy reports that boot.efi seems to work fine without this block of code.
  // However, I believe that UEFI specification does not provide any standard way
  // to inform TextOut protocol about resolution change, which means the firmware
  // may not be aware of the change, especially when custom GOP is used.
  // We can move this to quirks if it causes problems, but I believe the code below
  // is legit.
  //
  // Note: this block of code may result in black screens on APTIO IV boards when
  // launching OpenCore from the Shell. Hence it is optional.
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
  APPLE_EG2_INFO_PROTOCOL       *Eg2;
  UINT32                        Rotation;

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

  if (Gop->Mode->Info->PixelFormat == PixelBltOnly) {
    DEBUG ((DEBUG_INFO, "OCC: This GOP does not support direct rendering\n"));
    return EFI_UNSUPPORTED;
  }

  Status = gBS->LocateProtocol (
    &gAppleEg2InfoProtocolGuid,
    NULL,
    (VOID **) &Eg2
    );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCC: Found EG2 support %X\n", Eg2->Revision));
    if (Eg2->Revision >= APPLE_EG2_INFO_PROTOCOL_REVISION) {
      Rotation = 0;
      Status   = Eg2->GetRotation (Eg2, &Rotation);
      if (!EFI_ERROR (Status) && Rotation < AppleDisplayRotateMax)  {
        if (Rotation == AppleDisplayRotate90) {
          mGop.Rotation = 90;
        } else if (Rotation == AppleDisplayRotate180) {
          mGop.Rotation = 180;
        } else if (Rotation == AppleDisplayRotate270) {
          mGop.Rotation = 270;
        }
        DEBUG ((DEBUG_INFO, "OCC: Got rotation %u degrees from EG2\n", mGop.Rotation));
      } else {
        DEBUG ((DEBUG_INFO, "OCC: Invalid rotation %u from EG2 - %r\n", Rotation, Status));
      }
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCC: No Apple EG2 support - %r\n", Status));
  }

  RotateMode (Gop, mGop.Rotation);

  mGop.FramebufferContext = DirectGopFromTarget (
    mGop.OriginalFrameBufferBase,
    &mGop.OriginalModeInfo,
    &mGop.FramebufferContextPageCount
    );
  if (mGop.FramebufferContext == NULL) {
    DEBUG ((DEBUG_INFO, "OCC: Delaying direct GOP configuration...\n"));
    //
    // This is possible at the start.
    //
  }

  mGop.OriginalGopSetMode   = Gop->SetMode;
  mGop.OriginalGopQueryMode = Gop->QueryMode;
  Gop->SetMode   = DirectGopSetMode;
  Gop->QueryMode = DirectQueryMode;
  Gop->Blt       = DirectGopBlt;
  mGop.CachePolicy = -1;

  if (CacheType >= 0) {
    Status = MtrrSetMemoryAttribute (
      mGop.OriginalFrameBufferBase,
      mGop.OriginalFrameBufferSize,
      CacheType
      );
    DEBUG ((
      DEBUG_INFO,
      "OCC: FB (%Lx, %Lx) MTRR (%x) - %r\n",
      (UINT64) mGop.OriginalFrameBufferBase,
      (UINT64) mGop.OriginalFrameBufferSize,
      CacheType,
      Status
      ));
    if (!EFI_ERROR (Status)) {
      mGop.CachePolicy = CacheType;
    }
  }

  return EFI_SUCCESS;
}

CONST CONSOLE_GOP_CONTEXT *
InternalGetDirectGopContext (
  VOID
  )
{
  if (mGop.Rotation != 0 && mGop.OriginalGopSetMode != NULL) {
    return &mGop;
  }

  return NULL;
}
