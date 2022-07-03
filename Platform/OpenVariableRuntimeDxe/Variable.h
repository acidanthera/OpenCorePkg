/** @file
  The internal header file includes the common header files, defines
  internal structure and functions used by Variable modules.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VARIABLE_H_
#define _VARIABLE_H_

#include <PiDxe.h>
#include <Protocol/VariableWrite.h>
#include <Protocol/FaultTolerantWrite.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/Variable.h>
#include <Protocol/VariableLock.h>
#include <Protocol/VarCheck.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/AuthVariableLib.h>
#include <Library/VarCheckLib.h>
#include <Guid/GlobalVariable.h>
#include <Guid/EventGroup.h>
#include <Guid/VariableFormat.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/FaultTolerantWrite.h>
#include <Guid/VarErrorFlag.h>

#include "PrivilegePolymorphic.h"

#define NV_STORAGE_VARIABLE_BASE  (EFI_PHYSICAL_ADDRESS)\
                                   (PcdGet64 (PcdFlashNvStorageVariableBase64) != 0 ? \
                                    PcdGet64 (PcdFlashNvStorageVariableBase64) : \
                                    PcdGet32 (PcdFlashNvStorageVariableBase))

#define EFI_VARIABLE_ATTRIBUTES_MASK  (EFI_VARIABLE_NON_VOLATILE |\
                                      EFI_VARIABLE_BOOTSERVICE_ACCESS | \
                                      EFI_VARIABLE_RUNTIME_ACCESS | \
                                      EFI_VARIABLE_HARDWARE_ERROR_RECORD | \
                                      EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS | \
                                      EFI_VARIABLE_APPEND_WRITE)

///
/// The size of a 3 character ISO639 language code.
///
#define ISO_639_2_ENTRY_SIZE  3

typedef enum {
  VariableStoreTypeVolatile,
  VariableStoreTypeHob,
  VariableStoreTypeNv,
  VariableStoreTypeMax
} VARIABLE_STORE_TYPE;

typedef struct {
  UINT32                   PendingUpdateOffset;
  UINT32                   PendingUpdateLength;
  VARIABLE_STORE_HEADER    *Store;
} VARIABLE_RUNTIME_CACHE;

typedef struct {
  BOOLEAN                   *ReadLock;
  BOOLEAN                   *PendingUpdate;
  BOOLEAN                   *HobFlushComplete;
  VARIABLE_RUNTIME_CACHE    VariableRuntimeHobCache;
  VARIABLE_RUNTIME_CACHE    VariableRuntimeNvCache;
  VARIABLE_RUNTIME_CACHE    VariableRuntimeVolatileCache;
} VARIABLE_RUNTIME_CACHE_CONTEXT;

typedef struct {
  VARIABLE_HEADER    *CurrPtr;
  //
  // If both ADDED and IN_DELETED_TRANSITION variable are present,
  // InDeletedTransitionPtr will point to the IN_DELETED_TRANSITION one.
  // Otherwise, CurrPtr will point to the ADDED or IN_DELETED_TRANSITION one,
  // and InDeletedTransitionPtr will be NULL at the same time.
  //
  VARIABLE_HEADER    *InDeletedTransitionPtr;
  VARIABLE_HEADER    *EndPtr;
  VARIABLE_HEADER    *StartPtr;
  BOOLEAN            Volatile;
} VARIABLE_POINTER_TRACK;

typedef struct {
  EFI_PHYSICAL_ADDRESS              HobVariableBase;
  EFI_PHYSICAL_ADDRESS              VolatileVariableBase;
  EFI_PHYSICAL_ADDRESS              NonVolatileVariableBase;
  VARIABLE_RUNTIME_CACHE_CONTEXT    VariableRuntimeCacheContext;
  EFI_LOCK                          VariableServicesLock;
  UINT32                            ReentrantState;
  BOOLEAN                           AuthFormat;
  BOOLEAN                           AuthSupport;
  BOOLEAN                           EmuNvMode;
} VARIABLE_GLOBAL;

typedef struct {
  VARIABLE_GLOBAL                       VariableGlobal;
  UINTN                                 VolatileLastVariableOffset;
  UINTN                                 NonVolatileLastVariableOffset;
  UINTN                                 CommonVariableSpace;
  UINTN                                 CommonMaxUserVariableSpace;
  UINTN                                 CommonRuntimeVariableSpace;
  UINTN                                 CommonVariableTotalSize;
  UINTN                                 CommonUserVariableTotalSize;
  UINTN                                 HwErrVariableTotalSize;
  UINTN                                 MaxVariableSize;
  UINTN                                 MaxAuthVariableSize;
  UINTN                                 MaxVolatileVariableSize;
  UINTN                                 ScratchBufferSize;
  CHAR8                                 *PlatformLangCodes;
  CHAR8                                 *LangCodes;
  CHAR8                                 *PlatformLang;
  CHAR8                                 Lang[ISO_639_2_ENTRY_SIZE + 1];
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *FvbInstance;
} VARIABLE_MODULE_GLOBAL;

/**
  Flush the HOB variable to flash.

  @param[in] VariableName       Name of variable has been updated or deleted.
  @param[in] VendorGuid         Guid of variable has been updated or deleted.

**/
VOID
FlushHobVariableToFlash (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid
  );

/**
  Writes a buffer to variable storage space, in the working block.

  This function writes a buffer to variable storage space into a firmware
  volume block device. The destination is specified by the parameter
  VariableBase. Fault Tolerant Write protocol is used for writing.

  @param  VariableBase   Base address of the variable to write.
  @param  VariableBuffer Point to the variable data buffer.

  @retval EFI_SUCCESS    The function completed successfully.
  @retval EFI_NOT_FOUND  Fail to locate Fault Tolerant Write protocol.
  @retval EFI_ABORTED    The function could not complete successfully.

**/
EFI_STATUS
FtwVariableSpace (
  IN EFI_PHYSICAL_ADDRESS   VariableBase,
  IN VARIABLE_STORE_HEADER  *VariableBuffer
  );

/**
  Finds variable in storage blocks of volatile and non-volatile storage areas.

  This code finds variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.
  If IgnoreRtCheck is TRUE, then we ignore the EFI_VARIABLE_RUNTIME_ACCESS attribute check
  at runtime when searching existing variable, only VariableName and VendorGuid are compared.
  Otherwise, variables without EFI_VARIABLE_RUNTIME_ACCESS are not visible at runtime.

  @param[in]   VariableName           Name of the variable to be found.
  @param[in]   VendorGuid             Vendor GUID to be found.
  @param[out]  PtrTrack               VARIABLE_POINTER_TRACK structure for output,
                                      including the range searched and the target position.
  @param[in]   Global                 Pointer to VARIABLE_GLOBAL structure, including
                                      base of volatile variable storage area, base of
                                      NV variable storage area, and a lock.
  @param[in]   IgnoreRtCheck          Ignore EFI_VARIABLE_RUNTIME_ACCESS attribute
                                      check at runtime when searching variable.

  @retval EFI_INVALID_PARAMETER       If VariableName is not an empty string, while
                                      VendorGuid is NULL.
  @retval EFI_SUCCESS                 Variable successfully found.
  @retval EFI_NOT_FOUND               Variable not found

**/
EFI_STATUS
FindVariable (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  IN  VARIABLE_GLOBAL         *Global,
  IN  BOOLEAN                 IgnoreRtCheck
  );

/**
  This function is to check if the remaining variable space is enough to set
  all Variables from argument list successfully. The purpose of the check
  is to keep the consistency of the Variables to be in variable storage.

  Note: Variables are assumed to be in same storage.
  The set sequence of Variables will be same with the sequence of VariableEntry from argument list,
  so follow the argument sequence to check the Variables.

  @param[in] Attributes         Variable attributes for Variable entries.
  @param[in] Marker             VA_LIST style variable argument list.
                                The variable argument list with type VARIABLE_ENTRY_CONSISTENCY *.
                                A NULL terminates the list. The VariableSize of
                                VARIABLE_ENTRY_CONSISTENCY is the variable data size as input.
                                It will be changed to variable total size as output.

  @retval TRUE                  Have enough variable space to set the Variables successfully.
  @retval FALSE                 No enough variable space to set the Variables successfully.

**/
BOOLEAN
EFIAPI
CheckRemainingSpaceForConsistencyInternal (
  IN UINT32   Attributes,
  IN VA_LIST  Marker
  );

/**
  Update the variable region with Variable information. If EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is set,
  index of associated public key is needed.

  @param[in] VariableName       Name of variable.
  @param[in] VendorGuid         Guid of variable.
  @param[in] Data               Variable data.
  @param[in] DataSize           Size of data. 0 means delete.
  @param[in] Attributes         Attributes of the variable.
  @param[in] KeyIndex           Index of associated public key.
  @param[in] MonotonicCount     Value of associated monotonic count.
  @param[in, out] Variable      The variable information that is used to keep track of variable usage.

  @param[in] TimeStamp          Value of associated TimeStamp.

  @retval EFI_SUCCESS           The update operation is success.
  @retval EFI_OUT_OF_RESOURCES  Variable region is full, cannot write other data into this region.

**/
EFI_STATUS
UpdateVariable (
  IN      CHAR16                  *VariableName,
  IN      EFI_GUID                *VendorGuid,
  IN      VOID                    *Data,
  IN      UINTN                   DataSize,
  IN      UINT32                  Attributes OPTIONAL,
  IN      UINT32                  KeyIndex  OPTIONAL,
  IN      UINT64                  MonotonicCount  OPTIONAL,
  IN OUT  VARIABLE_POINTER_TRACK  *Variable,
  IN      EFI_TIME                *TimeStamp  OPTIONAL
  );

/**
  Return TRUE if ExitBootServices () has been called.

  @retval TRUE If ExitBootServices () has been called.
**/
BOOLEAN
AtRuntime (
  VOID
  );

/**
  Initializes a basic mutual exclusion lock.

  This function initializes a basic mutual exclusion lock to the released state
  and returns the lock.  Each lock provides mutual exclusion access at its task
  priority level.  Since there is no preemption or multiprocessor support in EFI,
  acquiring the lock only consists of raising to the locks TPL.
  If Lock is NULL, then ASSERT().
  If Priority is not a valid TPL value, then ASSERT().

  @param  Lock       A pointer to the lock data structure to initialize.
  @param  Priority   EFI TPL is associated with the lock.

  @return The lock.

**/
EFI_LOCK *
InitializeLock (
  IN OUT EFI_LOCK  *Lock,
  IN EFI_TPL       Priority
  );

/**
  Acquires lock only at boot time. Simply returns at runtime.

  This is a temperary function that will be removed when
  EfiAcquireLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiAcquireLock() at boot time, and simply returns
  at runtime.

  @param  Lock         A pointer to the lock to acquire.

**/
VOID
AcquireLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  );

/**
  Releases lock only at boot time. Simply returns at runtime.

  This is a temperary function which will be removed when
  EfiReleaseLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiReleaseLock() at boot time and simply returns
  at runtime.

  @param  Lock         A pointer to the lock to release.

**/
VOID
ReleaseLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  );

/**
  Retrieve the FVB protocol interface by HANDLE.

  @param[in]  FvBlockHandle     The handle of FVB protocol that provides services for
                                reading, writing, and erasing the target block.
  @param[out] FvBlock           The interface of FVB protocol

  @retval EFI_SUCCESS           The interface information for the specified protocol was returned.
  @retval EFI_UNSUPPORTED       The device does not support the FVB protocol.
  @retval EFI_INVALID_PARAMETER FvBlockHandle is not a valid EFI_HANDLE or FvBlock is NULL.

**/
EFI_STATUS
GetFvbByHandle (
  IN  EFI_HANDLE                          FvBlockHandle,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
  );

/**
  Function returns an array of handles that support the FVB protocol
  in a buffer allocated from pool.

  @param[out]  NumberHandles    The number of handles returned in Buffer.
  @param[out]  Buffer           A pointer to the buffer to return the requested
                                array of  handles that support FVB protocol.

  @retval EFI_SUCCESS           The array of handles was returned in Buffer, and the number of
                                handles in Buffer was returned in NumberHandles.
  @retval EFI_NOT_FOUND         No FVB handle was found.
  @retval EFI_OUT_OF_RESOURCES  There is not enough pool memory to store the matching results.
  @retval EFI_INVALID_PARAMETER NumberHandles is NULL or Buffer is NULL.

**/
EFI_STATUS
GetFvbCountAndBuffer (
  OUT UINTN       *NumberHandles,
  OUT EFI_HANDLE  **Buffer
  );

/**
  Initializes variable store area for non-volatile and volatile variable.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.

**/
EFI_STATUS
VariableCommonInitialize (
  VOID
  );

/**
  This function reclaims variable storage if free size is below the threshold.

**/
VOID
ReclaimForOS (
  VOID
  );

/**
  Get maximum variable size, covering both non-volatile and volatile variables.

  @return Maximum variable size.

**/
UINTN
GetMaxVariableSize (
  VOID
  );

/**
  Initializes variable write service.

  @retval EFI_SUCCESS          Function successfully executed.
  @retval Others               Fail to initialize the variable service.

**/
EFI_STATUS
VariableWriteServiceInitialize (
  VOID
  );

/**
  Retrieve the SMM Fault Tolerent Write protocol interface.

  @param[out] FtwProtocol       The interface of SMM Ftw protocol

  @retval EFI_SUCCESS           The SMM SAR protocol instance was found and returned in SarProtocol.
  @retval EFI_NOT_FOUND         The SMM SAR protocol instance was not found.
  @retval EFI_INVALID_PARAMETER SarProtocol is NULL.

**/
EFI_STATUS
GetFtwProtocol (
  OUT VOID  **FtwProtocol
  );

/**
  Get the proper fvb handle and/or fvb protocol by the given Flash address.

  @param[in] Address        The Flash address.
  @param[out] FvbHandle     In output, if it is not NULL, it points to the proper FVB handle.
  @param[out] FvbProtocol   In output, if it is not NULL, it points to the proper FVB protocol.

**/
EFI_STATUS
GetFvbInfoByAddress (
  IN  EFI_PHYSICAL_ADDRESS                Address,
  OUT EFI_HANDLE                          *FvbHandle OPTIONAL,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvbProtocol OPTIONAL
  );

/**

  This code finds variable in storage blocks (Volatile or Non-Volatile).

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.

  @param VariableName               Name of Variable to be found.
  @param VendorGuid                 Variable vendor GUID.
  @param Attributes                 Attribute value of the variable found.
  @param DataSize                   Size of Data found. If size is less than the
                                    data, this value contains the required size.
  @param Data                       The buffer to return the contents of the variable. May be NULL
                                    with a zero DataSize in order to determine the size buffer needed.

  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Find the specified variable.
  @return EFI_NOT_FOUND             Not found.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.

**/
EFI_STATUS
EFIAPI
VariableServiceGetVariable (
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data OPTIONAL
  );

/**

  This code Finds the Next available variable.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param VariableNameSize           The size of the VariableName buffer. The size must be large
                                    enough to fit input string supplied in VariableName buffer.
  @param VariableName               Pointer to variable name.
  @param VendorGuid                 Variable Vendor Guid.

  @retval EFI_SUCCESS               The function completed successfully.
  @retval EFI_NOT_FOUND             The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL      The VariableNameSize is too small for the result.
                                    VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER     VariableNameSize is NULL.
  @retval EFI_INVALID_PARAMETER     VariableName is NULL.
  @retval EFI_INVALID_PARAMETER     VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER     The input values of VariableName and VendorGuid are not a name and
                                    GUID of an existing variable.
  @retval EFI_INVALID_PARAMETER     Null-terminator is not found in the first VariableNameSize bytes of
                                    the input VariableName buffer.

**/
EFI_STATUS
EFIAPI
VariableServiceGetNextVariableName (
  IN OUT  UINTN     *VariableNameSize,
  IN OUT  CHAR16    *VariableName,
  IN OUT  EFI_GUID  *VendorGuid
  );

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.
  This function will check attribute carefully to avoid authentication bypass.

  @param VariableName                     Name of Variable to be found.
  @param VendorGuid                       Variable vendor GUID.
  @param Attributes                       Attribute value of the variable found
  @param DataSize                         Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param Data                             Data pointer.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SUCCESS                     Set successfully.
  @return EFI_OUT_OF_RESOURCES            Resource not enough to set variable.
  @return EFI_NOT_FOUND                   Not found.
  @return EFI_WRITE_PROTECTED             Variable is read-only.

**/
EFI_STATUS
EFIAPI
VariableServiceSetVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  );

/**

  This code returns information about the EFI variables.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param Attributes                     Attributes bitmask to specify the type of variables
                                        on which to return information.
  @param MaximumVariableStorageSize     Pointer to the maximum size of the storage space available
                                        for the EFI variables associated with the attributes specified.
  @param RemainingVariableStorageSize   Pointer to the remaining size of the storage space available
                                        for EFI variables associated with the attributes specified.
  @param MaximumVariableSize            Pointer to the maximum size of an individual EFI variables
                                        associated with the attributes specified.

  @return EFI_SUCCESS                   Query successfully.

**/
EFI_STATUS
EFIAPI
VariableServiceQueryVariableInfoInternal (
  IN  UINT32  Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  );

/**

  This code returns information about the EFI variables.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param Attributes                     Attributes bitmask to specify the type of variables
                                        on which to return information.
  @param MaximumVariableStorageSize     Pointer to the maximum size of the storage space available
                                        for the EFI variables associated with the attributes specified.
  @param RemainingVariableStorageSize   Pointer to the remaining size of the storage space available
                                        for EFI variables associated with the attributes specified.
  @param MaximumVariableSize            Pointer to the maximum size of an individual EFI variables
                                        associated with the attributes specified.

  @return EFI_INVALID_PARAMETER         An invalid combination of attribute bits was supplied.
  @return EFI_SUCCESS                   Query successfully.
  @return EFI_UNSUPPORTED               The attribute is not supported on this platform.

**/
EFI_STATUS
EFIAPI
VariableServiceQueryVariableInfo (
  IN  UINT32  Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  );

/**
  Mark a variable that will become read-only after leaving the DXE phase of execution.

  @param[in] This          The VARIABLE_LOCK_PROTOCOL instance.
  @param[in] VariableName  A pointer to the variable name that will be made read-only subsequently.
  @param[in] VendorGuid    A pointer to the vendor GUID that will be made read-only subsequently.

  @retval EFI_SUCCESS           The variable specified by the VariableName and the VendorGuid was marked
                                as pending to be read-only.
  @retval EFI_INVALID_PARAMETER VariableName or VendorGuid is NULL.
                                Or VariableName is an empty string.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource to hold the lock request.
**/
EFI_STATUS
EFIAPI
VariableLockRequestToLock (
  IN CONST EDKII_VARIABLE_LOCK_PROTOCOL  *This,
  IN       CHAR16                        *VariableName,
  IN       EFI_GUID                      *VendorGuid
  );

/**
  Register SetVariable check handler.

  @param[in] Handler            Pointer to check handler.

  @retval EFI_SUCCESS           The SetVariable check handler was registered successfully.
  @retval EFI_INVALID_PARAMETER Handler is NULL.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the SetVariable check handler register request.
  @retval EFI_UNSUPPORTED       This interface is not implemented.
                                For example, it is unsupported in VarCheck protocol if both VarCheck and SmmVarCheck protocols are present.

**/
EFI_STATUS
EFIAPI
VarCheckRegisterSetVariableCheckHandler (
  IN VAR_CHECK_SET_VARIABLE_CHECK_HANDLER  Handler
  );

/**
  Variable property set.

  @param[in] Name               Pointer to the variable name.
  @param[in] Guid               Pointer to the vendor GUID.
  @param[in] VariableProperty   Pointer to the input variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was set successfully.
  @retval EFI_INVALID_PARAMETER Name, Guid or VariableProperty is NULL, or Name is an empty string,
                                or the fields of VariableProperty are not valid.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the variable property set request.

**/
EFI_STATUS
EFIAPI
VarCheckVariablePropertySet (
  IN CHAR16                       *Name,
  IN EFI_GUID                     *Guid,
  IN VAR_CHECK_VARIABLE_PROPERTY  *VariableProperty
  );

/**
  Variable property get.

  @param[in]  Name              Pointer to the variable name.
  @param[in]  Guid              Pointer to the vendor GUID.
  @param[out] VariableProperty  Pointer to the output variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was got successfully.
  @retval EFI_INVALID_PARAMETER Name, Guid or VariableProperty is NULL, or Name is an empty string.
  @retval EFI_NOT_FOUND         The property of variable specified by the Name and Guid was not found.

**/
EFI_STATUS
EFIAPI
VarCheckVariablePropertyGet (
  IN CHAR16                        *Name,
  IN EFI_GUID                      *Guid,
  OUT VAR_CHECK_VARIABLE_PROPERTY  *VariableProperty
  );

/**
  Initialize variable quota.

**/
VOID
InitializeVariableQuota (
  VOID
  );

extern VARIABLE_MODULE_GLOBAL      *mVariableModuleGlobal;
extern EFI_FIRMWARE_VOLUME_HEADER  *mNvFvHeaderCache;
extern VARIABLE_STORE_HEADER       *mNvVariableCache;
extern VARIABLE_INFO_ENTRY         *gVariableInfo;
extern BOOLEAN                     mEndOfDxe;
extern VAR_CHECK_REQUEST_SOURCE    mRequestSource;

extern AUTH_VAR_LIB_CONTEXT_OUT  mAuthContextOut;

/**
  Finds variable in storage blocks of volatile and non-volatile storage areas.

  This code finds variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.

  @param[in]  VariableName          Name of the variable to be found.
  @param[in]  VendorGuid            Variable vendor GUID to be found.
  @param[out] AuthVariableInfo      Pointer to AUTH_VARIABLE_INFO structure for
                                    output of the variable found.

  @retval EFI_INVALID_PARAMETER     If VariableName is not an empty string,
                                    while VendorGuid is NULL.
  @retval EFI_SUCCESS               Variable successfully found.
  @retval EFI_NOT_FOUND             Variable not found

**/
EFI_STATUS
EFIAPI
VariableExLibFindVariable (
  IN  CHAR16              *VariableName,
  IN  EFI_GUID            *VendorGuid,
  OUT AUTH_VARIABLE_INFO  *AuthVariableInfo
  );

/**
  Finds next variable in storage blocks of volatile and non-volatile storage areas.

  This code finds next variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.

  @param[in]  VariableName          Name of the variable to be found.
  @param[in]  VendorGuid            Variable vendor GUID to be found.
  @param[out] AuthVariableInfo      Pointer to AUTH_VARIABLE_INFO structure for
                                    output of the next variable.

  @retval EFI_INVALID_PARAMETER     If VariableName is not an empty string,
                                    while VendorGuid is NULL.
  @retval EFI_SUCCESS               Variable successfully found.
  @retval EFI_NOT_FOUND             Variable not found

**/
EFI_STATUS
EFIAPI
VariableExLibFindNextVariable (
  IN  CHAR16              *VariableName,
  IN  EFI_GUID            *VendorGuid,
  OUT AUTH_VARIABLE_INFO  *AuthVariableInfo
  );

/**
  Update the variable region with Variable information.

  @param[in] AuthVariableInfo       Pointer AUTH_VARIABLE_INFO structure for
                                    input of the variable.

  @retval EFI_SUCCESS               The update operation is success.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
  @retval EFI_WRITE_PROTECTED       Variable is write-protected.
  @retval EFI_OUT_OF_RESOURCES      There is not enough resource.

**/
EFI_STATUS
EFIAPI
VariableExLibUpdateVariable (
  IN AUTH_VARIABLE_INFO  *AuthVariableInfo
  );

/**
  Get scratch buffer.

  @param[in, out] ScratchBufferSize Scratch buffer size. If input size is greater than
                                    the maximum supported buffer size, this value contains
                                    the maximum supported buffer size as output.
  @param[out]     ScratchBuffer     Pointer to scratch buffer address.

  @retval EFI_SUCCESS       Get scratch buffer successfully.
  @retval EFI_UNSUPPORTED   If input size is greater than the maximum supported buffer size.

**/
EFI_STATUS
EFIAPI
VariableExLibGetScratchBuffer (
  IN OUT UINTN  *ScratchBufferSize,
  OUT    VOID   **ScratchBuffer
  );

/**
  This function is to check if the remaining variable space is enough to set
  all Variables from argument list successfully. The purpose of the check
  is to keep the consistency of the Variables to be in variable storage.

  Note: Variables are assumed to be in same storage.
  The set sequence of Variables will be same with the sequence of VariableEntry from argument list,
  so follow the argument sequence to check the Variables.

  @param[in] Attributes         Variable attributes for Variable entries.
  @param ...                    The variable argument list with type VARIABLE_ENTRY_CONSISTENCY *.
                                A NULL terminates the list. The VariableSize of
                                VARIABLE_ENTRY_CONSISTENCY is the variable data size as input.
                                It will be changed to variable total size as output.

  @retval TRUE                  Have enough variable space to set the Variables successfully.
  @retval FALSE                 No enough variable space to set the Variables successfully.

**/
BOOLEAN
EFIAPI
VariableExLibCheckRemainingSpaceForConsistency (
  IN UINT32  Attributes,
  ...
  );

/**
  Return TRUE if at OS runtime.

  @retval TRUE If at OS runtime.
  @retval FALSE If at boot time.

**/
BOOLEAN
EFIAPI
VariableExLibAtRuntime (
  VOID
  );

#endif
