/** @file
  OpenCore driver.

Copyright (c) 2019, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OpenCore.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcVirtualFsLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/DevicePath.h>

STATIC OC_STORAGE_CONTEXT  *mOcStorage;
STATIC OC_GLOBAL_CONFIG    *mOcConfiguration;

STATIC
UINT32
OcKernelLoadKextsAndReserve (
  IN OC_STORAGE_CONTEXT  *Storage,
  IN OC_GLOBAL_CONFIG    *Config
  )
{
  UINT32  Index;
  UINT32  ReserveSize;
  CHAR8   *BundleName;
  CHAR8   *PlistPath;
  CHAR8   *ExecutablePath;
  CHAR16  FullPath[128];

  ReserveSize = PRELINK_INFO_RESERVE_SIZE;

  for (Index = 0; Index < Config->Kernel.Add.Count; ++Index) {
    if (Config->Kernel.Add.Values[Index]->Disabled) {
      continue;
    }

    if (Config->Kernel.Add.Values[Index]->PlistDataSize == 0) {
      BundleName     = OC_BLOB_GET (&Config->Kernel.Add.Values[Index]->BundleName);
      PlistPath      = OC_BLOB_GET (&Config->Kernel.Add.Values[Index]->PlistPath);
      if (BundleName[0] == '\0' || PlistPath[0] == '\0') {
        DEBUG ((DEBUG_ERROR, "OC: Your config has improper for kext info\n"));
        continue;
      }

      UnicodeSPrint (
        FullPath,
        sizeof (FullPath),
        OPEN_CORE_KEXT_PATH "%a\\%a",
        BundleName,
        PlistPath
        );

      Config->Kernel.Add.Values[Index]->PlistData = OcStorageReadFileUnicode (
        Storage,
        FullPath,
        &Config->Kernel.Add.Values[Index]->PlistDataSize
        );

      if (Config->Kernel.Add.Values[Index]->PlistData == NULL) {
        DEBUG ((DEBUG_ERROR, "OC: Plist %s is missing for kext %s\n", FullPath, BundleName));
        continue;
      }

      ExecutablePath = OC_BLOB_GET (&Config->Kernel.Add.Values[Index]->ExecutablePath);
      if (ExecutablePath[0] != '\0') {
        UnicodeSPrint (
          FullPath,
          sizeof (FullPath),
          OPEN_CORE_KEXT_PATH "%a\\%a",
          BundleName,
          ExecutablePath
          );

        Config->Kernel.Add.Values[Index]->ImageData = OcStorageReadFileUnicode (
          Storage,
          FullPath,
          &Config->Kernel.Add.Values[Index]->ImageDataSize
          );

        if (Config->Kernel.Add.Values[Index]->ImageData == NULL) {
          DEBUG ((DEBUG_ERROR, "OC: Image %s is missing for kext %s\n", FullPath, BundleName));
          //
          // Still continue loading?
          //
        }
      }
    }

    PrelinkedReserveKextSize (
      &ReserveSize,
      Config->Kernel.Add.Values[Index]->PlistDataSize,
      Config->Kernel.Add.Values[Index]->ImageData,
      Config->Kernel.Add.Values[Index]->ImageDataSize
      );
  }

  DEBUG ((DEBUG_INFO, "Kext reservation size %u\n", ReserveSize));

  return ReserveSize;
}

STATIC
EFI_STATUS
OcKernelProcessPrelinked (
  IN     OC_GLOBAL_CONFIG  *Config,
  IN OUT UINT8             *Kernel,
  IN     UINT32            *KernelSize,
  IN     UINT32            AllocatedSize
  )
{
  EFI_STATUS         Status;
  PRELINKED_CONTEXT  Context;
  CHAR8              *BundleName;
  CHAR8              *ExecutablePath;
  UINT32             Index;
  CHAR8              FullPath[128];

  Status = PrelinkedContextInit (&Context, Kernel, *KernelSize, AllocatedSize);

  if (!EFI_ERROR (Status)) {

    /* TODO: ApplyKextPatches (&Context); */

    Status = PrelinkedInjectPrepare (&Context);
    if (!EFI_ERROR (Status)) {

      for (Index = 0; Index < Config->Kernel.Add.Count; ++Index) {
        if (Config->Kernel.Add.Values[Index]->Disabled
          || Config->Kernel.Add.Values[Index]->PlistDataSize == 0) {
          continue;
        }

        BundleName     = OC_BLOB_GET (&Config->Kernel.Add.Values[Index]->BundleName);
        AsciiSPrint (FullPath, sizeof (FullPath), "/Library/Extensions/%a", BundleName);
        if (Config->Kernel.Add.Values[Index]->ImageData != NULL) {
          ExecutablePath = OC_BLOB_GET (&Config->Kernel.Add.Values[Index]->ExecutablePath);
        } else {
          ExecutablePath = NULL;
        }

        Status = PrelinkedInjectKext (
          &Context,
          FullPath,
          Config->Kernel.Add.Values[Index]->PlistData,
          Config->Kernel.Add.Values[Index]->PlistDataSize,
          ExecutablePath,
          Config->Kernel.Add.Values[Index]->ImageData,
          Config->Kernel.Add.Values[Index]->ImageDataSize
          );

        DEBUG ((DEBUG_INFO, "OC: Prelink injection %a - %r\n", BundleName, Status));
      }

      Status = PrelinkedInjectComplete (&Context);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "OC: Prelink insertion error - %r\n", Status));
      }
    } else {
      DEBUG ((DEBUG_WARN, "OC: Prelink inject prepare error - %r\n", Status));
    }

    *KernelSize = Context.PrelinkedSize;

    PrelinkedContextFree (&Context);
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
OcKernelFileOpen (
  IN  EFI_FILE_PROTOCOL       *This,
  OUT EFI_FILE_PROTOCOL       **NewHandle,
  IN  CHAR16                  *FileName,
  IN  UINT64                  OpenMode,
  IN  UINT64                  Attributes
  )
{
  EFI_STATUS         Status;
  UINT8              *Kernel;
  UINT32             KernelSize;
  UINT32             AllocatedSize;
  CHAR16             *FileNameCopy;
  EFI_FILE_PROTOCOL  *VirtualFileHandle;
  EFI_STATUS         PrelinkedStatus;
  EFI_TIME           ModificationTime;

  Status = This->Open (This, NewHandle, FileName, OpenMode, Attributes);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // boot.efi uses /S/L/K/kernel as is to determine valid filesystem.
  // Just skip it to speedup the boot process.
  // On 10.9 mach_kernel is loaded for manual linking aferwards, so we cannot skip it.
  //
  if (OpenMode == EFI_FILE_MODE_READ
    && StrStr (FileName, L"kernel") != NULL
    && StrCmp (FileName, L"System\\Library\\Kernels\\kernel") != 0) {

    DEBUG ((DEBUG_INFO, "Trying XNU hook on %s\n", FileName));
    Status = ReadAppleKernel (
      *NewHandle,
      &Kernel,
      &KernelSize,
      &AllocatedSize,
      OcKernelLoadKextsAndReserve (mOcStorage, mOcConfiguration)
      );
    DEBUG ((DEBUG_INFO, "Result of XNU hook on %s is %r\n", FileName, Status));

    //
    // This is not Apple kernel, just return the original file.
    //
    if (!EFI_ERROR (Status)) {
      /* TODO: get Darwin Kernel Version  */
      /* ApplyKernelPatches (Kernel, KernelSize); */

      PrelinkedStatus = OcKernelProcessPrelinked (
        mOcConfiguration,
        Kernel,
        &KernelSize,
        AllocatedSize
        );

      DEBUG ((DEBUG_INFO, "Prelinked status - %r\n", PrelinkedStatus));

      Status = GetFileModifcationTime (*NewHandle, &ModificationTime);
      if (EFI_ERROR (Status)) {
        ZeroMem (&ModificationTime, sizeof (ModificationTime));
      }

      (*NewHandle)->Close(*NewHandle);

      //
      // This was our file, yet firmware is dying.
      //
      FileNameCopy = AllocateCopyPool (StrSize (FileName), FileName);
      if (FileNameCopy == NULL) {
        DEBUG ((DEBUG_WARN, "Failed to allocate kernel name (%a) copy\n", FileName));
        FreePool (Kernel);
        return EFI_OUT_OF_RESOURCES;
      }

      Status = CreateVirtualFile (FileNameCopy, Kernel, KernelSize, &ModificationTime, &VirtualFileHandle);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "Failed to virtualise kernel file (%a)\n", FileName));
        FreePool (Kernel);
        FreePool (FileNameCopy);
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Return our handle.
      //
      *NewHandle = VirtualFileHandle;
      return EFI_SUCCESS;
    }
  }

  return CreateRealFile (*NewHandle, NULL, TRUE, NewHandle);
}

VOID
OcLoadKernelSupport (
  IN OC_STORAGE_CONTEXT  *Storage,
  IN OC_GLOBAL_CONFIG    *Config
  )
{
  EFI_STATUS  Status;

  Status = EnableVirtualFs (gBS, OcKernelFileOpen);

  if (!EFI_ERROR (Status)) {
    mOcStorage       = Storage;
    mOcConfiguration = Config;
  } else {
    DEBUG ((DEBUG_ERROR, "OC: Failed to enable vfs - %r\n", Status));
  }
}

VOID
OcUnloadKernelSupport (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mOcStorage != NULL) {
    Status = DisableVirtualFs (gBS);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OC: Failed to disable vfs - %r\n", Status));
    }
    mOcStorage       = NULL;
    mOcConfiguration = NULL;
  }
}
