/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>
#include <FrameworkDxe.h>

#include <Guid/FileInfo.h>

#include <Protocol/Decompress.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "OcFileLibInternal.h"

// ReadFvFile
/** Search for and read a file from a firmware volume

  @param[in] FvFileGuid   The guid of the firmware file to read.
  @param[out] FileBuffer  The address of a buffer with the contents of the file read or NULL
  @param[out] FileSize    The size of the file read or 0

  @retval EFI_SUCCESS            The file was loaded successfully.
  @retval EFI_INVALID_PARAMETER  The parameters passed were invalid.
  @retval EFI_OUT_OF_RESOURCES   The system ran out of memory.
**/
EFI_STATUS
ReadFvFile (
  IN  GUID   *FvFileGuid,
  OUT VOID   **FileBuffer,
  OUT UINTN  *FileSize
  )
{
  EFI_STATUS                    Status;

  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv = NULL;
  EFI_COMPRESSION_SECTION       *CompressionHeader;
  EFI_DECOMPRESS_PROTOCOL       *Decompress = NULL;
  EFI_FV_FILE_ATTRIBUTES        Attributes = 0;
  EFI_FV_FILETYPE               Type = 0;
  EFI_HANDLE                    *FvHandleBuffer;
  UINT32                        AuthenticationStatus = 0;
  UINTN                         Index;
  UINTN                         FvHandleCount;
  UINT8                         *FvFileBuffer = NULL;
  UINTN                         FvFileSize = 0;
  UINT8                         *DestinationBuffer;
  VOID                          *ScratchBuffer;
  UINT32                        DestinationSize;
  UINT32                        ScratchSize;

  Status = EFI_INVALID_PARAMETER;

  if ((FvFileGuid != NULL) && (FileBuffer != NULL) & (FileSize != NULL)) {
    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    NULL,
                    &FvHandleCount,
                    &FvHandleBuffer
                    );

    if (!EFI_ERROR (Status)) {
      for (Index = 0; Index < FvHandleCount; ++Index) {
        Status = gBS->HandleProtocol (
                        FvHandleBuffer[Index],
                        &gEfiFirmwareVolume2ProtocolGuid,
                        (VOID **)&Fv
                        );

        if (!EFI_ERROR (Status)) {
          Status = Fv->ReadFile (
                         Fv,
                         FvFileGuid,
                         (VOID **)&FvFileBuffer,
                         &FvFileSize,
                         &Type,
                         &Attributes,
                         &AuthenticationStatus
                         );

          if (!EFI_ERROR (Status)) {
            CompressionHeader = (EFI_COMPRESSION_SECTION *)FvFileBuffer;

            if (CompressionHeader->CommonHeader.Type == EFI_SECTION_COMPRESSION) {
              if (CompressionHeader->CompressionType == EFI_NOT_COMPRESSED) {
                // The stream is not actually compressed, just encapsulated.  So just copy it.

                *FileBuffer = AllocatePool (FvFileSize - sizeof (*CompressionHeader));
                Status      = EFI_OUT_OF_RESOURCES;

                if (*FileBuffer != NULL) {
                  *FileSize = (FvFileSize - sizeof (*CompressionHeader));

                  CopyMem (*FileBuffer, FvFileBuffer + sizeof (*CompressionHeader), *FileSize);

                  Status = EFI_SUCCESS;
                }
              } else {
                Status = gBS->LocateProtocol (
                                &gEfiDecompressProtocolGuid,
                                NULL,
                                (VOID **)&Decompress
                                );

                if (EFI_ERROR (Status)) {
                  Status = EFI_NOT_FOUND;
                } else {
                  // Retrieve buffer size requirements for decompression

                  Status = Decompress->GetInfo (
                                         Decompress,
                                         (CompressionHeader + 1),
                                         ((UINT32)FvFileSize - sizeof (*CompressionHeader)),
                                         &DestinationSize,
                                         &ScratchSize
                                         );

                  if (EFI_ERROR (Status)) {
                    continue;
                  }

                  DestinationBuffer = AllocatePool (DestinationSize);
                  ScratchBuffer     = AllocatePool (ScratchSize);
                  Status            = EFI_OUT_OF_RESOURCES;

                  if ((DestinationBuffer != NULL) && (ScratchBuffer != NULL)) {
                    Status = Decompress->Decompress (
                                           Decompress,
                                           (CompressionHeader + 1),
                                           ((UINT32)FvFileSize - sizeof (*CompressionHeader)),
                                           DestinationBuffer,
                                           DestinationSize,
                                           ScratchBuffer,
                                           ScratchSize
                                           );

                    if (!EFI_ERROR (Status)) {
                      *FileBuffer = (DestinationBuffer + sizeof (*CompressionHeader));
                      *FileSize   = (DestinationSize - sizeof (*CompressionHeader));
                    }

                    FreePool (ScratchBuffer);
                  }
                }
              }
            } else {
              *FileBuffer = AllocatePool (FvFileSize - sizeof (EFI_COMMON_SECTION_HEADER));
              Status      = EFI_OUT_OF_RESOURCES;

              if (*FileBuffer != NULL) {
                *FileSize = (FvFileSize - sizeof (EFI_COMMON_SECTION_HEADER));

                CopyMem (*FileBuffer, (FvFileBuffer + sizeof (EFI_COMMON_SECTION_HEADER)), *FileSize);

                Status = EFI_SUCCESS;
              }
            }
            break;
          }
        }
      }
    }
  }

  return Status;
}
