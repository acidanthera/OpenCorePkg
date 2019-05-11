/** @file
  This file declares the Firmware Volume Protocol.

  The Firmware Volume Protocol provides file-level access to the firmware volume.
  Each firmware volume driver must produce an instance of the Firmware Volume
  Protocol if the firmware volume is to be visible to the system. The Firmware
  Volume Protocol also provides mechanisms for determining and modifying some
  attributes of the firmware volume.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This protocol is defined in Firmware Volume specification.
  Version 0.9.

**/

#ifndef _FIRMWARE_VOLUME_H_
#define _FIRMWARE_VOLUME_H_


//
// Firmware Volume Protocol GUID definition
//
#define EFI_FIRMWARE_VOLUME_PROTOCOL_GUID \
  { \
    0x389F751F, 0x1838, 0x4388, {0x83, 0x90, 0xCD, 0x81, 0x54, 0xBD, 0x27, 0xF8 } \
  }

#define FV_DEVICE_SIGNATURE SIGNATURE_32 ('_', 'F', 'V', '_')

typedef struct _EFI_FIRMWARE_VOLUME_PROTOCOL  EFI_FIRMWARE_VOLUME_PROTOCOL;

//
// FRAMEWORK_EFI_FV_ATTRIBUTES bit definitions
//
typedef UINT64  FRAMEWORK_EFI_FV_ATTRIBUTES;

//
// ************************************************************
// FRAMEWORK_EFI_FV_ATTRIBUTES bit definitions
// ************************************************************
//
#define EFI_FV_READ_DISABLE_CAP       0x0000000000000001ULL
#define EFI_FV_READ_ENABLE_CAP        0x0000000000000002ULL
#define EFI_FV_READ_STATUS            0x0000000000000004ULL

#define EFI_FV_WRITE_DISABLE_CAP      0x0000000000000008ULL
#define EFI_FV_WRITE_ENABLE_CAP       0x0000000000000010ULL
#define EFI_FV_WRITE_STATUS           0x0000000000000020ULL

#define EFI_FV_LOCK_CAP               0x0000000000000040ULL
#define EFI_FV_LOCK_STATUS            0x0000000000000080ULL
#define EFI_FV_WRITE_POLICY_RELIABLE  0x0000000000000100ULL

#define EFI_FV_ALIGNMENT_CAP          0x0000000000008000ULL
#define EFI_FV_ALIGNMENT_2            0x0000000000010000ULL
#define EFI_FV_ALIGNMENT_4            0x0000000000020000ULL
#define EFI_FV_ALIGNMENT_8            0x0000000000040000ULL
#define EFI_FV_ALIGNMENT_16           0x0000000000080000ULL
#define EFI_FV_ALIGNMENT_32           0x0000000000100000ULL
#define EFI_FV_ALIGNMENT_64           0x0000000000200000ULL
#define EFI_FV_ALIGNMENT_128          0x0000000000400000ULL
#define EFI_FV_ALIGNMENT_256          0x0000000000800000ULL
#define EFI_FV_ALIGNMENT_512          0x0000000001000000ULL
#define EFI_FV_ALIGNMENT_1K           0x0000000002000000ULL
#define EFI_FV_ALIGNMENT_2K           0x0000000004000000ULL
#define EFI_FV_ALIGNMENT_4K           0x0000000008000000ULL
#define EFI_FV_ALIGNMENT_8K           0x0000000010000000ULL
#define EFI_FV_ALIGNMENT_16K          0x0000000020000000ULL
#define EFI_FV_ALIGNMENT_32K          0x0000000040000000ULL
#define EFI_FV_ALIGNMENT_64K          0x0000000080000000ULL

//
// Protocol API definitions
//

/**
  Retrieves attributes, insures positive polarity of attribute bits, and returns
  resulting attributes in an output parameter.

  @param  This                  Indicates the EFI_FIRMWARE_VOLUME_PROTOCOL instance.
  @param  Attributes            Output buffer containing attributes.

  @retval EFI_SUCCESS           The firmware volume attributes were returned.
**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_FV_GET_ATTRIBUTES)(
  IN  EFI_FIRMWARE_VOLUME_PROTOCOL            *This,
  OUT FRAMEWORK_EFI_FV_ATTRIBUTES             *Attributes
  );

/**
  Sets volume attributes

  @param  This                  Indicates the EFI_FIRMWARE_VOLUME_PROTOCOL instance.
  @param  Attributes            On input, Attributes is a pointer to an
                                EFI_FV_ATTRIBUTES containing the desired firmware
                                volume settings. On successful return, it contains
                                the new settings of the firmware volume. On
                                unsuccessful return, Attributes is not modified
                                and the firmware volume settings are not changed.

  @retval EFI_INVALID_PARAMETER A bit in Attributes was invalid.
  @retval EFI_SUCCESS           The requested firmware volume attributes were set
                                and the resulting EFI_FV_ATTRIBUTES is returned in
                                Attributes.
  @retval EFI_ACCESS_DENIED     The Device is locked and does not permit modification.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_FV_SET_ATTRIBUTES)(
  IN EFI_FIRMWARE_VOLUME_PROTOCOL       *This,
  IN OUT FRAMEWORK_EFI_FV_ATTRIBUTES    *Attributes
  );

/**
  Read the requested file (NameGuid) or file information from the firmware volume
  and returns data in Buffer.

  @param  This                  The EFI_FIRMWARE_VOLUME_PROTOCOL instance.
  @param  NameGuid              The pointer to EFI_GUID, which is the filename of
                                the file to read.
  @param  Buffer                The pointer to pointer to buffer in which contents of file are returned.
                                <br>
                                If Buffer is NULL, only type, attributes, and size
                                are returned as there is no output buffer.
                                <br>
                                If Buffer != NULL and *Buffer == NULL, the output
                                buffer is allocated from BS pool by ReadFile.
                                <br>
                                If Buffer != NULL and *Buffer != NULL, the output
                                buffer has been allocated by the caller and is being
                                passed in.
  @param  BufferSize            On input: The buffer size. On output: The size
                                required to complete the read.
  @param  FoundType             The pointer to the type of the file whose data
                                is returned.
  @param  FileAttributes        The pointer to attributes of the file whose data
                                is returned.
  @param  AuthenticationStatus  The pointer to the authentication status of the data.

  @retval EFI_SUCCESS               The call completed successfully.
  @retval EFI_WARN_BUFFER_TOO_SMALL The buffer is too small to contain the requested output.
                                    The buffer filled, and the output is truncated.
  @retval EFI_NOT_FOUND             NameGuid was not found in the firmware volume.
  @retval EFI_DEVICE_ERROR          A hardware error occurred when attempting to
                                    access the firmware volume.
  @retval EFI_ACCESS_DENIED         The firmware volume is configured to disallow reads.
  @retval EFI_OUT_OF_RESOURCES      An allocation failure occurred.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_FV_READ_FILE)(
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN EFI_GUID                       *NameGuid,
  IN OUT VOID                       **Buffer,
  IN OUT UINTN                      *BufferSize,
  OUT EFI_FV_FILETYPE               *FoundType,
  OUT EFI_FV_FILE_ATTRIBUTES        *FileAttributes,
  OUT UINT32                        *AuthenticationStatus
  );

/**
  Read the requested section from the specified file and returns data in Buffer.

  @param  This                  Indicates the EFI_FIRMWARE_VOLUME_PROTOCOL instance.
  @param  NameGuid              Filename identifying the file from which to read.
  @param  SectionType           The section type to retrieve.
  @param  SectionInstance       The instance of SectionType to retrieve.
  @param  Buffer                Pointer to pointer to buffer in which contents of
                                a file are returned.
                                <br>
                                If Buffer is NULL, only type, attributes, and size
                                are returned as there is no output buffer.
                                <br>
                                If Buffer != NULL and *Buffer == NULL, the output
                                buffer is allocated from BS pool by ReadFile.
                                <br>
                                If Buffer != NULL and *Buffer != NULL, the output
                                buffer has been allocated by the caller and is being
                                passed in.
  @param  BufferSize            The pointer to the buffer size passed in, and on
                                output the size required to complete the read.
  @param  AuthenticationStatus  The pointer to the authentication status of the data.

  @retval EFI_SUCCESS                The call completed successfully.
  @retval EFI_WARN_BUFFER_TOO_SMALL  The buffer is too small to contain the requested output.
                                     The buffer is filled and the output is truncated.
  @retval EFI_OUT_OF_RESOURCES       An allocation failure occurred.
  @retval EFI_NOT_FOUND              The name was not found in the firmware volume.
  @retval EFI_DEVICE_ERROR           A hardware error occurred when attempting to
                                     access the firmware volume.
  @retval EFI_ACCESS_DENIED          The firmware volume is configured to disallow reads.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_FV_READ_SECTION)(
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN EFI_GUID                       *NameGuid,
  IN EFI_SECTION_TYPE               SectionType,
  IN UINTN                          SectionInstance,
  IN OUT VOID                       **Buffer,
  IN OUT UINTN                      *BufferSize,
  OUT UINT32                        *AuthenticationStatus
  );

typedef UINT32  FRAMEWORK_EFI_FV_WRITE_POLICY;

#define FRAMEWORK_EFI_FV_UNRELIABLE_WRITE 0x00000000
#define FRAMEWORK_EFI_FV_RELIABLE_WRITE   0x00000001

typedef struct {
  EFI_GUID                *NameGuid;
  EFI_FV_FILETYPE         Type;
  EFI_FV_FILE_ATTRIBUTES  FileAttributes;
  VOID                    *Buffer;
  UINT32                  BufferSize;
} FRAMEWORK_EFI_FV_WRITE_FILE_DATA;

/**
  Write the supplied file (NameGuid) to the FV.

  @param  This                  Indicates the EFI_FIRMWARE_VOLUME_PROTOCOL instance.
  @param  NumberOfFiles         Indicates the number of file records pointed to
                                by FileData.
  @param  WritePolicy           Indicates the level of reliability of the write
                                with respect to things like power failure events.
  @param  FileData              A pointer to an array of EFI_FV_WRITE_FILE_DATA
                                structures. Each element in the array indicates
                                a file to write, and there are NumberOfFiles
                                elements in the input array.

  @retval EFI_SUCCESS           The write completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The firmware volume does not have enough free
                                space to store file(s).
  @retval EFI_DEVICE_ERROR      A hardware error occurred when attempting to
                                access the firmware volume.
  @retval EFI_WRITE_PROTECTED   The firmware volume is configured to disallow writes.
  @retval EFI_NOT_FOUND         A delete was requested, but the requested file was
                                not found in the firmware volume.
  @retval EFI_INVALID_PARAMETER A delete was requested with a multiple file write.
                                An unsupported WritePolicy was requested.
                                An unknown file type was specified.
                                A file system specific error has occurred.
**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_FV_WRITE_FILE)(
  IN EFI_FIRMWARE_VOLUME_PROTOCOL             *This,
  IN UINT32                                   NumberOfFiles,
  IN FRAMEWORK_EFI_FV_WRITE_POLICY            WritePolicy,
  IN FRAMEWORK_EFI_FV_WRITE_FILE_DATA         *FileData
  );

/**
  Given the input key, search for the next matching file in the volume.

  @param  This                  Indicates the EFI_FIRMWARE_VOLUME_PROTOCOL instance.
  @param  Key                   Pointer to a caller allocated buffer that contains
                                an implementation-specific key that is used to track
                                where to begin searching on successive calls.
  @param  FileType              The pointer to the file type to filter for.
  @param  NameGuid              The pointer to Guid filename of the file found.
  @param  Attributes            The pointer to Attributes of the file found.
  @param  Size                  The pointer to Size in bytes of the file found.

  @retval EFI_SUCCESS           The output parameters are filled with data obtained from
                                the first matching file that was found.
  @retval EFI_NOT_FOUND         No files of type FileType were found.
  @retval EFI_DEVICE_ERROR      A hardware error occurred when attempting to access
                                the firmware volume.
  @retval EFI_ACCESS_DENIED     The firmware volume is configured to disallow reads.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_FV_GET_NEXT_FILE)(
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN OUT VOID                       *Key,
  IN OUT EFI_FV_FILETYPE            *FileType,
  OUT EFI_GUID                      *NameGuid,
  OUT EFI_FV_FILE_ATTRIBUTES        *Attributes,
  OUT UINTN                         *Size
  );

//
// Protocol interface structure
//
struct _EFI_FIRMWARE_VOLUME_PROTOCOL {
  ///
  /// Retrieves volume capabilities and current settings.
  ///
  FRAMEWORK_EFI_FV_GET_ATTRIBUTES GetVolumeAttributes;

  ///
  /// Modifies the current settings of the firmware volume.
  ///
  FRAMEWORK_EFI_FV_SET_ATTRIBUTES SetVolumeAttributes;

  ///
  /// Reads an entire file from the firmware volume.
  ///
  FRAMEWORK_EFI_FV_READ_FILE      ReadFile;

  ///
  /// Reads a single section from a file into a buffer.
  ///
  FRAMEWORK_EFI_FV_READ_SECTION   ReadSection;

  ///
  /// Writes an entire file into the firmware volume.
  ///
  FRAMEWORK_EFI_FV_WRITE_FILE     WriteFile;

  ///
  /// Provides service to allow searching the firmware volume.
  ///
  FRAMEWORK_EFI_FV_GET_NEXT_FILE  GetNextFile;

  ///
  ///  Data field that indicates the size in bytes of the Key input buffer for
  ///  the GetNextFile() API.
  ///
  UINT32                KeySize;

  ///
  ///  Handle of the parent firmware volume.
  ///
  EFI_HANDLE            ParentHandle;
};

extern EFI_GUID gEfiFirmwareVolumeProtocolGuid;

#endif
