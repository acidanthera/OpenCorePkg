/** @file
  Operations with disc drive through Apple Removable Media Protocol.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcMiscLib.h>

#include <Protocol/AppleRemovableMedia.h>

#include <Protocol/BlockIo.h>

BOOLEAN
OcEjectInjectRemMediaDrive (
  VOID
  )
{
  APPLE_REMOVABLE_MEDIA_PROTOCOL  *RemMedia;
  BOOLEAN                         TrayOpen;
  BOOLEAN                         MediaPresent;
  BOOLEAN                         Result;
  EFI_BLOCK_IO_PROTOCOL           *BlockIo;
  EFI_HANDLE                      *HandleBuffer;
  EFI_STATUS                      Status;
  EFI_TPL                         OldTpl;
  UINTN                           HandleCount;
  UINTN                           Index;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  HandleBuffer = NULL;
  Result       = FALSE;

  Status = gBS->LocateHandleBuffer (ByProtocol, &gAppleRemovableMediaProtocolGuid, NULL, &HandleCount, &HandleBuffer);

  if (!EFI_ERROR (Status)) {
    for (Index = HandleCount; Index > 0; Index--) {
      RemMedia = NULL;
      Status   = OcHandleProtocolFallback (HandleBuffer[Index - 1], &gAppleRemovableMediaProtocolGuid, (VOID **)&RemMedia);
      if (!EFI_ERROR (Status) && (RemMedia != NULL)) {
        TrayOpen = FALSE;
        Status   = RemMedia->DetectTrayState (RemMedia, &TrayOpen);
        if (!EFI_ERROR (Status) && TrayOpen) {
          Status = RemMedia->Inject (RemMedia);
          if (!EFI_ERROR (Status)) {
            Result = TRUE;
          }
        } else {
          BlockIo = NULL;
          Status  = OcHandleProtocolFallback (
                      HandleBuffer[Index - 1],
                      &gEfiBlockIoProtocolGuid,
                      (VOID **)&BlockIo
                      );
          if (EFI_ERROR (Status) || (BlockIo == NULL)) {
            BlockIo      = NULL;
            MediaPresent = FALSE;
          } else {
            MediaPresent = BlockIo->Media->MediaPresent;
          }

          Status = RemMedia->Eject (RemMedia);
          if (!EFI_ERROR (Status) && MediaPresent && (BlockIo != NULL)) {
            Status = gBS->ReinstallProtocolInterface (
                            HandleBuffer[Index - 1],
                            &gEfiBlockIoProtocolGuid,
                            BlockIo,
                            BlockIo
                            );
            if (!EFI_ERROR (Status)) {
              Result = TRUE;
            }
          }
        }
      }
    }
  }

  gBS->RestoreTPL (OldTpl);

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  return Result;
}
