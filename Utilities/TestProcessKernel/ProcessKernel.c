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
    DEBUG ((DEBUG_ERROR, "Usage: %a <path/to/OC/folder/> [path/to/kernel]\n\n", argv[0]));
    return -1;
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
