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

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
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
#include <UDK/MdePkg/Include/Guid/HiiPlatformSetupFormset.h>

#define DONT_STOP_AT 0xFFFF

/*
 Parameters to serch in a Form for
 ifrHeaders which contain a Description
 and to return the Varstore
 */
typedef struct {
    EFI_STRING                   searchText;    // Text that must be part of the options description
    EFI_HII_HANDLE               efiHandle;     // Handle of the list the Form is part of
    EFI_HII_PACKAGE_LIST_HEADER *listHeader;    // Ptr to the contents of the handle
    EFI_HII_PACKAGE_HEADER      *pkgHeader;     // Ptr to the current package in the list
    EFI_IFR_OP_HEADER           *firstIfrHeader; // Ptr to first IfrHeader in Package
    EFI_IFR_VARSTORE            *ifrVarStore;   // Ptr to ifrHeader of corresponding varstore in same list
    EFI_IFR_ONE_OF              *ifrOneOf;      // Ptr to ifrHeader of BIOS Option
    UINT16                       stopAt;        // Used to find a certain BIOS Option
    UINT16                       count;         // Running number of suitable BIOS Options
    
} OneOfContext;

/*
 Callback - What to do, when a ifrHeader with a defined OP_CODE is found
 ifrHeader   Current ifrHeader under scrutiny
 stop        Ptr to Stop flag. If true no further search for opcodes. Can be NULL.
 context     Ptr to Hanlder specific data
 */
typedef VOID EFIAPI OpCodeHandler (EFI_IFR_OP_HEADER* ifrHeader, UINT8* stop, VOID* context);

/*
 Commandline Arguments
 */
enum {
    ARG_LOCK = 1,
    ARG_UNLOCK = 2,
    ARG_CHECK = 4,
    ARG_INTERACTIVE = 8,
    ARG_VERBOSE = 16
};

#define PADD(x,y) (void*)(((char*) x) + y)

#define IS_VERBOSE() (mFlags & ARG_VERBOSE)
#define IS_LOCK() (mFlags & ARG_LOCK)
#define IS_UNLOCK() (mFlags & ARG_UNLOCK)
#define IS_CHECK() (mFlags & ARG_CHECK)
#define IS_INTERACTIVE() (mFlags & ARG_INTERACTIVE)
#define BUFFER_LENGTH 128

extern UINTN mFlags;

// Check MsrE2 Status - original VerifyMSRE2
extern EFI_STATUS EFIAPI VerifyMSRE2 (
                                      IN EFI_HANDLE         ImageHandle,
                                      IN EFI_SYSTEM_TABLE  *SystemTable
                                      );


// Wait for Keypress of Y or N. Ignores case.
extern UINT32 ReadYN ();
// Wait for any key press
extern CHAR16 ReadAnyKey ();

// Parse commandline arguments
extern UINTN InterpretArguments ();
extern VOID  PrintUINT8Str (IN UINT8 n[1]);
extern VOID  PrintGuid (IN EFI_GUID* Guid);
// Displays SearchString and allows to change it
extern EFI_STRING  ModifySearchString (EFI_STRING SearchString);

// Copies Package Lists to Memory
extern EFI_HII_PACKAGE_LIST_HEADER* HiiExportPackageLists (EFI_HII_HANDLE h);
// Callback to Handle IFR_ONE_OF_OP
extern VOID HandleOneOf (
                         IN EFI_IFR_OP_HEADER* ifrHeader,
                         IN UINT8* stop,
                         IN VOID* context
                         );
// Displaying and Changing value of BIOS Option. Including UI
extern VOID HandleOneVariable (IN OneOfContext* context);
// Call Handler for each occurence of opCode, starting to search at header. Called recursively
extern EFI_IFR_OP_HEADER* DoForEachOpCode (
                                           IN EFI_IFR_OP_HEADER* header,
                                           IN UINT8 opCode,
                                           IN UINT8* stop,
                                           IN void* context,
                                           IN OpCodeHandler handler
                                           );
