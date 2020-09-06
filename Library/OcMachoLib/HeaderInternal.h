/**
  Provides services for Mach-O headers.

Copyright (C) 2016 - 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

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

#define MACH_LOAD_COMMAND_SEGMENT_X MACH_LOAD_COMMAND_SEGMENT

#define MACH_X(a)                   a##32
#define MACH_ASSERT_X               ASSERT (Context->Is32Bit)

//
// 64-bit functions
//
#else

#define MACH_UINT_X                 UINT64
#define MACH_HEADER_X               MACH_HEADER_64
#define MACH_SECTION_X              MACH_SECTION_64
#define MACH_SEGMENT_COMMAND_X      MACH_SEGMENT_COMMAND_64

#define MACH_LOAD_COMMAND_SEGMENT_X MACH_LOAD_COMMAND_SEGMENT_64

#define MACH_X(a)                   a##64
#define MACH_ASSERT_X               ASSERT (!Context->Is32Bit)
#endif

/**
  Returns whether Section is sane.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Section  Section to verify.
  @param[in]     Segment  Segment the section is part of.

**/
STATIC
BOOLEAN
InternalSectionIsSane (
  IN OUT OC_MACHO_CONTEXT               *Context,
  IN     CONST MACH_SECTION_X           *Section,
  IN     CONST MACH_SEGMENT_COMMAND_X   *Segment
  )
{
  MACH_UINT_X   TopOffsetX;
  UINT32        TopOffset32;
  MACH_UINT_X   TopOfSegment;
  BOOLEAN       Result;
  MACH_UINT_X   TopOfSection;

  ASSERT (Context != NULL);
  MACH_ASSERT_X;
  ASSERT (Section != NULL);
  ASSERT (Segment != NULL);
  //
  // Section->Alignment is stored as a power of 2.
  //
  if ((Section->Alignment > 31)
   || ((Section->Offset != 0) && (Section->Offset < Segment->FileOffset))) {
    return FALSE;
  }

  TopOfSegment = (Segment->VirtualAddress + Segment->Size);
  Result       = MACH_X (OcOverflowAddU) (
                   Section->Address,
                   Section->Size,
                   &TopOfSection
                   );
  if (Result || (TopOfSection > TopOfSegment)) {
    return FALSE;
  }

  Result   = MACH_X (OcOverflowAddU) (
                Section->Offset,
                Section->Size,
                &TopOffsetX
                );
  if (Result || (TopOffsetX > (Segment->FileOffset + Segment->FileSize))) {
    return FALSE;
  }

  if (Section->NumRelocations != 0) {
    Result = OcOverflowSubU32 (
                Section->RelocationsOffset,
                Context->ContainerOffset,
                &TopOffset32
                );
    Result |= OcOverflowMulAddU32 (
               Section->NumRelocations,
               sizeof (MACH_RELOCATION_INFO),
               TopOffset32,
               &TopOffset32
               );
    if (Result || (TopOffset32 > Context->FileSize)) {
      return FALSE;
    }
  }

  return TRUE;
}

MACH_HEADER_X *
MACH_X (MachoGetMachHeader) (
  IN OUT OC_MACHO_CONTEXT   *Context
  )
{
  ASSERT (Context != NULL);
  ASSERT (Context->MachHeaderAny != NULL);
  MACH_ASSERT_X;

  return MACH_X (&Context->MachHeaderAny->Header);
}

MACH_SEGMENT_COMMAND_X *
MACH_X (MachoGetSegmentByName) (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *SegmentName
  )
{
  MACH_SEGMENT_COMMAND_X  *Segment;
  INTN                    Result;

  ASSERT (Context != NULL);
  ASSERT (SegmentName != NULL);
  MACH_ASSERT_X;

  Result = 0;

  for (
    Segment = MACH_X (MachoGetNextSegment) (Context, NULL);
    Segment != NULL;
    Segment = MACH_X (MachoGetNextSegment) (Context, Segment)
    ) {
    Result = AsciiStrnCmp (
                Segment->SegmentName,
                SegmentName,
                ARRAY_SIZE (Segment->SegmentName)
                );
    if (Result == 0) {
      return Segment;
    }
  }

  return NULL;
}

MACH_SECTION_X *
MACH_X (MachoGetSectionByName) (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_SEGMENT_COMMAND_X   *Segment,
  IN     CONST CHAR8              *SectionName
  )
{
  MACH_SECTION_X  *Section;
  INTN            Result;

  ASSERT (Context != NULL);
  ASSERT (Segment != NULL);
  ASSERT (SectionName != NULL);
  MACH_ASSERT_X;

  for (
    Section = MACH_X (MachoGetNextSection) (Context, Segment, NULL);
    Section != NULL;
    Section = MACH_X (MachoGetNextSection) (Context, Segment, Section)
    ) {
    //
    // Assumption: Mach-O is not of type MH_OBJECT.
    // MH_OBJECT might have sections in segments they do not belong in for
    // performance reasons.  This library does not support intermediate
    // objects.
    //
    Result = AsciiStrnCmp (
               Section->SectionName,
               SectionName,
               ARRAY_SIZE (Section->SectionName)
               );
    if (Result == 0) {
      return Section;
    }
  }

  return NULL;
}

MACH_SECTION_X *
MACH_X (MachoGetSegmentSectionByName) (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *SegmentName,
  IN     CONST CHAR8       *SectionName
  )
{
  MACH_SEGMENT_COMMAND_X  *Segment;

  ASSERT (Context != NULL);
  ASSERT (SegmentName != NULL);
  ASSERT (SectionName != NULL);
  MACH_ASSERT_X;

  Segment = MACH_X (MachoGetSegmentByName) (Context, SegmentName);

  if (Segment != NULL) {
    return MACH_X (MachoGetSectionByName) (Context, Segment, SectionName);
  }

  return NULL;
}

MACH_SEGMENT_COMMAND_X *
MACH_X (MachoGetNextSegment) (
  IN OUT OC_MACHO_CONTEXT               *Context,
  IN     CONST MACH_SEGMENT_COMMAND_X   *Segment  OPTIONAL
  )
{
  MACH_SEGMENT_COMMAND_X  *NextSegment;

  CONST MACH_HEADER_X     *MachHeader;
  UINTN                   TopOfCommands;
  BOOLEAN                 Result;
  MACH_UINT_X             TopOfSegment;
  UINTN                   TopOfSections;

  ASSERT (Context != NULL);
  ASSERT (Context->FileSize > 0);
  MACH_ASSERT_X;

  if (Segment != NULL) {
    MachHeader    = MACH_X (MachoGetMachHeader) (Context);
    TopOfCommands = ((UINTN) MachHeader->Commands + MachHeader->CommandsSize);
    ASSERT (
      ((UINTN) Segment >= (UINTN) &MachHeader->Commands[0])
        && ((UINTN) Segment < TopOfCommands)
      );
  }
  //
  // Context initialisation guarantees the command size is a multiple of 8.
  //
  STATIC_ASSERT (
    OC_ALIGNOF (MACH_SEGMENT_COMMAND_X) <= sizeof (UINT64),
    "Alignment is not guaranteed."
    );
  NextSegment = (MACH_SEGMENT_COMMAND_X *) (VOID *) MachoGetNextCommand (
    Context,
    MACH_LOAD_COMMAND_SEGMENT_X,
    (CONST MACH_LOAD_COMMAND *) Segment
    );
  if (NextSegment == NULL || NextSegment->CommandSize < sizeof (*NextSegment)) {
    return NULL;
  }

  Result = OcOverflowMulAddUN (
             NextSegment->NumSections,
             sizeof (*NextSegment->Sections),
             (UINTN) NextSegment->Sections,
             &TopOfSections
             );
  if (Result || (((UINTN) NextSegment + NextSegment->CommandSize) < TopOfSections)) {
    return NULL;
  }

  Result = MACH_X (OcOverflowSubU) (
             NextSegment->FileOffset,
             Context->ContainerOffset,
             &TopOfSegment
             );
  Result |= MACH_X (OcOverflowAddU) (
              TopOfSegment,
              NextSegment->FileSize,
              &TopOfSegment
              );
  if (Result || (TopOfSegment > Context->FileSize)) {
    return NULL;
  }

  if (NextSegment->VirtualAddress + NextSegment->Size < NextSegment->VirtualAddress) {
    return NULL;
  }

  return NextSegment;
}

MACH_SECTION_X *
MACH_X (MachoGetNextSection) (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_SEGMENT_COMMAND_X   *Segment,
  IN     MACH_SECTION_X           *Section  OPTIONAL
  )
{
  ASSERT (Context != NULL);
  ASSERT (Segment != NULL);
  MACH_ASSERT_X;

  if (Section != NULL) {
    ASSERT (Section >= Segment->Sections);

    ++Section;

    if (Section >= &Segment->Sections[Segment->NumSections]) {
      return NULL;
    }
  } else if (Segment->NumSections > 0) {
    Section = &Segment->Sections[0];
  } else {
    return NULL;
  }

  if (!InternalSectionIsSane (Context, Section, Segment)) {
    return NULL;
  }

  return Section;
}

MACH_SECTION_X *
MACH_X (MachoGetSectionByIndex) (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT32            Index
  )
{
  MACH_SECTION_X          *Section;

  MACH_SEGMENT_COMMAND_X  *Segment;
  UINT32                  SectionIndex;
  UINT32                  NextSectionIndex;
  BOOLEAN                 Result;

  ASSERT (Context != NULL);
  MACH_ASSERT_X;

  SectionIndex = 0;

  Segment = NULL;
  for (
    Segment = MACH_X (MachoGetNextSegment) (Context, NULL);
    Segment != NULL;
    Segment = MACH_X (MachoGetNextSegment) (Context, Segment)
    ) {
    Result = OcOverflowAddU32 (
               SectionIndex,
               Segment->NumSections,
               &NextSectionIndex
               );
    //
    // If NextSectionIndex is wrapping around, Index must be contained.
    //
    if (Result || (Index < NextSectionIndex)) {
      Section = &Segment->Sections[Index - SectionIndex];
      if (!InternalSectionIsSane (Context, Section, Segment)) {
        return NULL;
      }

      return Section;
    }

    SectionIndex = NextSectionIndex;
  }

  return NULL;
}

MACH_SECTION_X *
MACH_X (MachoGetSectionByAddress) (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     MACH_UINT_X       Address
  )
{
  MACH_SEGMENT_COMMAND_X  *Segment;
  MACH_SECTION_X          *Section;
  MACH_UINT_X             TopOfSegment;
  MACH_UINT_X             TopOfSection;

  ASSERT (Context != NULL);
  MACH_ASSERT_X;

  for (
    Segment = MACH_X (MachoGetNextSegment) (Context, NULL);
    Segment != NULL;
    Segment = MACH_X (MachoGetNextSegment) (Context, Segment)
    ) {
    TopOfSegment = (Segment->VirtualAddress + Segment->Size);
    if ((Address >= Segment->VirtualAddress) && (Address < TopOfSegment)) {
      for (
        Section = MACH_X (MachoGetNextSection) (Context, Segment, NULL);
        Section != NULL;
        Section = MACH_X (MachoGetNextSection) (Context, Segment, Section)
        ) {
        TopOfSection = (Section->Address + Section->Size);
        if ((Address >= Section->Address) && (Address < TopOfSection)) {
          return Section;
        }
      }
    }
  }

  return NULL;
}
