/** @file
  Control CFGLock BIOS Option

Copyright (c) 2020, Brumbaer. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

///
/// Unless otherwise specified all data types are naturally aligned. Structures are
/// aligned on boundaries equal to the largest internal datum of the structure and
/// internal data are implicitly padded to achieve natural alignment.
///

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Register/Msr.h>
#include <Protocol/MpService.h>

#include <Library/MemoryAllocationLib.h>
#include <Uefi/UefiInternalFormRepresentation.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/HiiLib.h>

#include <Protocol/HiiFont.h>
#include <Protocol/HiiImage.h>
#include <Protocol/HiiImageEx.h>
#include <Protocol/HiiImageDecoder.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigKeyword.h>
#include <Guid/HiiPlatformSetupFormset.h>

#define DONT_STOP_AT 0xFFFF

/**
  Parameters to search in a Form for
  IfrHeaders which contain a Description
  and to return the VarStore

  @param SearchText        Text that must be part of the options description
  @param EfiHandle         Handle of the list the Form is part of
  @param ListHeader        Ptr to the contents of the handle
  @param PkgHeader         Ptr to the current package in the list
  @param FirstIfrHeader    Ptr to first IfrHeader in Package
  @param IfrVarStore       Ptr to IfrHeader of corresponding varstore in same list
  @param IfrOneOf          Ptr to IfrHeader of BIOS Option
  @param StopAt            Used to find a certain BIOS Option
  @param Count             Running number of suitable BIOS Options
**/
typedef struct ONE_OF_CONTEXT_ {
  EFI_STRING                    SearchText;
  EFI_HII_HANDLE                EfiHandle;
  EFI_HII_PACKAGE_LIST_HEADER   *ListHeader;
  EFI_HII_PACKAGE_HEADER        *PkgHeader;
  EFI_IFR_OP_HEADER             *FirstIfrHeader;
  EFI_IFR_VARSTORE              *IfrVarStore;
  EFI_IFR_ONE_OF                *IfrOneOf;
  UINT16                        StopAt;
  UINT16                        Count;
} ONE_OF_CONTEXT;

/**
  Callback - What to do, when a IfrHeader with a defined OP_CODE is found

  @param[in] IfrHeader   Current IfrHeader under scrutiny
  @param[in] Stop        Ptr to Stop flag. If TRUE no further search for opcodes. Can be NULL.
  @param[in] Context     Ptr to Hanlder specific data
**/
typedef
VOID
OP_CODE_HANDLER (
  IN     EFI_IFR_OP_HEADER *IfrHeader,
  IN OUT BOOLEAN           *Stop  OPTIONAL,
  IN     VOID              *Context
  );

/**
  Commandline Arguments
**/
enum {
  ARG_LOCK = 1,
  ARG_UNLOCK = 2,
  ARG_CHECK = 4,
  ARG_INTERACTIVE = 8,
};

#define PADD(x,y) (VOID *)(((CHAR8 *) x) + y)

#define IS_LOCK() ((Flags & ARG_LOCK) != 0)
#define IS_UNLOCK() ((Flags & ARG_UNLOCK) != 0)
#define IS_CHECK() ((Flags & ARG_CHECK) != 0)
#define IS_INTERACTIVE() ((Flags & ARG_INTERACTIVE) != 0)
#define BUFFER_LENGTH 128

extern UINTN Flags;

/**
  Check MsrE2 Status - original VerifyMSRE2
**/
extern
EFI_STATUS
EFIAPI
VerifyMSRE2 (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Wait for Keypress of Y or N. Ignores case.
**/
BOOLEAN
ReadYN (
  VOID
  );

/**
  Wait for any key press
**/
CHAR16
ReadAnyKey (
  VOID
  );

/**
  Parse commandline arguments
**/
UINTN
InterpretArguments (
  VOID
  );

/**
  Displays SearchString and allows to change it
**/
EFI_STRING
ModifySearchString (
  EFI_STRING SearchString
  );

/**
  Copies Package Lists to Memory
**/
EFI_HII_PACKAGE_LIST_HEADER *
HiiExportPackageLists (
  EFI_HII_HANDLE Handle
  );

/**
  Callback to Handle IFR_ONE_OF_OP
**/
VOID
HandleOneOf (
  IN     EFI_IFR_OP_HEADER   *IfrHeader,
  IN OUT BOOLEAN             *Stop  OPTIONAL,
  IN OUT VOID                *Context
  );

/**
  Displaying and Changing value of BIOS Option. Including UI
**/
VOID
HandleOneVariable (
  IN OUT ONE_OF_CONTEXT      *Context
  );

/**
  Call Handler for each occurence of opCode, starting to search at header. Called recursively
**/
EFI_IFR_OP_HEADER *
DoForEachOpCode (
  IN     EFI_IFR_OP_HEADER   *Header,
  IN     UINT8               OpCode,
  IN OUT BOOLEAN             *Stop  OPTIONAL,
  IN     VOID                *Context,
  IN     OP_CODE_HANDLER     Handler
  );
