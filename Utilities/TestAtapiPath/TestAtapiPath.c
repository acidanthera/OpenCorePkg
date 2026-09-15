/** @file
  Regression tests for the generic device path resolver with mocked disks.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Uefi.h>
#include <UserMemory.h>
#include <Protocol/BlockIo.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcDebugLogLib.h>

//
// Diagnostic output does not affect device-path resolution.
//
VOID
DebugPrintDevicePath (
  UINTN                     ErrorLevel,
  CONST CHAR8               *Message,
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
}

STATIC CONST UINT8            mImage[] = {
  2,    1,    12, 0, 0xd0, 0x41, 3,   0x0a, 0,   0, 0,   0,
  1,    1,    6,  0, 0,    0x1c, 1,   1,    6,   0, 0,   0,
  3,    1,    8,  0, 0,    1,    0,   0,
  4,    4,    16, 0, '1',  0,    '0', 0,    '0', 0, ':', 0,'A',0, 0, 0,
  0x7f, 0xff, 4,  0
};
STATIC UINT8                  mPaths[3][86];
STATIC EFI_BLOCK_IO_MEDIA     mMedia[3];
STATIC EFI_BLOCK_IO_PROTOCOL  mBlockIo[3];
STATIC UINTN                  mHandleCount;
STATIC BOOLEAN                mAlreadyValid;
STATIC BOOLEAN                mRejectFixed;
STATIC BOOLEAN                mNoHandles;
STATIC BOOLEAN                mRequireRootFix;
STATIC UINT32                 mTests;

STATIC
VOID
Check (
  BOOLEAN      Condition,
  CONST CHAR8  *Description
  )
{
  if (!Condition) {
    fprintf (stderr, "FAIL: %s\n", Description);
    exit (1);
  }
}

STATIC
EFI_STATUS
EFIAPI
MockHandleProtocol (
  EFI_HANDLE  Handle,
  EFI_GUID    *Protocol,
  VOID        **Interface
  )
{
  UINTN  Index;

  Index = (UINTN)Handle - 1;

  if (Index >= mHandleCount) {
    return EFI_NOT_FOUND;
  }

  if (CompareGuid (Protocol, &gEfiBlockIoProtocolGuid)) {
    *Interface = &mBlockIo[Index];
  } else if (CompareGuid (Protocol, &gEfiDevicePathProtocolGuid)) {
    *Interface = mPaths[Index];
  } else {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MockLocateHandleBuffer (
  EFI_LOCATE_SEARCH_TYPE  SearchType,
  EFI_GUID                *Protocol,
  VOID                    *SearchKey,
  UINTN                   *Count,
  EFI_HANDLE              **HandleBuffer
  )
{
  UINTN  Index;

  if (mNoHandles || !mHandleCount) {
    return EFI_NOT_FOUND;
  }

  *Count        = mHandleCount;
  *HandleBuffer = AllocatePool (mHandleCount * sizeof (**HandleBuffer));
  for (Index = 0; Index < mHandleCount; ++Index) {
    (*HandleBuffer)[Index] = (EFI_HANDLE)(UINTN)(Index + 1);
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MockLocateDevicePath (
  EFI_GUID                  *Protocol,
  EFI_DEVICE_PATH_PROTOCOL  **DevicePath,
  EFI_HANDLE                *Handle
  )
{
  UINTN  Index;
  UINTN  PrefixSize;
  UINT8  *Bytes;

  Bytes = (UINT8 *)*DevicePath;

  if (mRequireRootFix && (memcmp (Bytes, mPaths[0], 12) != 0)) {
    *Handle = NULL;
    return EFI_NOT_FOUND;
  }

  if (mAlreadyValid) {
    *DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)(Bytes + 24 + Bytes[26]);
    *Handle     = (EFI_HANDLE)1;
    return EFI_SUCCESS;
  }

  for (Index = 0; Index < mHandleCount; ++Index) {
    PrefixSize = GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *)mPaths[Index]) - END_DEVICE_PATH_LENGTH;
    if ((GetDevicePathSize (*DevicePath) >= PrefixSize + END_DEVICE_PATH_LENGTH) && (memcmp (Bytes, mPaths[Index], PrefixSize) == 0)) {
      if (mRejectFixed) {
        return EFI_NOT_FOUND;
      }

      *DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)(Bytes + PrefixSize);
      // Partition Block I/O may consume HD too; DevicePath can resolve APFS.
      if ((DevicePathType (*DevicePath) == MEDIA_DEVICE_PATH) && (DevicePathSubType (*DevicePath) == MEDIA_HARDDRIVE_DP)) {
        *DevicePath = NextDevicePathNode (*DevicePath);
      }

      if (CompareGuid (Protocol, &gEfiDevicePathProtocolGuid)) {
        while (!IsDevicePathEnd (*DevicePath) && DevicePathSubType (*DevicePath) != MEDIA_FILEPATH_DP) {
          *DevicePath = NextDevicePathNode (*DevicePath);
        }
      }

      *Handle = (EFI_HANDLE)(UINTN)(Index + 1);
      return EFI_SUCCESS;
    }
  }

  if (CompareGuid (Protocol, &gEfiDevicePathProtocolGuid)) {
    // Firmware can locate the controller, but not its malformed storage node.
    *DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)(Bytes + 24);
    *Handle     = (EFI_HANDLE)1;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

STATIC
VOID
Reset (
  VOID
  )
{
  UINTN  Index;

  mHandleCount  = 1;
  mAlreadyValid = mRejectFixed = mNoHandles = mRequireRootFix = 0;
  memset (mPaths, 0, sizeof (mPaths));
  memset (mMedia, 0, sizeof (mMedia));
  memset (mBlockIo, 0, sizeof (mBlockIo));
  for (Index = 0; Index < 3; ++Index) {
    memcpy (mPaths[Index], mImage, 24);
    mPaths[Index][24] = 3;
    mPaths[Index][25] = MSG_SASEX_DP;
    mPaths[Index][26] = 16;
    mPaths[Index][28] = (UINT8)(Index + 1);
    memcpy (mPaths[Index] + 40, mImage + sizeof (mImage) - 4, 4);
    mMedia[Index].MediaPresent = TRUE;
    mMedia[Index].BlockSize    = 4096;
    mMedia[Index].LastBlock    = 1000000;
    mBlockIo[Index].Media      = &mMedia[Index];
  }

  gBS->HandleProtocol     = MockHandleProtocol;
  gBS->LocateHandleBuffer = MockLocateHandleBuffer;
  gBS->LocateDevicePath   = MockLocateDevicePath;
}

STATIC
VOID
Run (
  CONST CHAR8  *Name,
  CONST UINT8  *Input,
  UINTN        Size,
  BOOLEAN      Expected
  )
{
  UINTN                     Allocations;
  UINTN                     SuffixOffset;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *OriginalPath;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;
  INTN                      Result;

  Allocations  = mPoolAllocations;
  SuffixOffset = 24 + DevicePathNodeLength ((EFI_DEVICE_PATH_PROTOCOL *)(Input + 24));
  DevicePath   = AllocateCopyPool (Size, Input);
  OriginalPath = DevicePath;
  Result       = OcFixAppleBootDevicePath (&DevicePath, &RemainingDevicePath);

  Check ((Result > 0) == Expected, Name);
  Check ((UINTN)RemainingDevicePath >= (UINTN)DevicePath && (UINTN)RemainingDevicePath < (UINTN)DevicePath + GetDevicePathSize (DevicePath), "valid remainder allocation");
  if (Expected) {
    Check (GetDevicePathSize (DevicePath) == Size + 40 - SuffixOffset, "correct replacement size");
    Check (memcmp (DevicePath, mPaths[0], 40) == 0, "exact firmware disk path");
    Check (memcmp ((UINT8 *)DevicePath + 40, Input + SuffixOffset, Size - SuffixOffset) == 0, "unchanged suffix");
  } else {
    Check (DevicePath == OriginalPath && memcmp (DevicePath, Input, Size) == 0, "failure preserves allocation and bytes");
  }

  FreePool (DevicePath);
  Check (mPoolAllocations == Allocations, "resolver releases temporary allocations");
  ++mTests;
  printf ("PASS: %s\n", Name);
}

STATIC
VOID
TestVirtioCleanup (
  VOID
  )
{
  UINT8                     Input[24 + 36 + 16 + 4];
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;
  UINTN                     Allocations;

  Reset ();
  memcpy (Input, mImage, 24);
  memset (Input + 24, 0x42, 36);
  Input[24] = MEDIA_DEVICE_PATH;
  Input[25] = MEDIA_VENDOR_DP;
  Input[26] = 36;
  Input[27] = 0;
  memcpy (Input + 60, mImage + 32, sizeof (mImage) - 32);

  memset (mPaths[0] + 24, 0, 42);
  mPaths[0][24] = MEDIA_DEVICE_PATH;
  mPaths[0][25] = MEDIA_HARDDRIVE_DP;
  mPaths[0][26] = 42;
  memcpy (mPaths[0] + 66, mImage + sizeof (mImage) - 4, 4);
  mMedia[0].LogicalPartition = TRUE;

  Allocations = mPoolAllocations;
  DevicePath  = AllocateCopyPool (sizeof (Input), Input);
  Check (OcFixAppleBootDevicePath (&DevicePath, &RemainingDevicePath) == 1, "existing VirtIO expansion");
  Check (GetDevicePathSize (DevicePath) == sizeof (Input) + 42, "VirtIO inserts HD node");
  Check (memcmp (DevicePath, mPaths[0], 66) == 0 && memcmp ((UINT8 *)DevicePath + 66, Input + 24, sizeof (Input) - 24) == 0, "VirtIO retains suffix");
  FreePool (DevicePath);
  Check (mPoolAllocations == Allocations, "VirtIO expansion releases original allocation");
  ++mTests;
  printf ("PASS: existing VirtIO expansion releases original allocation\n");
}

int
main (
  int   argc,
  char  **argv
  )
{
  UINT8                        OtherNode[sizeof (mImage)];
  UINT8                        Extended[sizeof (mImage) + 4];
  UINT8                        MultiInstance[sizeof (mImage) * 2];
  UINT8                        Sata[sizeof (mImage) + 2];
  UINT8                        Disk[36];
  UINT8                        Partition[32 + 42 + 4];
  UINT8                        Apfs[sizeof (mImage) + 42 + 36];
  EFI_DEVICE_PATH_PROTOCOL     *DevicePath, *Node;
  APPLE_BOOT_DP_PATCH_CONTEXT  Context;

  TestVirtioCleanup ();
  Reset ();
  mPaths[0][6]    = 8;
  mRequireRootFix = 1;
  Run ("ACPI correction followed by ATAPI expansion", mImage, sizeof (mImage), 1);
  Reset ();
  memcpy (Extended, mImage, 32);
  memset (Extended + 32, 0, 4);
  Extended[26] = 12;
  memcpy (Extended + 36, mImage + 32, sizeof (mImage) - 32);
  Run ("extended ATAPI node", Extended, sizeof (Extended), 1);
  Reset ();
  Run ("Apple NVMe node", mImage, sizeof (mImage), 1);
  Reset ();
  mPaths[0][25] = MSG_NVME_NAMESPACE_DP;
  Run ("UEFI NVMe node", mImage, sizeof (mImage), 1);
  Reset ();
  Run ("working NVMe path unchanged", mPaths[0], GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *)mPaths[0]), 0);
  Reset ();
  memcpy (Sata, mImage, 24);
  memset (Sata + 24, 0, 10);
  Sata[24] = MESSAGING_DEVICE_PATH;
  Sata[25] = MSG_SATA_DP;
  Sata[26] = 10;
  memcpy (Sata + 34, mImage + 32, sizeof (mImage) - 32);
  mAlreadyValid = 1;
  Run ("working SATA path unchanged", Sata, sizeof (Sata), 0);
  Reset ();
  mHandleCount = 2;
  Run ("two namespaces rejected", mImage, sizeof (mImage), 0);
  Reset ();
  mHandleCount  = 2;
  mPaths[1][17] = 0x1d;
  Run ("other PCI controller ignored", mImage, sizeof (mImage), 1);
  Reset ();
  mPaths[0][17] = 0x1d;
  Run ("wrong PCI controller rejected", mImage, sizeof (mImage), 0);
  Reset ();
  mMedia[0].LogicalPartition = TRUE;
  memset (mPaths[0] + 40, 0, 42);
  mPaths[0][40] = MEDIA_DEVICE_PATH;
  mPaths[0][41] = MEDIA_HARDDRIVE_DP;
  mPaths[0][42] = 42;
  memcpy (mPaths[0] + 82, mImage + sizeof (mImage) - 4, 4);
  Run ("partition handle excluded by path", mImage, sizeof (mImage), 0);
  Reset ();
  mMedia[0].RemovableMedia = TRUE;
  Run ("removable NVMe disk", mImage, sizeof (mImage), 1);
  Reset ();
  mMedia[0].MediaPresent = FALSE;
  Run ("unset media-present flag", mImage, sizeof (mImage), 1);
  Reset ();
  mBlockIo[0].Media = NULL;
  Run ("device path does not require media metadata", mImage, sizeof (mImage), 1);
  Reset ();
  mNoHandles = 1;
  Run ("enumeration failure unchanged", mImage, sizeof (mImage), 0);
  Reset ();
  mAlreadyValid = 1;
  Run ("working original path unchanged", mImage, sizeof (mImage), 0);
  Reset ();
  mRejectFixed = 1;
  Run ("replacement must resolve", mImage, sizeof (mImage), 0);
  Reset ();
  mPaths[0][25] = MSG_USB_DP;
  Run ("non-NVMe disk rejected", mImage, sizeof (mImage), 0);
  Reset ();
  memcpy (OtherNode, mImage, sizeof (mImage));
  OtherNode[25] = MSG_USB_DP;
  Run ("non-ATAPI input unchanged", OtherNode, sizeof (OtherNode), 0);
  Reset ();
  memcpy (Disk, mImage, 32);
  memcpy (Disk + 32, mImage + sizeof (mImage) - 4, 4);
  Run ("whole disk without file suffix", Disk, sizeof (Disk), 1);
  Reset ();
  memset (Partition, 0, sizeof (Partition));
  memcpy (Partition, mImage, 32);
  Partition[32] = MEDIA_DEVICE_PATH;
  Partition[33] = MEDIA_HARDDRIVE_DP;
  Partition[34] = 42;
  memcpy (Partition + 74, Disk + 32, 4);
  Run ("partition path", Partition, sizeof (Partition), 1);
  Reset ();
  memcpy (Apfs, Partition, 74);
  memset (Apfs + 74, 0x42, 36);
  Apfs[74] = MEDIA_DEVICE_PATH;
  Apfs[75] = MEDIA_VENDOR_DP;
  Apfs[76] = 36;
  Apfs[77] = 0;
  memcpy (Apfs + 110, mImage + 32, sizeof (mImage) - 32);
  Run ("partition APFS file path", Apfs, sizeof (Apfs), 1);
  Reset ();
  mAlreadyValid = 1;
  Run ("working ATA partition path", Partition, sizeof (Partition), 0);
  Reset ();
  mAlreadyValid = 1;
  Run ("working ATA disk path", Disk, sizeof (Disk), 0);
  Reset ();
  memcpy (MultiInstance, mImage, sizeof (mImage));
  memcpy (MultiInstance+sizeof (mImage), mImage, sizeof (mImage));
  MultiInstance[49] = 1;
  Run ("first instance ATAPI expansion", MultiInstance, sizeof (MultiInstance), 1);
  Reset ();
  mAlreadyValid = 1;
  DevicePath    = AllocateCopyPool (sizeof (mImage), mImage);
  Node          = (EFI_DEVICE_PATH_PROTOCOL *)((UINT8 *)DevicePath + 24);
  Check (OcFixAppleBootDevicePathNode (&DevicePath, &Node, &Context, NULL) == 0, "node API preserves working ATA");
  Check (Context.OldPath == NULL && memcmp (DevicePath, mImage, sizeof (mImage)) == 0, "working node has no restore allocation");
  FreePool (DevicePath);
  ++mTests;
  Reset ();
  DevicePath = AllocateCopyPool (sizeof (mImage), mImage);
  Node       = (EFI_DEVICE_PATH_PROTOCOL *)((UINT8 *)DevicePath + 24);
  Check (OcFixAppleBootDevicePathNode (&DevicePath, &Node, &Context, NULL) == 1, "node API repairs ATAPI");
  Check (Context.OldPath != NULL && memcmp (Context.OldPath, mImage, sizeof (mImage)) == 0, "restore context keeps old allocation");
  OcFixAppleBootDevicePathNodeRestore (&DevicePath, &Node, &Context);
  Check (memcmp (DevicePath, mImage, sizeof (mImage)) == 0 && (UINT8 *)Node == (UINT8 *)DevicePath + 24, "node API rollback restores bytes and cursor");
  FreePool (DevicePath);
  ++mTests;
  Reset ();
  DevicePath = AllocateCopyPool (sizeof (mImage), mImage);
  Node       = (EFI_DEVICE_PATH_PROTOCOL *)((UINT8 *)DevicePath + 24);
  Check (OcFixAppleBootDevicePathNode (&DevicePath, &Node, &Context, NULL) == 1, "node API commits repair");
  OcFixAppleBootDevicePathNodeRestoreFree (DevicePath, &Context);
  FreePool (DevicePath);
  ++mTests;
  Reset ();
  DevicePath = AllocateCopyPool (sizeof (mImage), mImage);
  Node       = (EFI_DEVICE_PATH_PROTOCOL *)((UINT8 *)DevicePath + 24);
  Check (OcFixAppleBootDevicePathNode (&DevicePath, &Node, NULL, NULL) == 1, "node API without restore context");
  Check (memcmp (DevicePath, mPaths[0], 40) == 0 && memcmp ((UINT8 *)DevicePath + 40, mImage + 32, sizeof (mImage) - 32) == 0, "no-context replacement retains suffix");
  FreePool (DevicePath);
  ++mTests;
  if (argc == 2) {
    UINT8  Captured[4096];
    FILE   *File = fopen (argv[1], "rb");
    Check (File != NULL, "open captured fixture");
    size_t  Size = fread (Captured, 1, sizeof (Captured), File);
    Check (feof (File) && Size >= sizeof (mImage), "read captured fixture");
    fclose (File);
    Reset ();
    Run ("actual captured boot-image", Captured, Size, 1);
  }

  Check (mPoolAllocations == 0, "all pool allocations released");
  printf ("%u regression checks passed\n", mTests);
  return 0;
}
