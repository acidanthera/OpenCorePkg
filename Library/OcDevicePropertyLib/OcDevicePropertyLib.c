/** @file
  Copyright (C) 2017, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <AppleMacEfi.h>

#include <Guid/AppleVariable.h>

#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathPropertyDatabase.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMiscLib.h>

#define DEVICE_PATH_PROPERTY_DATA_SIGNATURE  \
  SIGNATURE_32 ('D', 'p', 'p', 'P')

// PROPERTY_DATABASE_FROM_PROTOCOL
#define PROPERTY_DATABASE_FROM_PROTOCOL(This) \
  CR (                                        \
    This,                                     \
    DEVICE_PATH_PROPERTY_DATA,                \
    Protocol,                                 \
    DEVICE_PATH_PROPERTY_DATA_SIGNATURE       \
    )

// DEVICE_PATH_PROPERTY_DATABASE
typedef struct {
  UINTN                                      Signature;
  LIST_ENTRY                                 Nodes;
  EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL Protocol;
  BOOLEAN                                    Modified;
} DEVICE_PATH_PROPERTY_DATA;

#define APPLE_PATH_PROPERTIES_VARIABLE_NAME    L"AAPL,PathProperties"
#define APPLE_PATH_PROPERTY_VARIABLE_MAX_SIZE  768
#define APPLE_PATH_PROPERTY_VARIABLE_MAX_NUM   0x10000

#define EFI_DEVICE_PATH_PROPERTY_NODE_SIGNATURE  \
  SIGNATURE_32 ('D', 'p', 'n', '\0')

#define PROPERTY_NODE_FROM_LIST_ENTRY(Entry)   \
  ((EFI_DEVICE_PATH_PROPERTY_NODE *)(          \
    CR (                                       \
      Entry,                                   \
      EFI_DEVICE_PATH_PROPERTY_NODE_HDR,       \
      Link,                                    \
      EFI_DEVICE_PATH_PROPERTY_NODE_SIGNATURE  \
      )                                        \
    ))

#define EFI_DEVICE_PATH_PROPERTY_NODE_SIZE(Node)  \
  (sizeof (EFI_DEVICE_PATH_PROPERTY_BUFFER_NODE_HDR) + GetDevicePathSize (&(Node)->DevicePath))

// EFI_DEVICE_PATH_PROPERTY_NODE_HDR
typedef struct {
  UINTN      Signature;           ///<
  LIST_ENTRY Link;                ///<
  UINTN      NumberOfProperties;  ///<
  LIST_ENTRY Properties;          ///<
} EFI_DEVICE_PATH_PROPERTY_NODE_HDR;

// DEVICE_PATH_PROPERTY_NODE
typedef struct {
  EFI_DEVICE_PATH_PROPERTY_NODE_HDR Hdr;         ///<
  EFI_DEVICE_PATH_PROTOCOL          DevicePath;  ///<
} EFI_DEVICE_PATH_PROPERTY_NODE;

#define EFI_DEVICE_PATH_PROPERTY_SIGNATURE  \
  SIGNATURE_32 ('D', 'p', 'p', '\0')

#define EFI_DEVICE_PATH_PROPERTY_DATABASE_VERSION 1

#define EFI_DEVICE_PATH_PROPERTY_FROM_LIST_ENTRY(Entry)  \
  (CR (                                                   \
    (Entry),                                             \
    EFI_DEVICE_PATH_PROPERTY,                            \
    Link,                                                \
    EFI_DEVICE_PATH_PROPERTY_SIGNATURE                   \
    ))

#define EFI_DEVICE_PATH_PROPERTY_SIZE(Property)  \
  ((Property)->Name->Size + (Property)->Value->Size)

#define EFI_DEVICE_PATH_PROPERTY_VALUE_SIZE(Property)  \
  ((Property)->Value->Size - sizeof (*(Property)->Value))

#define NEXT_EFI_DEVICE_PATH_PROPERTY(Property)                   \
  (EFI_DEVICE_PATH_PROPERTY *)(                                   \
    (UINTN)(Property) + EFI_DEVICE_PATH_PROPERTY_SIZE (Property)  \
    )

// EFI_DEVICE_PATH_PROPERTY
typedef struct {
  UINTN                         Signature;  ///<
  LIST_ENTRY                    Link;       ///<
  EFI_DEVICE_PATH_PROPERTY_DATA *Name;      ///<
  EFI_DEVICE_PATH_PROPERTY_DATA *Value;     ///<
} EFI_DEVICE_PATH_PROPERTY;

// TODO: Move to own header
// C649D4F3-D502-4DAA-A139-394ACCF2A63B
// This protocol describes every thunderbolt device:
// - Thunderbolt Inter-domain Boundary
// - Thunderbolt Device
// TODO: explore this. Need to RE Vendor/Apple/ThunderboltPkg drivers, ThunderboltNhi in particular.
#define APPLE_THUNDERBOLT_NATIVE_HOST_INTERFACE_PROTOCOL_GUID \
  { 0xC649D4F3, 0xD502, 0x4DAA,                               \
    { 0xA1, 0x39, 0x39, 0x4A, 0xCC, 0xF2, 0xA6, 0x3B } }

EFI_GUID mAppleThunderboltNativeHostInterfaceProtocolGuid = APPLE_THUNDERBOLT_NATIVE_HOST_INTERFACE_PROTOCOL_GUID;

// InternalGetPropertyNode
STATIC
EFI_DEVICE_PATH_PROPERTY_NODE *
InternalGetPropertyNode (
  IN DEVICE_PATH_PROPERTY_DATA  *DevicePathPropertyData,
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
{
  LIST_ENTRY  *Node;
  UINTN       DevicePathSize;
  UINTN       DevicePathSize2;

  Node = GetFirstNode (&DevicePathPropertyData->Nodes);
  DevicePathSize = GetDevicePathSize (DevicePath);

  while (!IsNull (&DevicePathPropertyData->Nodes, Node)) {
    DevicePathSize2 = GetDevicePathSize (&PROPERTY_NODE_FROM_LIST_ENTRY (Node)->DevicePath);

    if (DevicePathSize == DevicePathSize2
      && CompareMem (DevicePath, &PROPERTY_NODE_FROM_LIST_ENTRY (Node)->DevicePath, DevicePathSize) == 0) {
      return PROPERTY_NODE_FROM_LIST_ENTRY (Node);
    }

    Node = GetNextNode (&DevicePathPropertyData->Nodes, Node);
  }

  return NULL;
}

// InternalGetProperty
STATIC
EFI_DEVICE_PATH_PROPERTY *
InternalGetProperty (
  IN EFI_DEVICE_PATH_PROPERTY_NODE  *Node,
  IN CONST CHAR16                   *Name
  )
{
  LIST_ENTRY  *Property;


  Property = GetFirstNode (&Node->Hdr.Properties);

  while (!IsNull (&Node->Hdr.Properties, Property)) {
    if (StrCmp (Name, (CONST CHAR16 *) &EFI_DEVICE_PATH_PROPERTY_FROM_LIST_ENTRY (Property)->Name->Data[0]) == 0) {
      return EFI_DEVICE_PATH_PROPERTY_FROM_LIST_ENTRY (Property);
    }

    Property = GetNextNode (&Node->Hdr.Properties, Property);
  }

  return NULL;
}

// InternalSyncWithThunderboltDevices
STATIC
VOID
InternalSyncWithThunderboltDevices (
  VOID
  )
{
  EFI_STATUS Status;
  UINTN      NumberHandles;
  EFI_HANDLE *Buffer;
  UINTN      Index;
  VOID       *Interface;

  Buffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &mAppleThunderboltNativeHostInterfaceProtocolGuid,
                  NULL,
                  &NumberHandles,
                  &Buffer
                  );

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < NumberHandles; ++Index) {
      Status = gBS->HandleProtocol (
                      Buffer[Index],
                      &mAppleThunderboltNativeHostInterfaceProtocolGuid,
                      &Interface
                      );

      if (!EFI_ERROR (Status)) {
        if (*(UINT32 *)((UINTN)Interface + sizeof (UINT32)) == 0) {
          (*(VOID (EFIAPI **)(VOID *))((UINTN)Interface + 232)) (Interface);
        }
      }
    }

    FreePool (Buffer);
  }
}

// DppDbGetProperty
/** Locates a device property in the database and returns its value into Value.

  @param[in]      This        A pointer to the protocol instance.
  @param[in]      DevicePath  The device path of the device to get the property
                              of.
  @param[in]      Name        The Name of the requested property.
  @param[out]     Value       The Buffer allocated by the caller to return the
                              value of the property into.
  @param[in,out]  Size        On input the size of the allocated Value Buffer.
                              On output the size required to fill the Buffer.

  @return                       The status of the operation is returned.
  @retval EFI_BUFFER_TOO_SMALL  The memory required to return the value exceeds
                                the size of the allocated Buffer.
                                The required size to complete the operation has
                                been returned into Size.
  @retval EFI_NOT_FOUND         The given device path does not have a property
                                with the specified Name.
  @retval EFI_SUCCESS           The operation completed successfully and the
                                Value Buffer has been filled.
**/
EFI_STATUS
EFIAPI
DppDbGetProperty (
  IN     EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL  *This,
  IN     EFI_DEVICE_PATH_PROTOCOL                    *DevicePath,
  IN     CONST CHAR16                                *Name,
  OUT    VOID                                        *Value OPTIONAL,
  IN OUT UINTN                                       *Size
  )
{
  DEVICE_PATH_PROPERTY_DATA         *Database;
  EFI_DEVICE_PATH_PROPERTY_NODE     *Node;
  EFI_DEVICE_PATH_PROPERTY          *Property;
  UINTN                             PropertySize;
  BOOLEAN                           BufferTooSmall;

  Database = PROPERTY_DATABASE_FROM_PROTOCOL (This);
  Node     = InternalGetPropertyNode (Database, DevicePath);
  if (Node == NULL) {
    return EFI_NOT_FOUND;
  }

  Property = InternalGetProperty (Node, Name);
  if (Property == NULL) {
    return EFI_NOT_FOUND;
  }

  PropertySize   = EFI_DEVICE_PATH_PROPERTY_VALUE_SIZE (Property);
  BufferTooSmall = PropertySize > *Size;
  *Size          = PropertySize;

  if (!BufferTooSmall) {
    CopyMem (Value, &Property->Value->Data[0], PropertySize);
    return EFI_SUCCESS;
  }

  return EFI_BUFFER_TOO_SMALL;
}

// DppDbSetProperty
/** Sets the sepcified property of the given device path to the provided Value.

  @param[in] This        A pointer to the protocol instance.
  @param[in] DevicePath  The device path of the device to set the property of.
  @param[in] Name        The Name of the desired property.
  @param[in] Value       The Buffer holding the value to set the property to.
  @param[in] Size        The size of the Value Buffer.

  @return                       The status of the operation is returned.
  @retval EFI_OUT_OF_RESOURCES  The memory necessary to complete the operation
                                could not be allocated.
  @retval EFI_SUCCESS           The operation completed successfully and the
                                Value Buffer has been filled.
**/
EFI_STATUS
EFIAPI
DppDbSetProperty (
  IN EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL                    *DevicePath,
  IN CONST CHAR16                                *Name,
  IN VOID                                        *Value,
  IN UINTN                                       Size
  )
{
  DEVICE_PATH_PROPERTY_DATA     *Database;
  EFI_DEVICE_PATH_PROPERTY_NODE *Node;
  UINTN                         DevicePathSize;
  EFI_DEVICE_PATH_PROPERTY      *Property;
  UINTN                         PropertyNameSize;
  UINTN                         PropertyValueSize;
  EFI_DEVICE_PATH_PROPERTY_DATA *PropertyName;
  EFI_DEVICE_PATH_PROPERTY_DATA *PropertyValue;

  Database = PROPERTY_DATABASE_FROM_PROTOCOL (This);
  Node     = InternalGetPropertyNode (Database, DevicePath);

  if (Node == NULL) {
    DevicePathSize = GetDevicePathSize (DevicePath);
    Node           = AllocateZeroPool (sizeof (*Node) + DevicePathSize);

    if (Node == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Node->Hdr.Signature = EFI_DEVICE_PATH_PROPERTY_NODE_SIGNATURE;

    InitializeListHead (&Node->Hdr.Properties);

    CopyMem (
      &Node->DevicePath,
      DevicePath,
      DevicePathSize
      );

    InsertTailList (&Database->Nodes, &Node->Hdr.Link);

    Database->Modified = TRUE;
  }

  Property = InternalGetProperty (Node, Name);

  if (Property != NULL) {
    if (Property->Value->Size == Size + sizeof (UINT32)
      && CompareMem (&Property->Value->Data[0], Value, Size) == 0) {
      return EFI_SUCCESS;
    }

    RemoveEntryList (&Property->Link);

    --Node->Hdr.NumberOfProperties;

    FreePool (Property->Name);
    FreePool (Property->Value);
    FreePool (Property);
  }

  Database->Modified = TRUE;
  Property           = AllocateZeroPool (sizeof (*Property));
  
  if (Property == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PropertyNameSize   = sizeof (*PropertyName) + StrSize (Name);
  PropertyName       = AllocateZeroPool (PropertyNameSize);
  Property->Name     = PropertyName;
  if (PropertyName == NULL) {
    FreePool (Property);
    return EFI_OUT_OF_RESOURCES;
  }

  PropertyValueSize = sizeof (*PropertyValue) + Size;
  PropertyValue     = AllocateZeroPool (PropertyValueSize);
  Property->Value   = PropertyValue;

  if (PropertyValue == NULL) {
    FreePool (PropertyName);
    FreePool (Property);
    return EFI_OUT_OF_RESOURCES;
  }
  
  Property->Signature = EFI_DEVICE_PATH_PROPERTY_SIGNATURE;

  CopyMem (&Property->Name->Data[0], Name, PropertyNameSize - sizeof (*PropertyName));
  Property->Name->Size = (UINT32) PropertyNameSize;

  CopyMem (&Property->Value->Data[0], Value, Size);
  Property->Value->Size = (UINT32) PropertyValueSize;

  InsertTailList (&Node->Hdr.Properties, &Property->Link);

  ++Node->Hdr.NumberOfProperties;

  return EFI_SUCCESS;
}

// DppDbRemoveProperty
/** Removes the sepcified property from the given device path.

  @param[in] This        A pointer to the protocol instance.
  @param[in] DevicePath  The device path of the device to set the property of.
  @param[in] Name        The Name of the desired property.

  @return                The status of the operation is returned.
  @retval EFI_NOT_FOUND  The given device path does not have a property with
                         the specified Name.
  @retval EFI_SUCCESS    The operation completed successfully.
**/
EFI_STATUS
EFIAPI
DppDbRemoveProperty (
  IN EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL                    *DevicePath,
  IN CONST CHAR16                                *Name
  )
{
  DEVICE_PATH_PROPERTY_DATA     *DevicePathPropertyData;
  EFI_DEVICE_PATH_PROPERTY_NODE *Node;
  EFI_DEVICE_PATH_PROPERTY      *Property;

  DevicePathPropertyData = PROPERTY_DATABASE_FROM_PROTOCOL (This);
  Node = InternalGetPropertyNode (DevicePathPropertyData, DevicePath);
  if (Node == NULL) {
    return EFI_NOT_FOUND;
  }

  Property = InternalGetProperty (Node, Name);
  if (Property == NULL) {
    return EFI_NOT_FOUND;
  }

  DevicePathPropertyData->Modified = TRUE;

  RemoveEntryList (&Property->Link);

  --Node->Hdr.NumberOfProperties;

  FreePool (Property->Name);
  FreePool (Property->Value);
  FreePool (Property);

  if (Node->Hdr.NumberOfProperties == 0) {
    RemoveEntryList (&Node->Hdr.Link);

    FreePool (Node);
  }

  return EFI_SUCCESS;
}

// DppDbGetPropertyBuffer
/** Returns a Buffer of all device properties into Buffer.

  @param[in]      This    A pointer to the protocol instance.
  @param[out]     Buffer  The Buffer allocated by the caller to return the
                          property Buffer into.
  @param[in,out]  Size    On input the size of the allocated Buffer.
                          On output the size required to fill the Buffer.

  @return                       The status of the operation is returned.
  @retval EFI_BUFFER_TOO_SMALL  The memory required to return the value exceeds
                                the size of the allocated Buffer.
                                The required size to complete the operation has
                                been returned into Size.
  @retval EFI_SUCCESS           The operation completed successfully.
**/
EFI_STATUS
EFIAPI
DppDbGetPropertyBuffer (
  IN     EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL  *This,
  OUT    EFI_DEVICE_PATH_PROPERTY_BUFFER             *Buffer OPTIONAL,
  IN OUT UINTN                                       *Size
  )
{
  LIST_ENTRY                           *Nodes;
  LIST_ENTRY                           *NodeWalker;
  UINTN                                BufferSize;
  LIST_ENTRY                           *Property;
  UINT32                               NumberOfNodes;
  EFI_DEVICE_PATH_PROPERTY_BUFFER_NODE *BufferNode;
  UINT8                                *BufferPtr;
  BOOLEAN                              BufferTooSmall;

  Nodes  = &(PROPERTY_DATABASE_FROM_PROTOCOL (This))->Nodes;

  if (IsListEmpty (Nodes)) {
    *Size  = 0;
    return EFI_SUCCESS;
  }

  if (PcdGetBool (PcdEnableAppleThunderboltSync)) {
    InternalSyncWithThunderboltDevices ();
  }

  NodeWalker    = GetFirstNode (Nodes);
  BufferSize    = sizeof (*Buffer);
  NumberOfNodes = 0;

  while (!IsNull (Nodes, NodeWalker)) {
    Property = GetFirstNode (&PROPERTY_NODE_FROM_LIST_ENTRY (NodeWalker)->Hdr.Properties);

    while (!IsNull (&PROPERTY_NODE_FROM_LIST_ENTRY (NodeWalker)->Hdr.Properties, Property)) {
      BufferSize += EFI_DEVICE_PATH_PROPERTY_SIZE (EFI_DEVICE_PATH_PROPERTY_FROM_LIST_ENTRY (Property));
      Property = GetNextNode (&PROPERTY_NODE_FROM_LIST_ENTRY (NodeWalker)->Hdr.Properties, Property);
    }

    BufferSize += EFI_DEVICE_PATH_PROPERTY_NODE_SIZE (PROPERTY_NODE_FROM_LIST_ENTRY (NodeWalker));

    NodeWalker = GetNextNode (Nodes, NodeWalker);

    ++NumberOfNodes;
  }

  DEBUG ((DEBUG_VERBOSE, "Saving to %p, given %u, requested %u\n", Buffer, (UINT32) *Size, (UINT32) BufferSize));

  BufferTooSmall = *Size < BufferSize;
  *Size  = BufferSize;
  if (BufferTooSmall) {
    return EFI_BUFFER_TOO_SMALL;
  }

  Buffer->Size          = (UINT32) BufferSize;
  Buffer->Version       = EFI_DEVICE_PATH_PROPERTY_DATABASE_VERSION;
  Buffer->NumberOfNodes = NumberOfNodes;

  NodeWalker = GetFirstNode (Nodes);

  BufferNode = &Buffer->Nodes[0];

  while (!IsNull (Nodes, NodeWalker)) {
    BufferSize = GetDevicePathSize (&PROPERTY_NODE_FROM_LIST_ENTRY (NodeWalker)->DevicePath);

    CopyMem (
      &BufferNode->DevicePath,
      &PROPERTY_NODE_FROM_LIST_ENTRY (NodeWalker)->DevicePath,
      BufferSize
      );

    BufferNode->Hdr.NumberOfProperties = (UINT32) PROPERTY_NODE_FROM_LIST_ENTRY (NodeWalker)->Hdr.NumberOfProperties;

    Property = GetFirstNode (&PROPERTY_NODE_FROM_LIST_ENTRY (NodeWalker)->Hdr.Properties);

    BufferSize += sizeof (BufferNode->Hdr);
    BufferPtr   = (UINT8 *) BufferNode + BufferSize;

    while (!IsNull (&PROPERTY_NODE_FROM_LIST_ENTRY (NodeWalker)->Hdr.Properties, Property)) {
      CopyMem (
        BufferPtr,
        EFI_DEVICE_PATH_PROPERTY_FROM_LIST_ENTRY (Property)->Name,
        EFI_DEVICE_PATH_PROPERTY_FROM_LIST_ENTRY (Property)->Name->Size
        );

      CopyMem (
        BufferPtr + (EFI_DEVICE_PATH_PROPERTY_FROM_LIST_ENTRY (Property))->Name->Size,
        EFI_DEVICE_PATH_PROPERTY_FROM_LIST_ENTRY (Property)->Value,
        EFI_DEVICE_PATH_PROPERTY_FROM_LIST_ENTRY (Property)->Value->Size
        );

      BufferPtr  += EFI_DEVICE_PATH_PROPERTY_SIZE (EFI_DEVICE_PATH_PROPERTY_FROM_LIST_ENTRY (Property));
      BufferSize += EFI_DEVICE_PATH_PROPERTY_SIZE (EFI_DEVICE_PATH_PROPERTY_FROM_LIST_ENTRY (Property));
      Property    = GetNextNode (
                        &PROPERTY_NODE_FROM_LIST_ENTRY (NodeWalker)->Hdr.Properties,
                        Property
                        );
    }

    BufferNode->Hdr.Size = (UINT32) BufferSize;
    BufferNode           = (EFI_DEVICE_PATH_PROPERTY_BUFFER_NODE *)(
                              (UINTN) BufferNode + BufferSize
                              );

    NodeWalker = GetNextNode (Nodes, NodeWalker);
  }

  return EFI_SUCCESS;
}

// InternalReadEfiVariableProperties
STATIC
EFI_STATUS
InternalReadEfiVariableProperties (
  IN EFI_GUID                   *VendorGuid,
  IN BOOLEAN                    DeleteVariables,
  IN DEVICE_PATH_PROPERTY_DATA  *DevicePathPropertyData
  )
{
  EFI_STATUS                           Status;

  CHAR16                               IndexBuffer[5];
  UINT32                               NumberOfVariables;
  UINT32                               VariableIndex;
  CHAR16                               VariableName[64];
  UINTN                                DataSize;
  UINTN                                BufferSize;
  UINTN                                CurrentBufferSize;
  EFI_DEVICE_PATH_PROPERTY_BUFFER      *Buffer;
  UINT8                                *BufferPtr;
  EFI_DEVICE_PATH_PROPERTY_BUFFER_NODE *BufferNode;
  UINTN                                NodeIndex;
  UINTN                                Index;
  EFI_DEVICE_PATH_PROPERTY_DATA        *NameData;
  EFI_DEVICE_PATH_PROPERTY_DATA        *ValueData;
  UINT32                               Attributes;

  NumberOfVariables = 0;
  BufferSize        = 0;
  Status = EFI_BUFFER_TOO_SMALL;

  while (Status == EFI_BUFFER_TOO_SMALL && NumberOfVariables < APPLE_PATH_PROPERTY_VARIABLE_MAX_NUM) {
    UnicodeSPrint (
      &IndexBuffer[0],
      sizeof (IndexBuffer),
      L"%04x",
      NumberOfVariables
      );

    StrCpyS (VariableName, ARRAY_SIZE (VariableName), APPLE_PATH_PROPERTIES_VARIABLE_NAME);
    StrCatS (VariableName, ARRAY_SIZE (VariableName), IndexBuffer);

    DataSize = 0;
    Status   = gRT->GetVariable (
                      VariableName,
                      VendorGuid,
                      NULL,
                      &DataSize,
                      NULL
                      );

    ++NumberOfVariables;
    if (OcOverflowAddUN (BufferSize, DataSize, &BufferSize)) {
      //
      // Should never trigger due to BufferSize being 4G at least.
      //
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // NumberOfVariables check is an extra caution here, writing 0x10000 variables
    // to NVRAM is pretty much impossible.
    //
  }

  //
  // Discard low size with forced approval.
  //
  if (BufferSize < sizeof (EFI_DEVICE_PATH_PROPERTY_BUFFER)) {
    return EFI_SUCCESS;
  }

  Buffer = AllocateZeroPool (BufferSize);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BufferPtr = (UINT8 *) Buffer;
  CurrentBufferSize = BufferSize;
  Status = EFI_SUCCESS;

  for (VariableIndex = 0; VariableIndex < NumberOfVariables; ++VariableIndex) {
    UnicodeSPrint (
      &IndexBuffer[0],
      sizeof (IndexBuffer),
      L"%04x",
      VariableIndex
      );

    StrCpyS (VariableName, ARRAY_SIZE (VariableName), APPLE_PATH_PROPERTIES_VARIABLE_NAME);
    StrCatS (VariableName, ARRAY_SIZE (VariableName), IndexBuffer);

    DataSize = CurrentBufferSize;
    Status   = gRT->GetVariable (
                      VariableName,
                      VendorGuid,
                      &Attributes,
                      &DataSize,
                      BufferPtr
                      );

    if (EFI_ERROR (Status)) {
      break;
    }

    if (DeleteVariables) {
      Status = gRT->SetVariable (VariableName, VendorGuid, Attributes, 0, NULL);

      if (EFI_ERROR (Status)) {
        break;
      }
    }

    BufferPtr         += DataSize;
    CurrentBufferSize -= DataSize;
  }

  //
  // Force success on format mismatch, this slightly differs from Apple implementation,
  // where variable read failure results in error unless EFI_NOT_FOUND.
  //
  if (Buffer->Size != BufferSize
    || Buffer->Version != EFI_DEVICE_PATH_PROPERTY_DATABASE_VERSION
    || Buffer->NumberOfNodes == 0) {
    FreePool (Buffer);
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return Status;
  }

  //
  // TODO: while this does not seem exploitable, we should sanity check the input data.
  //
 
  BufferNode    = &Buffer->Nodes[0];
  for (NodeIndex = 0; NodeIndex < Buffer->NumberOfNodes; ++NodeIndex) {
    DataSize = GetDevicePathSize (&BufferNode->DevicePath);

    if (BufferNode->Hdr.NumberOfProperties > 0) {
      NameData = (EFI_DEVICE_PATH_PROPERTY_DATA *)(
                   (UINTN)BufferNode + DataSize + sizeof (*Buffer)
                   );

      ValueData = (EFI_DEVICE_PATH_PROPERTY_DATA *)(
                    (UINTN)NameData + NameData->Size
                    );

      for (Index = 0; Index < BufferNode->Hdr.NumberOfProperties; ++Index) {
        //
        // Apple implementation does check for an error here, and returns failure
        // if all the writes failed. This is illogical, so we just ignore it.
        //
        DevicePathPropertyData->Protocol.SetProperty (
          &DevicePathPropertyData->Protocol,
          &BufferNode->DevicePath,
          (CHAR16 *)&NameData->Data,
          (VOID *)&ValueData->Data,
          (UINTN)(
            ValueData->Size
              - sizeof (ValueData->Size)
            )
          );

        NameData = (EFI_DEVICE_PATH_PROPERTY_DATA *)(
                     (UINTN)ValueData + ValueData->Size
                     );

        ValueData =
          (EFI_DEVICE_PATH_PROPERTY_DATA *)(
            (UINTN)ValueData + ValueData->Size + NameData->Size
            );
      }
    }

    BufferNode = (EFI_DEVICE_PATH_PROPERTY_BUFFER_NODE *)(
                   (UINTN)BufferNode + (UINTN)BufferNode->Hdr.Size
                   );
  }

  FreePool (Buffer);

  return EFI_SUCCESS;
}

STATIC EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL DppDbProtocolTemplate = {
  EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL_REVISION,
  DppDbGetProperty,
  DppDbSetProperty,
  DppDbRemoveProperty,
  DppDbGetPropertyBuffer
};

EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL *
OcDevicePathPropertyInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS                                  Status;

  EFI_DEVICE_PATH_PROPERTY_BUFFER             *Buffer;
  EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL  *Protocol;
  UINT8                                       *BufferPtr;
  DEVICE_PATH_PROPERTY_DATA                   *DevicePathPropertyData;
  UINTN                                       DataSize;
  UINT32                                      VariableIndex;
  CHAR16                                      IndexBuffer[5];
  CHAR16                                      VariableName[64];
  UINTN                                       VariableSize;
  UINT32                                      Attributes;
  EFI_HANDLE                                  Handle;

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gEfiDevicePathPropertyDatabaseProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCDP: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
                    &gEfiDevicePathPropertyDatabaseProtocolGuid,
                    NULL,
                    (VOID *)&Protocol
                    );

    if (!EFI_ERROR (Status)) {
      return Protocol;
    }
  }
  
  DevicePathPropertyData = AllocateZeroPool (sizeof (*DevicePathPropertyData));
  if (DevicePathPropertyData == NULL) {
    return NULL;
  }

  DevicePathPropertyData->Signature = DEVICE_PATH_PROPERTY_DATA_SIGNATURE;

  CopyMem (
    &DevicePathPropertyData->Protocol,
    &DppDbProtocolTemplate,
    sizeof (DppDbProtocolTemplate)
    );

  InitializeListHead (&DevicePathPropertyData->Nodes);

  if (PcdGetBool (PcNvramInitDevicePropertyDatabase)) {
    Status = InternalReadEfiVariableProperties (
               &gAppleVendorVariableGuid,
               FALSE,
               DevicePathPropertyData
               );

    if (EFI_ERROR (Status)) {
      FreePool (DevicePathPropertyData);
      return NULL;
    }

    DevicePathPropertyData->Modified = FALSE;

    Status = InternalReadEfiVariableProperties (
               &gAppleBootVariableGuid,
               TRUE,
               DevicePathPropertyData
               );

    if (EFI_ERROR (Status)) {
      FreePool (DevicePathPropertyData);
      return NULL;
    }

    if (DevicePathPropertyData->Modified) {
      DataSize = 0;
      Status   = DppDbGetPropertyBuffer (
                   &DevicePathPropertyData->Protocol,
                   NULL,
                   &DataSize
                   );

      if (Status != EFI_BUFFER_TOO_SMALL) {
        FreePool (DevicePathPropertyData);
        return NULL;
      }

      Buffer = AllocateZeroPool (DataSize);
      if (Buffer == NULL) {
        FreePool (DevicePathPropertyData);
        return NULL;
      }

      Status = DppDbGetPropertyBuffer (
                 &DevicePathPropertyData->Protocol,
                 Buffer,
                 &DataSize
                 );
      if (EFI_ERROR (Status)) {
        FreePool (Buffer);
        FreePool (DevicePathPropertyData);
        return NULL;
      }

      VariableIndex = 0;
      Attributes = (EFI_VARIABLE_NON_VOLATILE
                  | EFI_VARIABLE_BOOTSERVICE_ACCESS
                  | EFI_VARIABLE_RUNTIME_ACCESS);
      BufferPtr = (UINT8 *) Buffer;

      while (VariableIndex < APPLE_PATH_PROPERTY_VARIABLE_MAX_NUM && DataSize > 0) {
        UnicodeSPrint (
          &IndexBuffer[0],
          sizeof (IndexBuffer),
          L"%04x",
          VariableIndex
          );

        StrCpyS (VariableName, ARRAY_SIZE (VariableName), APPLE_PATH_PROPERTIES_VARIABLE_NAME);
        StrCatS (VariableName, ARRAY_SIZE (VariableName), IndexBuffer);

        VariableSize = MIN (
                         DataSize,
                         APPLE_PATH_PROPERTY_VARIABLE_MAX_SIZE
                         );

        Status = gRT->SetVariable (
                        VariableName,
                        &gAppleVendorVariableGuid,
                        Attributes,
                        VariableSize,
                        BufferPtr
                        );

        if (EFI_ERROR (Status)) {
          break;
        }

        BufferPtr += VariableSize;
        DataSize  -= VariableSize;
        ++VariableIndex;
      }

      FreePool (Buffer);
      if (EFI_ERROR (Status) || DataSize != 0) {
        FreePool (DevicePathPropertyData);
        return NULL;
      }

      while (!EFI_ERROR (Status) && VariableIndex < APPLE_PATH_PROPERTY_VARIABLE_MAX_NUM) {
        UnicodeSPrint (
          &IndexBuffer[0],
          sizeof (IndexBuffer),
          L"%04x",
          VariableIndex
          );

        StrCpyS (VariableName, ARRAY_SIZE (VariableName), APPLE_PATH_PROPERTIES_VARIABLE_NAME);
        StrCatS (VariableName, ARRAY_SIZE (VariableName), IndexBuffer);

        VariableSize = 0;
        Status       = gRT->GetVariable (
                              VariableName,
                              &gAppleVendorVariableGuid,
                              &Attributes,
                              &VariableSize,
                              NULL
                              );

        if (Status != EFI_BUFFER_TOO_SMALL) {
          Status = EFI_SUCCESS;
          break;
        }

        Status       = gRT->SetVariable (
                              VariableName,
                              &gAppleVendorVariableGuid,
                              Attributes,
                              0,
                              NULL
                              );

        ++VariableIndex;
      }

      if (EFI_ERROR (Status)) {
        FreePool (DevicePathPropertyData);
        return NULL;
      }
    }

    DevicePathPropertyData->Modified = FALSE;
  }

  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
    &Handle,
    &gEfiDevicePathPropertyDatabaseProtocolGuid,
    EFI_NATIVE_INTERFACE,
    &DevicePathPropertyData->Protocol
    );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &DevicePathPropertyData->Protocol;
}
