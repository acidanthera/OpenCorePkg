/** @file
  Test data hub support.

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/OcDevicePropertyLib.h>
#include <Library/DevicePathLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcProtocolLib.h>
#include <Library/OcAppleBootPolicyLib.h>
#include <Library/OcSmbiosLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcVirtualFsLib.h>
#include <Library/OcAppleKernelLib.h>

#include <Protocol/AppleBootPolicy.h>
#include <Protocol/DevicePathPropertyDatabase.h>


#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/SimpleFileSystem.h>

STATIC
EFI_STATUS
EFIAPI
TestFileOpen (
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

  Status = This->Open (This, NewHandle, FileName, OpenMode, Attributes);

  if (!EFI_ERROR (Status)
    && OpenMode == EFI_FILE_MODE_READ
    && StrStr (FileName, L"kernel") != NULL) {
    Print (L"Trying XNU hook on %s\n", FileName);
    Status = ReadAppleKernel (*NewHandle, &Kernel, &KernelSize, &AllocatedSize);
    Print (L"Result of XNU hook on %s is %r\n", FileName, Status);

    //
    // This is not Apple kernel, just return the original file.
    //
    if (EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }

    //
    // TODO: patches, dropping, and injection here.
    //

    ApplyPatch (
      (UINT8 *) "Darwin Kernel Version",
      NULL,
      L_STR_LEN ("Darwin Kernel Version"),
      (UINT8 *) "OpenCore Boot Version",
      Kernel,
      KernelSize,
      1,
      0
      );

    //
    // This was our file, yet firmware is dying.
    //
    FileNameCopy = AllocateCopyPool (StrSize (FileName), FileName);
    if (FileNameCopy == NULL) {
      DEBUG ((DEBUG_WARN, "Failed to allocate kernel name (%a) copy\n", FileName));
      FreePool (Kernel);
      return EFI_SUCCESS;
    }

    Status = CreateVirtualFile (FileNameCopy, Kernel, KernelSize, &VirtualFileHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Failed to virtualise kernel file (%a)\n", FileName));
      FreePool (Kernel);
      FreePool (FileNameCopy);
      return EFI_SUCCESS;
    }

    //
    // Return our handle.
    //
    (*NewHandle)->Close(*NewHandle);
    *NewHandle = VirtualFileHandle;
  }

  return Status;
}

EFI_STATUS
EFIAPI
TestKernel (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EnableVirtualFs (gBS, TestFileOpen);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Failed to enable vfs - %r\n", Status));
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  WaitForKeyPress (L"Press any key...");

  for (Index = 0; Index < SystemTable->NumberOfTableEntries; ++Index) {
    Print (L"Table %u is %g\n", (UINT32) Index, &SystemTable->ConfigurationTable[Index].VendorGuid);
  }

  Print (L"This is test app...\n");

  TestKernel (ImageHandle, SystemTable);

  Status = DisableVirtualFs (gBS);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Failed to disable vfs - %r\n", Status));
  }

  return EFI_SUCCESS;
}
