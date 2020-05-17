/** @file
Copyright (C) 2014 - 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef DEVICE_PATH_PROPERTY_DATABASE_H
#define DEVICE_PATH_PROPERTY_DATABASE_H

// EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL_REVISION
#define EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL_REVISION  0x00010000

// Related definitions

// DEVICE_PATH_PROPERTY_DATA
/// The structure defining the header of a Device Path Property.
typedef struct {
  UINT32 Size;  ///< The size, in bytes, of the current data set.
  UINT8  Data[];
} EFI_DEVICE_PATH_PROPERTY_DATA;

// DEVICE_PATH_PROPERTY_BUFFER_NODE_HDR
/// The node header structure.
typedef struct {
  UINT32 Size;                /// The size, in bytes, of the entire node.
  UINT32 NumberOfProperties;  /// The number of properties within this node.
} EFI_DEVICE_PATH_PROPERTY_BUFFER_NODE_HDR;

// DEVICE_PATH_PROPERTY_BUFFER_NODE
/// The structure defining the header of a Device Property node.
typedef struct {
  /// The node header structure.
  EFI_DEVICE_PATH_PROPERTY_BUFFER_NODE_HDR  Hdr;
  /// The device path for the current node.
  EFI_DEVICE_PATH_PROTOCOL                   DevicePath;

///// The begin of the Device Property data set.
//EFI_DEVICE_PATH_PROPERTY_DATA Data;
} EFI_DEVICE_PATH_PROPERTY_BUFFER_NODE;

// DEVICE_PATH_PROPERTY_BUFFER
/// The structure defining the header of a Device Property Buffer.
typedef struct {
  ///
  /// The size, in bytes, of the entire Buffer.
  ///
  UINT32                               Size;
  UINT32                               Version;
  ///
  /// The number of nodes in the Buffer.
  ///
  UINT32                               NumberOfNodes;
  EFI_DEVICE_PATH_PROPERTY_BUFFER_NODE Nodes[];
} EFI_DEVICE_PATH_PROPERTY_BUFFER;

// Protocol declaration

// EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL_GUID
/// The GUID of the EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL.
#define EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL_GUID   \
  { 0x91BD12FE, 0xF6C3, 0x44FB,                           \
    { 0xA5, 0xB7, 0x51, 0x22, 0xAB, 0x30, 0x3A, 0xE0 } }

typedef
struct EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL
EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL;

// DPP_DATABASE_GET_PROPERTY
/** Locates a device property in the database and returns its value into Value.

  @param[in]      This        Protocol instance pointer.
  @param[in]      DevicePath  The device path of the device to get the property
                              of.
  @param[in]      Name        The Name of the requested property.
  @param[out]     Value       The Buffer allocated by the caller to return the
                              value of the property into.
  @param[in,out]  Size        On input the size of the allocated Value Buffer.
                              On output the size required to fill the Buffer.

  @retval EFI_BUFFER_TOO_SMALL  The memory required to return the value exceeds
                                the size of the allocated Buffer.
                                The required size to complete the operation has
                                been returned into Size.
  @retval EFI_NOT_FOUND         The given device path does not have a property
                                with the specified Name.
  @retval EFI_SUCCESS           The operation completed successfully.
**/
typedef
EFI_STATUS
(EFIAPI *DPP_DATABASE_GET_PROPERTY)(
  IN     EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL  *This,
  IN     EFI_DEVICE_PATH_PROTOCOL                    *DevicePath,
  IN     CONST CHAR16                                *Name,
  OUT    VOID                                        *Value OPTIONAL,
  IN OUT UINTN                                       *Size
  );

// DPP_DATABASE_SET_PROPERTY
/** Sets the sepcified property of the given device path to the provided Value.

  @param[in]  This        Protocol instance pointer.
  @param[in]  DevicePath  The device path of the device to set the property of.
  @param[in]  Name        The Name of the desired property.
  @param[in]  Value       The Buffer holding the value to set the property to.
  @param[out] Size        The size of the Value Buffer.

  @retval EFI_OUT_OF_RESOURCES  The memory necessary to complete the operation
                                could not be allocated.
  @retval EFI_SUCCESS           The operation completed successfully.
  **/
typedef
EFI_STATUS
(EFIAPI *DPP_DATABASE_SET_PROPERTY)(
  IN EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL                    *DevicePath,
  IN CONST CHAR16                                *Name,
  IN VOID                                        *Value,
  IN UINTN                                       Size
  );

// DPP_DATABASE_REMOVE_PROPERTY
/** Removes the sepcified property from the given device path.

  @param[in] This        Protocol instance pointer.
  @param[in] DevicePath  The device path of the device to set the property of.
  @param[in] Name        The Name of the desired property.

  @retval EFI_NOT_FOUND  The given device path does not have a property with
                         the specified Name.
  @retval EFI_SUCCESS    The operation completed successfully.
**/
typedef
EFI_STATUS
(EFIAPI *DPP_DATABASE_REMOVE_PROPERTY)(
  IN EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL                    *DevicePath,
  IN CONST CHAR16                                *Name
  );

// DPP_DATABASE_GET_PROPERTY_BUFFER
/** Returns a Buffer of all device properties into Buffer.

  @param[in]      This    Protocol instance pointer.
  @param[out]     Buffer  The Buffer allocated by the caller to return the
                          property Buffer into.
  @param[in,out]  Size    On input the size of the allocated Buffer.
                          On output the size required to fill the Buffer.

  @retval EFI_BUFFER_TOO_SMALL  The memory required to return the value exceeds
                                the size of the allocated Buffer.
                                The required size to complete the operation has
                                been returned into Size.
  @retval EFI_SUCCESS           The operation completed successfully.
**/
typedef
EFI_STATUS
(EFIAPI *DPP_DATABASE_GET_PROPERTY_BUFFER)(
  IN     EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL  *This,
  OUT    EFI_DEVICE_PATH_PROPERTY_BUFFER             *Buffer OPTIONAL,
  IN OUT UINTN                                       *Size
  );

// EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL
/// The structure exposed by the EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL.
struct EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL {
  /// The revision of the installed protocol.
  UINT64                           Revision;

  /// Locates a device property in the database and returns its value into Value.
  DPP_DATABASE_GET_PROPERTY        GetProperty;

  /// Sets the sepcified property of the given device path to the provided Value.
  DPP_DATABASE_SET_PROPERTY        SetProperty;

  /// Removes the sepcified property from the given device path.
  DPP_DATABASE_REMOVE_PROPERTY     RemoveProperty;

  /// Returns a Buffer of all device properties into Buffer.
  DPP_DATABASE_GET_PROPERTY_BUFFER GetPropertyBuffer;
};

// gEfiDevicePathPropertyDatabaseProtocolGuid
extern EFI_GUID gEfiDevicePathPropertyDatabaseProtocolGuid;

#endif // DEVICE_PATH_PROPERTY_DATABASE_H
