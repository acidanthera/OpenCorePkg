/**
  Mach-O library functions layer.

Copyright (C) 2020, Goldfish64.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef MACHO_X_INTERNAL_H
#define MACHO_X_INTERNAL_H

#include <Uefi.h>

#include <IndustryStandard/AppleMachoImage.h>
#include <IndustryStandard/AppleFatBinaryImage.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMachoLib.h>

#include "OcMachoLibInternal.h"

//
// 32-bit functions
//
#ifdef MACHO_LIB_32

#define MACH_UINT_X                 UINT32
#define MACH_HEADER_X               MACH_HEADER
#define MACH_SECTION_X              MACH_SECTION
#define MACH_SEGMENT_COMMAND_X      MACH_SEGMENT_COMMAND
#define MACH_NLIST_X                MACH_NLIST

#define MACH_LOAD_COMMAND_SEGMENT_X MACH_LOAD_COMMAND_SEGMENT

#define MACH_X(a)                   a##32
#define MACH_ASSERT_X(a)            ASSERT ((a)->Is32Bit)

#define MACH_X_TO_UINT32(a)         (a)

//
// 64-bit functions
//
#else

#define MACH_UINT_X                 UINT64
#define MACH_HEADER_X               MACH_HEADER_64
#define MACH_SECTION_X              MACH_SECTION_64
#define MACH_SEGMENT_COMMAND_X      MACH_SEGMENT_COMMAND_64
#define MACH_NLIST_X                MACH_NLIST_64

#define MACH_LOAD_COMMAND_SEGMENT_X MACH_LOAD_COMMAND_SEGMENT_64

#define MACH_X(a)                   a##64
#define MACH_ASSERT_X(a)            ASSERT (!(a)->Is32Bit)

#define MACH_X_TO_UINT32(a)         (UINT32)(a)
#endif

#endif // MACHO_X_INTERNAL_H
