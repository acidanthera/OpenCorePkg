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

//
// Unless otherwise specified all data types are naturally aligned. Structures are
// aligned on boundaries equal to the largest internal datum of the structure and
// internal data are implicitly padded to achieve natural alignment.
//

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>

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

#define CHAR_ESC     0x1B
#define DONT_STOP_AT 0xFFFF

/**
  Parameters to search in a Form for
  IfrHeaders which contain a Description
  and to return the VarStore

  @param SearchText        Text that must be part of the options
                           description
  @param EfiHandle         Handle of the list the Form is part of
  @param ListHeader        Pointer to the contents of the handle
  @param PkgHeader         Pointer to the current package in the
                           list
  @param FirstIfrHeader    Pointer to first IfrHeader in Package
  @param IfrVarStore       Pointer to IfrHeader of corresponding
                           varstore in same list
  @param IfrOneOf          Pointer to IfrHeader of BIOS Option
  @param StopAt            Find a certain BIOS Option
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
  Callback - What to do, when a IfrHeader with a defined
  OP_CODE is found

  @param[in]     IfrHeader   Current IfrHeader under scrutiny
  @param[in,out] Stop        Pointer to Stop flag. If TRUE no
                             further search for opcodes. Can
                             be NULL.
  @param[in]     Context     Pointer to Handler specific data
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
  ARG_VERIFY      = 0,
  ARG_LOCK        = 1,
  ARG_UNLOCK      = 2,
  ARG_INTERACTIVE = 8,
};

#define PADD(x,y) (VOID *)(((CHAR8 *) x) + y)

#define BUFFER_LENGTH 128

extern UINTN mArgumentFlags;

/**
  Check MsrE2 Status - original VerifyMSRE2

  @retval EFI_SUCCESS    Success
  @retval Other          Fail to verify MSR 0xE2 status
**/
EFI_STATUS
EFIAPI
VerifyMSRE2 (
  VOID
  );

/**
  Wait for Keypress of Y or N. Ignores case.

  @retval TRUE     Keypress is Y
  @retval FALSE    Keypress is N
**/
BOOLEAN
ReadYN (
  VOID
  );

/**
  Wait for any key press

  @retval Other    Unicode for pressed key
**/
CHAR16
ReadAnyKey (
  VOID
  );

/**
  Parse commandline arguments

  @retval EFI_SUCCESS               Success
  @retval EFI_OUT_OF_RESOURCES      Could not allocate memory
  @retval EFI_INVALID_PARAMETER     More than 1 parameter provided
**/
EFI_STATUS
InterpretArguments (
  VOID
  );

/**
  Displays SearchString and allows to change it

  @param[in,out] SearchString    Current configuration string
**/
VOID
ModifySearchString (
  IN OUT EFI_STRING *SearchString
  );

/**
  Copies Package Lists to Memory

  @param[in]     Handle     An EFI_HII_HANDLE that corresponds
                            to the desired package list in the
                            HIIdatabase.

**/
EFI_HII_PACKAGE_LIST_HEADER *
HiiExportPackageLists (
  IN     EFI_HII_HANDLE Handle
  );

/**
  Callback to Handle EFI_IFR_ONE_OF_OP

  @param[in]     IfrHeader   Current IfrHeader under scrutiny
  @param[in,out] Stop        Pointer to Stop flag. If TRUE no
                             further search for opcodes. Can
                             be NULL.
  @param[in]     Context     Pointer to Handler specific data
**/
VOID
HandleIfrOption (
  IN     EFI_IFR_OP_HEADER   *IfrHeader,
  IN OUT BOOLEAN             *Stop  OPTIONAL,
  IN OUT VOID                *Context
  );

/**
  Displaying and Changing value of BIOS Option. Including UI

  @param[in]     Context     Pointer to Handler specific data
**/
VOID
HandleIfrVariable (
  IN OUT ONE_OF_CONTEXT      *Context
  );

/**
  Call Handler for each occurence of opCode, starting to search
  at header. Called recursively

  @param[in]     Header      Current Header under scrutiny
  @param[in]     OpCode      Type for OpCode
  @param[in,out] Stop        Pointer to Stop flag. If TRUE no
                             further search for opcodes. Can
                             be NULL.
  @param[in]     Context     Pointer to Handler specific data
  @param[in]     Handler     Handler for data
**/
EFI_IFR_OP_HEADER *
IterateOpCode (
  IN     EFI_IFR_OP_HEADER   *Header,
  IN     UINT8               OpCode,
  IN OUT BOOLEAN             *Stop  OPTIONAL,
  IN     VOID                *Context,
  IN     OP_CODE_HANDLER     Handler
  );
