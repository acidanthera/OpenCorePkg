/** @file
  Library handling KEXT prelinking.
  Currently limited to Intel 64 architectures.
  
Copyright (c) 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <IndustryStandard/AppleMachoImage.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcAppleKernelLib.h>

#include "PrelinkedInternal.h"

EFI_STATUS
PrelinkedLinkExecutable (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN OUT OC_MACHO_CONTEXT   *Executable,
  IN     XML_NODE           *PlistRoot,
  IN     UINT64             LoadAddress
  )
{
  EFI_STATUS     Status;
  PRELINKED_KEXT *Kext;

  Kext = InternalNewPrelinkedKext (Executable, PlistRoot);
  if (Kext == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = InternalScanPrelinkedKext (Kext, Context);
  if (EFI_ERROR (Status)) {
    InternalFreePrelinkedKext (Kext);
    return Status;
  }

  Status = InternalPrelinkKext64 (Context, Kext, LoadAddress);

  InternalFreePrelinkedKext (Kext);

  return Status;
}
