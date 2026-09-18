/** @file
  Copyright (C) 2022, Marvin Haeuser. All rights reserved.
  Copyright (C) 2022, PMheart. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcTemplateLib.h>
#include <Library/OcSerializeLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcAppleKernelLib.h>

#include <Library/OcConfigurationLib.h>
#include <Library/OcMainLib.h>

#include <UserFile.h>

#define  OC_USER_FULL_PATH_MAX_SIZE  256

STATIC CHAR8  mFullPath[OC_USER_FULL_PATH_MAX_SIZE] = { 0 };
STATIC UINTN  mRootPathLen                          = 0;

STATIC
BOOLEAN
UserSetRootPath (
  IN CONST CHAR8  *RootPath
  )
{
  UINTN  RootPathLen;

  RootPathLen = AsciiStrLen (RootPath);
  //
  // Preserve 2 bytes for '/' and '\0'.
  //
  if (RootPathLen > OC_USER_FULL_PATH_MAX_SIZE - 2) {
    DEBUG ((DEBUG_ERROR, "RootPath is too long!\n"));
    return FALSE;
  }

  AsciiStrCpyS (mFullPath, sizeof (mFullPath) - 1, RootPath);
  //
  // If passed without '/' in the end, append it.
  //
  if (mFullPath[RootPathLen - 1] != '/') {
    mFullPath[RootPathLen]     = '/';
    mFullPath[RootPathLen + 1] = '\0';
    mRootPathLen               = RootPathLen + 1;
  } else {
    mRootPathLen = RootPathLen;
  }

  DEBUG ((DEBUG_ERROR, "Root Path: %a\n", mFullPath));
  return TRUE;
}

STATIC
UINT8 *
UserReadFileFromRoot (
  IN  CONST CHAR8  *FileName,
  OUT UINT32       *Size
  )
{
  AsciiStrCpyS (&mFullPath[mRootPathLen], sizeof (mFullPath) - mRootPathLen - 1, FileName);
  DEBUG ((DEBUG_ERROR, "Full path: %a\n", mFullPath));
  return UserReadFile (mFullPath, Size);
}

STATIC BOOLEAN  FailedToProcess = FALSE;
STATIC UINT32   KernelVersion   = 0;

STATIC EFI_FILE_PROTOCOL  NilFileProtocol;

STATIC UINT8   *mPrelinked    = NULL;
STATIC UINT32  mPrelinkedSize = 0;

//
// TODO: Windows portability.
//
STATIC
VOID
AsciiHostSlashes (
  IN OUT CHAR8  *String
  )
{
  CHAR8  *Needle;

  Needle = String;
  while ((Needle = AsciiStrStr (Needle, "\\")) != NULL) {
    *Needle++ = '/';
  }
}

STATIC
EFI_STATUS
UserOcKernelLoadAndReserveKext (
  IN     OC_KERNEL_ADD_ENTRY  *Kext,
  IN     UINT32               Index,
  IN     OC_GLOBAL_CONFIG     *Config,
  IN     BOOLEAN              Is32Bit,
  IN OUT UINT32               *ReservedExeSize,
  IN OUT UINT32               *ReservedInfoSize,
  IN OUT UINT32               *NumReservedKexts
  )
{
  EFI_STATUS   Status;
  CHAR8        *BundlePath;
  CHAR8        *Comment;
  CONST CHAR8  *Arch;
  CHAR8        *PlistPath;
  CHAR8        *ExecutablePath;
  CHAR8        FullPath[OC_STORAGE_SAFE_PATH_MAX];

  if (!Kext->Enabled) {
    return EFI_SUCCESS;
  }

  BundlePath = OC_BLOB_GET (&Kext->BundlePath);
  Comment    = OC_BLOB_GET (&Kext->Comment);
  Arch       = OC_BLOB_GET (&Kext->Arch);
  PlistPath  = OC_BLOB_GET (&Kext->PlistPath);
  if ((BundlePath[0] == '\0') || (PlistPath[0] == '\0')) {
    DEBUG ((
      DEBUG_ERROR,
      "OC: Injected kext %u (%a) has invalid info\n",
      Index,
      Comment
      ));
    Kext->Enabled = FALSE;
    return EFI_INVALID_PARAMETER;
  }

  if (AsciiStrCmp (Arch, Is32Bit ? "x86_64" : "i386") == 0) {
    DEBUG ((
      DEBUG_INFO,
      "OC: Injected kext %a (%a) at %u skipped due to arch %a != %a\n",
      BundlePath,
      Comment,
      Index,
      Arch,
      Is32Bit ? "i386" : "x86_64"
      ));
    return EFI_SUCCESS;
  }

  //
  // Required for possible cacheless force injection later on.
  //
  AsciiHostSlashes (BundlePath);

  //
  // Get plist path and data.
  //
  Status = OcAsciiSafeSPrint (
             FullPath,
             sizeof (FullPath),
             "%s%a\\%a",
             OPEN_CORE_KEXT_PATH,
             BundlePath,
             PlistPath
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "OC: Failed to fit injected kext path %s%a\\%a",
      OPEN_CORE_KEXT_PATH,
      BundlePath,
      PlistPath
      ));
    Kext->Enabled = FALSE;
    return Status;
  }

  AsciiHostSlashes (FullPath);

  Kext->PlistData = (CHAR8 *)UserReadFileFromRoot (
                               FullPath,
                               &Kext->PlistDataSize
                               );
  if (Kext->PlistData == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "OC: Plist injected is missing for %s kext %a (%a)\n",
      FullPath,
      BundlePath,
      Comment
      ));
    Kext->Enabled = FALSE;
    return EFI_UNSUPPORTED;
  }

  //
  // Get executable path and data, if present.
  //
  ExecutablePath = OC_BLOB_GET (&Kext->ExecutablePath);
  if (ExecutablePath[0] != '\0') {
    Status = OcAsciiSafeSPrint (
               FullPath,
               sizeof (FullPath),
               "%s%a\\%a",
               OPEN_CORE_KEXT_PATH,
               BundlePath,
               ExecutablePath
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "OC: Failed to fit injected kext path %s%a\\%a",
        OPEN_CORE_KEXT_PATH,
        BundlePath,
        ExecutablePath
        ));
      Kext->Enabled = FALSE;
      FreePool (Kext->PlistData);
      Kext->PlistData = NULL;
      return Status;
    }

    AsciiHostSlashes (FullPath);

    Kext->ImageData = UserReadFileFromRoot (
                        FullPath,
                        &Kext->ImageDataSize
                        );
    if (Kext->ImageData == NULL) {
      DEBUG ((
        DEBUG_ERROR,
        "OC: Image injected is missing for %a kext %a (%a)\n",
        FullPath,
        BundlePath,
        Comment
        ));
      Kext->Enabled = FALSE;
      FreePool (Kext->PlistData);
      Kext->PlistData = NULL;
      return EFI_UNSUPPORTED;
    }
  }

  Status = PrelinkedReserveKextSize (
             ReservedInfoSize,
             ReservedExeSize,
             Kext->PlistDataSize,
             Kext->ImageData,
             Kext->ImageDataSize,
             Is32Bit
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OC: Failed to fit %s kext %a (%a) - %r\n",
      Is32Bit ? L"32-bit" : L"64-bit",
      BundlePath,
      Comment,
      Status
      ));
    if (Kext->ImageData != NULL) {
      FreePool (Kext->ImageData);
      Kext->ImageData = NULL;
    }

    FreePool (Kext->PlistData);
    Kext->PlistData = NULL;
    return Status;
  }

  (*NumReservedKexts)++;

  return EFI_SUCCESS;
}

EFI_STATUS
OcGetFileData (
  IN  EFI_FILE_PROTOCOL  *File,
  IN  UINT32             Position,
  IN  UINT32             Size,
  OUT UINT8              *Buffer
  )
{
  ASSERT (File == &NilFileProtocol);

  if ((UINT64)Position + Size > mPrelinkedSize) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&Buffer[0], &mPrelinked[Position], Size);
  return EFI_SUCCESS;
}

EFI_STATUS
OcGetFileSize (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT UINT32             *Size
  )
{
  ASSERT (File == &NilFileProtocol);

  *Size = mPrelinkedSize;
  return EFI_SUCCESS;
}

//
// Synthetic chained-fixup buffer used by --test-fixup-walk to exercise
// KcWalkChainedFixupsInSegment for both supported pointer formats.
//
#define TEST_FIXUP_PAGE_SIZE   0x1000U
#define TEST_FIXUP_PAGE_COUNT  1
#define TEST_FIXUP_BUFFER_SZ   (TEST_FIXUP_PAGE_SIZE * TEST_FIXUP_PAGE_COUNT)

STATIC
VOID
TestFixupVisitor (
  IN OUT UINT64  *FixupLoc,
  IN OUT VOID    *VisitorContext
  )
{
  UINTN  *VisitedAddresses;

  VisitedAddresses = (UINTN *)VisitorContext;
  if (VisitedAddresses != NULL) {
    VisitedAddresses[*VisitedAddresses + 1] = (UINTN)FixupLoc;
    ++*VisitedAddresses;
  }
}

STATIC
INT32
RunFixupWalkTest (
  VOID
  )
{
  UINT8                                         *Buffer;
  UINT8                                         StartsBuffer[sizeof (MACH_DYLD_CHAINED_STARTS_IN_SEGMENT) + sizeof (UINT16) * TEST_FIXUP_PAGE_COUNT];
  MACH_DYLD_CHAINED_STARTS_IN_SEGMENT           *StartsSeg;
  MACH_DYLD_CHAINED_PTR_64_KERNEL_CACHE_REBASE  *Slot;
  UINTN                                         Count;
  UINTN                                         VisitedAddresses[8];
  UINTN                                         ByteIdx;
  INT32                                         FailCount;

  FailCount = 0;
  Buffer    = AllocateZeroPool (TEST_FIXUP_BUFFER_SZ);
  if (Buffer == NULL) {
    return -1;
  }

  ZeroMem (StartsBuffer, sizeof (StartsBuffer));
  StartsSeg                = (MACH_DYLD_CHAINED_STARTS_IN_SEGMENT *)StartsBuffer;
  StartsSeg->Size          = sizeof (StartsBuffer);
  StartsSeg->PageSize      = TEST_FIXUP_PAGE_SIZE;
  StartsSeg->SegmentOffset = 0;
  StartsSeg->PageCount     = TEST_FIXUP_PAGE_COUNT;
  StartsSeg->PageStart[0]  = 0;

  //
  // Lay down a 4-link chain at offsets 0, 16, 32, 48 within the page.
  // For X86_64_KERNEL_CACHE (stride 1) Next = 16; for 64_KERNEL_CACHE
  // (stride 4) Next = 4. Both encode the same byte distance between
  // slots, so the walker must produce identical visit counts and
  // addresses for both layouts.
  //
  Slot = (MACH_DYLD_CHAINED_PTR_64_KERNEL_CACHE_REBASE *)Buffer;
  ZeroMem (Slot, sizeof (*Slot));
  Slot[0].Next   = 16;
  Slot[0].Target = 0xAAAA;
  ZeroMem (&Slot[2], sizeof (*Slot));
  Slot[2].Next   = 16;
  Slot[2].Target = 0xBBBB;
  ZeroMem (&Slot[4], sizeof (*Slot));
  Slot[4].Next   = 16;
  Slot[4].Target = 0xCCCC;
  ZeroMem (&Slot[6], sizeof (*Slot));
  Slot[6].Next   = 0;
  Slot[6].Target = 0xDDDD;

  StartsSeg->PointerFormat = MACH_DYLD_CHAINED_PTR_X86_64_KERNEL_CACHE;
  ZeroMem (VisitedAddresses, sizeof (VisitedAddresses));
  Count = KcWalkChainedFixupsInSegment (
            Buffer,
            TEST_FIXUP_BUFFER_SZ,
            StartsSeg,
            sizeof (StartsBuffer),
            TestFixupVisitor,
            VisitedAddresses
            );
  if ((Count != 4) || (VisitedAddresses[0] != 4)) {
    DEBUG ((DEBUG_ERROR, "[FAIL] X86_64_KERNEL_CACHE walk: %u fixups\n", (UINT32)Count));
    ++FailCount;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] X86_64_KERNEL_CACHE walk visited 4 fixups (stride 1)\n"));
  }

  //
  // Reset slots with stride-4 Next encoding.
  //
  ZeroMem (Slot, sizeof (*Slot));
  Slot[0].Next   = 4;
  Slot[0].Target = 0xAAAA;
  ZeroMem (&Slot[2], sizeof (*Slot));
  Slot[2].Next   = 4;
  Slot[2].Target = 0xBBBB;
  ZeroMem (&Slot[4], sizeof (*Slot));
  Slot[4].Next   = 4;
  Slot[4].Target = 0xCCCC;
  ZeroMem (&Slot[6], sizeof (*Slot));
  Slot[6].Next   = 0;
  Slot[6].Target = 0xDDDD;

  StartsSeg->PointerFormat = MACH_DYLD_CHAINED_PTR_64_KERNEL_CACHE;
  ZeroMem (VisitedAddresses, sizeof (VisitedAddresses));
  Count = KcWalkChainedFixupsInSegment (
            Buffer,
            TEST_FIXUP_BUFFER_SZ,
            StartsSeg,
            sizeof (StartsBuffer),
            TestFixupVisitor,
            VisitedAddresses
            );
  if ((Count != 4) || (VisitedAddresses[0] != 4)) {
    DEBUG ((DEBUG_ERROR, "[FAIL] 64_KERNEL_CACHE walk: %u fixups\n", (UINT32)Count));
    ++FailCount;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] 64_KERNEL_CACHE walk visited 4 fixups (stride 4)\n"));
  }

  //
  // An unsupported format must return 0 and never invoke the visitor.
  //
  StartsSeg->PointerFormat = MACH_DYLD_CHAINED_PTR_ARM64E;
  ZeroMem (VisitedAddresses, sizeof (VisitedAddresses));
  Count = KcWalkChainedFixupsInSegment (
            Buffer,
            TEST_FIXUP_BUFFER_SZ,
            StartsSeg,
            sizeof (StartsBuffer),
            TestFixupVisitor,
            VisitedAddresses
            );
  if ((Count != 0) || (VisitedAddresses[0] != 0)) {
    DEBUG ((DEBUG_ERROR, "[FAIL] unsupported-format guard: %u fixups\n", (UINT32)Count));
    ++FailCount;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] unsupported-format guard returned 0\n"));
  }

  //
  // Bounds-check tests. Each constructs a malformed StartsSeg and
  // confirms the walker rejects it (returns 0, visits nothing) rather
  // than reading or writing out of the supplied buffers.
  //

  //
  // (a) PageCount lies about the array length. Buffer-of-record claims
  //     200 pages but only ~1 entry of metadata is supplied; walker
  //     must refuse rather than walk a UINT16 array off the end of
  //     StartsBuffer.
  //
  StartsSeg->PointerFormat = MACH_DYLD_CHAINED_PTR_X86_64_KERNEL_CACHE;
  StartsSeg->PageCount     = 200;
  ZeroMem (VisitedAddresses, sizeof (VisitedAddresses));
  Count = KcWalkChainedFixupsInSegment (
            Buffer,
            TEST_FIXUP_BUFFER_SZ,
            StartsSeg,
            sizeof (StartsBuffer),
            TestFixupVisitor,
            VisitedAddresses
            );
  if ((Count != 0) || (VisitedAddresses[0] != 0)) {
    DEBUG ((DEBUG_ERROR, "[FAIL] PageCount oversize guard: %u fixups\n", (UINT32)Count));
    ++FailCount;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] PageCount oversize rejected\n"));
  }

  //
  // (b) StartsSeg->Size > caller's StartsSegSize. Walker must refuse to
  //     trust the larger self-declared size.
  //
  StartsSeg->PageCount = TEST_FIXUP_PAGE_COUNT;
  StartsSeg->Size      = (UINT32)sizeof (StartsBuffer) + 64;
  ZeroMem (VisitedAddresses, sizeof (VisitedAddresses));
  Count = KcWalkChainedFixupsInSegment (
            Buffer,
            TEST_FIXUP_BUFFER_SZ,
            StartsSeg,
            sizeof (StartsBuffer),
            TestFixupVisitor,
            VisitedAddresses
            );
  if ((Count != 0) || (VisitedAddresses[0] != 0)) {
    DEBUG ((DEBUG_ERROR, "[FAIL] Size > StartsSegSize guard: %u fixups\n", (UINT32)Count));
    ++FailCount;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Size > StartsSegSize rejected\n"));
  }

  //
  // (c) SegmentOffset past end of buffer. Walker must not deref past
  //     Buffer + BufferSize even though Buffer itself is non-NULL.
  //
  StartsSeg->Size          = (UINT32)sizeof (StartsBuffer);
  StartsSeg->SegmentOffset = TEST_FIXUP_BUFFER_SZ;
  ZeroMem (VisitedAddresses, sizeof (VisitedAddresses));
  Count = KcWalkChainedFixupsInSegment (
            Buffer,
            TEST_FIXUP_BUFFER_SZ,
            StartsSeg,
            sizeof (StartsBuffer),
            TestFixupVisitor,
            VisitedAddresses
            );
  if ((Count != 0) || (VisitedAddresses[0] != 0)) {
    DEBUG ((DEBUG_ERROR, "[FAIL] SegmentOffset OOB guard: %u fixups\n", (UINT32)Count));
    ++FailCount;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] SegmentOffset out-of-buffer rejected\n"));
  }

  //
  // (d) PageStart >= PageSize — chain head outside its own page. Page
  //     is skipped; walker returns count 0 (nothing to visit).
  //
  StartsSeg->SegmentOffset = 0;
  StartsSeg->PageStart[0]  = TEST_FIXUP_PAGE_SIZE;
  ZeroMem (VisitedAddresses, sizeof (VisitedAddresses));
  Count = KcWalkChainedFixupsInSegment (
            Buffer,
            TEST_FIXUP_BUFFER_SZ,
            StartsSeg,
            sizeof (StartsBuffer),
            TestFixupVisitor,
            VisitedAddresses
            );
  if ((Count != 0) || (VisitedAddresses[0] != 0)) {
    DEBUG ((DEBUG_ERROR, "[FAIL] PageStart >= PageSize guard: %u fixups\n", (UINT32)Count));
    ++FailCount;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] PageStart >= PageSize rejected\n"));
  }

  //
  // (e) Pathological chain: every 4-byte step reads as Next=1, so the
  //     chain walks forward 4 bytes at a time and would visit
  //     PageSize/4 = 1024 slots if unchecked. The iteration cap
  //     (PageSize / sizeof(UINT64) = 512) must stop it short.
  //
  // Pattern: set byte i = 0x08 for i = 6, 10, 14, ..., so that the
  // 8-byte read at every 4-byte-aligned offset has bit 51 set and all
  // other Next-field bits clear, decoding to Next = 1. The
  // MACH_DYLD_CHAINED_PTR_64_KERNEL_CACHE_REBASE bitfield places Next
  // at bits 51-62 of the 64-bit slot (Apple mach-o/fixup-chains.h).
  //
  ZeroMem (Buffer, TEST_FIXUP_BUFFER_SZ);
  for (ByteIdx = 6; ByteIdx + 1 < TEST_FIXUP_BUFFER_SZ; ByteIdx += 4) {
    Buffer[ByteIdx] = 0x08;
  }

  StartsSeg->PointerFormat = MACH_DYLD_CHAINED_PTR_64_KERNEL_CACHE;
  StartsSeg->PageStart[0]  = 0;
  ZeroMem (VisitedAddresses, sizeof (VisitedAddresses));
  Count = KcWalkChainedFixupsInSegment (
            Buffer,
            TEST_FIXUP_BUFFER_SZ,
            StartsSeg,
            sizeof (StartsBuffer),
            TestFixupVisitor,
            VisitedAddresses
            );
  //
  // Cap is PageSize / sizeof(UINT64) = 0x1000 / 8 = 512.
  // Walker must stop at exactly that count rather than walking 1024.
  //
  if (Count != TEST_FIXUP_PAGE_SIZE / sizeof (UINT64)) {
    DEBUG ((DEBUG_ERROR, "[FAIL] iter cap: %u fixups (expected %u)\n", (UINT32)Count, (UINT32)(TEST_FIXUP_PAGE_SIZE / sizeof (UINT64))));
    ++FailCount;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] long-chain bounded at iteration cap (%u)\n", (UINT32)Count));
  }

  //
  // (f) Chain step would walk off the end of its page. Walker must
  //     stop before reading past the page boundary.
  //
  StartsSeg->PageStart[0] = (UINT16)(TEST_FIXUP_PAGE_SIZE - sizeof (UINT64));
  Slot                    = (MACH_DYLD_CHAINED_PTR_64_KERNEL_CACHE_REBASE *)
                            (Buffer + (TEST_FIXUP_PAGE_SIZE - sizeof (UINT64)));
  ZeroMem (Slot, sizeof (*Slot));
  Slot[0].Next   = 64;      // would step past the page end
  Slot[0].Target = 0xFFFF;

  ZeroMem (VisitedAddresses, sizeof (VisitedAddresses));
  Count = KcWalkChainedFixupsInSegment (
            Buffer,
            TEST_FIXUP_BUFFER_SZ,
            StartsSeg,
            sizeof (StartsBuffer),
            TestFixupVisitor,
            VisitedAddresses
            );
  //
  // The starting slot is in-page, so it's visited once; the next step
  // is rejected and the walk terminates.
  //
  if ((Count != 1) || (VisitedAddresses[0] != 1)) {
    DEBUG ((DEBUG_ERROR, "[FAIL] off-page chain step guard: %u fixups\n", (UINT32)Count));
    ++FailCount;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] chain step past page-end rejected after 1 visit\n"));
  }

  //
  // (g) Image-level walker: NumSegments lies. Walker must validate
  //     against StartsSize and return 0 cleanly.
  //
  {
    UINT8                              ImageStartsBuffer[sizeof (MACH_DYLD_CHAINED_STARTS_IN_IMAGE) + sizeof (UINT32)];
    MACH_DYLD_CHAINED_STARTS_IN_IMAGE  *ImageStarts;

    ZeroMem (ImageStartsBuffer, sizeof (ImageStartsBuffer));
    ImageStarts                   = (MACH_DYLD_CHAINED_STARTS_IN_IMAGE *)ImageStartsBuffer;
    ImageStarts->NumSegments      = 1000;
    ImageStarts->SegInfoOffset[0] = 0;

    ZeroMem (VisitedAddresses, sizeof (VisitedAddresses));
    Count = KcWalkChainedFixupsInImage (
              Buffer,
              TEST_FIXUP_BUFFER_SZ,
              ImageStarts,
              sizeof (ImageStartsBuffer),
              TestFixupVisitor,
              VisitedAddresses
              );
    if ((Count != 0) || (VisitedAddresses[0] != 0)) {
      DEBUG ((DEBUG_ERROR, "[FAIL] InImage NumSegments guard: %u fixups\n", (UINT32)Count));
      ++FailCount;
    } else {
      DEBUG ((DEBUG_WARN, "[OK] InImage oversize NumSegments rejected\n"));
    }
  }

  FreePool (Buffer);
  return FailCount;
}

int
WrapMain (
  int   argc,
  char  *argv[]
  )
{
  UINT8             *ConfigFileBuffer;
  UINT32            ConfigFileSize;
  OC_GLOBAL_CONFIG  Config;
  EFI_STATUS        Status;
  UINT32            ErrorCount;
  UINT32            Index;
  UINT32            AllocSize;
  EFI_STATUS        PrelinkedStatus;

  CONST CHAR8  *FileName;

  BOOLEAN  mUse32BitKernel;
  UINT32   ReservedInfoSize;
  UINT32   ReservedExeSize;
  UINT32   NumReservedKexts;
  UINT32   LinkedExpansion;
  UINT8    *NewPrelinked;
  UINT32   NewPrelinkedSize;
  UINT8    Sha384[48];
  BOOLEAN  Is32Bit;

  OC_CPU_INFO  DummyCpuInfo;

  OC_KERNEL_ADD_ENTRY  *Kext;

  if (argc < 2) {
    DEBUG ((DEBUG_ERROR, "Usage: %a <path/to/OC/folder/> [path/to/kernel]\n", argv[0]));
    DEBUG ((DEBUG_ERROR, "       %a --test-fixup-walk\n\n", argv[0]));
    return -1;
  }

  if (AsciiStrCmp (argv[1], "--test-fixup-walk") == 0) {
    return RunFixupWalkTest () != 0 ? -1 : 0;
  }

  FileName = argc > 2 ? argv[2] : "/System/Library/PrelinkedKernels/prelinkedkernel";
  if ((mPrelinked = UserReadFile (FileName, &mPrelinkedSize)) == NULL) {
    DEBUG ((DEBUG_ERROR, "Read fail %a\n", FileName));
    return -1;
  }

  if (!UserSetRootPath (argv[1])) {
    return -1;
  }

  //
  // Read config file (Only one single config is supported).
  //
  CHAR8  AsciiOcConfig[16];

  UnicodeStrToAsciiStrS (OPEN_CORE_CONFIG_PATH, AsciiOcConfig, L_STR_SIZE (OPEN_CORE_CONFIG_PATH));
  ConfigFileBuffer = UserReadFileFromRoot (AsciiOcConfig, &ConfigFileSize);
  if (ConfigFileBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to read %s\n", OPEN_CORE_CONFIG_PATH));
    return -1;
  }

  //
  // Initialise config structure to be checked, and exit on error.
  //
  ErrorCount = 0;
  Status     = OcConfigurationInit (&Config, ConfigFileBuffer, ConfigFileSize, &ErrorCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Invalid config\n"));
    return -1;
  }

  if (ErrorCount > 0) {
    DEBUG ((DEBUG_ERROR, "Serialisation returns %u %a!\n", ErrorCount, ErrorCount > 1 ? "errors" : "error"));
  }

  PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_INFO;
  PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_INFO;
  PcdGet8 (PcdDebugPropertyMask)          |= DEBUG_PROPERTY_DEBUG_CODE_ENABLED;

  mUse32BitKernel  = FALSE;
  ReservedInfoSize = PRELINK_INFO_RESERVE_SIZE;
  ReservedExeSize  = 0;
  NumReservedKexts = 0;
  //
  // Process kexts to be injected.
  //
  for (Index = 0; Index < Config.Kernel.Add.Count; ++Index) {
    Kext = Config.Kernel.Add.Values[Index];

    Status = UserOcKernelLoadAndReserveKext (
               Kext,
               Index,
               &Config,
               mUse32BitKernel,
               &ReservedExeSize,
               &ReservedInfoSize,
               &NumReservedKexts
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[FAIL] Kernel load and reserve - %r\n", Status));
      FailedToProcess = TRUE;
      return -1;
    }
  }

  LinkedExpansion = KcGetSegmentFixupChainsSize (ReservedExeSize);
  if (LinkedExpansion == 0) {
    FailedToProcess = TRUE;
    return -1;
  }

  Status = ReadAppleKernel (
             &NilFileProtocol,
             FALSE,
             &Is32Bit,
             &NewPrelinked,
             &NewPrelinkedSize,
             &AllocSize,
             ReservedInfoSize + ReservedExeSize + LinkedExpansion,
             Sha384
             );
  if (!EFI_ERROR (Status)) {
    FreePool (mPrelinked);
    mPrelinked     = NewPrelinked;
    mPrelinkedSize = NewPrelinkedSize;
    DEBUG ((DEBUG_WARN, "[OK] Sha384 is %02X%02X%02X%02X\n", Sha384[0], Sha384[1], Sha384[2], Sha384[3]));
  } else {
    DEBUG ((DEBUG_WARN, "[FAIL] Kernel unpack failure - %r\n", Status));
    FailedToProcess = TRUE;
    return -1;
  }

  KernelVersion = OcKernelReadDarwinVersion (mPrelinked, mPrelinkedSize);
  if (KernelVersion != 0) {
    DEBUG ((DEBUG_WARN, "[OK] Got version %u\n", KernelVersion));
  } else {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to detect version\n"));
    FailedToProcess = TRUE;
  }

  ZeroMem (&DummyCpuInfo, sizeof (DummyCpuInfo));
  //
  // Disable ProvideCurrentCpuInfo patch, as there is no CpuInfo available on userspace.
  //
  Config.Kernel.Quirks.ProvideCurrentCpuInfo = FALSE;
  ASSERT (Config.Kernel.Quirks.ProvideCurrentCpuInfo == FALSE);

  ZeroMem (Config.Kernel.Emulate.Cpuid1Data, sizeof (Config.Kernel.Emulate.Cpuid1Data));
  Config.Kernel.Emulate.Cpuid1Data[0] = 0x000306A9;
  ZeroMem (Config.Kernel.Emulate.Cpuid1Mask, sizeof (Config.Kernel.Emulate.Cpuid1Mask));
  Config.Kernel.Emulate.Cpuid1Mask[0] = 0xFFFFFFFF;

  ASSERT (Config.Kernel.Force.Count == 0);

  //
  // Apply patches to kernel itself, and then process prelinked.
  //
  OcKernelApplyPatches (
    &Config,
    &DummyCpuInfo,
    KernelVersion,
    FALSE,
    CacheTypeNone,
    NULL,
    NewPrelinked,
    NewPrelinkedSize
    );
  PrelinkedStatus = OcKernelProcessPrelinked (
                      &Config,
                      KernelVersion,
                      FALSE,
                      NewPrelinked,
                      &NewPrelinkedSize,
                      AllocSize,
                      LinkedExpansion,
                      ReservedExeSize
                      );
  if (EFI_ERROR (PrelinkedStatus)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Kernel process - %r\n", PrelinkedStatus));
    FailedToProcess = TRUE;
    return -1;
  }

  DEBUG ((DEBUG_INFO, "OC: Prelinked status - %r\n", PrelinkedStatus));

  UserWriteFile ("out.bin", NewPrelinked, NewPrelinkedSize);

  FreePool (mPrelinked);

  return 0;
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  int  code;

  code = WrapMain (argc, argv);
  if (FailedToProcess) {
    code = -1;
  }

  return code;
}
