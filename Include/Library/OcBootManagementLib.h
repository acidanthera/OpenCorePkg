/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_BOOT_MANAGEMENT_LIB_H
#define OC_BOOT_MANAGEMENT_LIB_H

#include <Uefi.h>
#include <IndustryStandard/AppleBootArgs.h>
#include <Library/OcAppleBootPolicyLib.h>
#include <Protocol/LoadedImage.h>

/**
  Operating system boot type.
  WARNING: This is only for debug purposes.
**/
typedef enum OC_BOOT_ENTRY_TYPE_ {
  OcBootUnknown,
  OcBootApple,
  OcBootAppleRecovery,
  OcBootWindows,
  OcBootCustom,
  OcBootSystem
} OC_BOOT_ENTRY_TYPE;

/**
  Action to perform as part of executing a system boot entry.
**/
typedef
EFI_STATUS
(*OC_BOOT_SYSTEM_ACTION)(
  VOID
  );

/**
  Discovered boot entry.
  Note, inner resources must be freed with OcResetBootEntry.
**/
typedef struct OC_BOOT_ENTRY_ {
  //
  // Device path to booter or its directory.
  // Can be NULL, for example, for custom or system entries.
  //
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  //
  // Action to perform on execution. Only valid for system entries.
  //
  OC_BOOT_SYSTEM_ACTION     SystemAction;
  //
  // Obtained human visible name.
  //
  CHAR16                    *Name;
  //
  // Obtained boot path directory.
  // For custom entries this contains tool path.
  //
  CHAR16                    *PathName;
  //
  // Heuristical value signalising about booted os.
  // WARNING: This is only for debug purposes.
  //
  OC_BOOT_ENTRY_TYPE        Type;
  //
  // Set when this entry is an externally available entry (e.g. USB).
  //
  BOOLEAN                   IsExternal;
  //
  // Should try booting from first dmg found in DevicePath.
  //
  BOOLEAN                   IsFolder;
  //
  // Load option data (usually "boot args") size.
  //
  UINT32                    LoadOptionsSize;
  //
  // Load option data (usually "boot args").
  //
  VOID                      *LoadOptions;
} OC_BOOT_ENTRY;

/**
  Perform filtering based on file system basis.
  Ignores all filesystems by default.
  Remove this bit to allow any file system.
**/
#define OC_SCAN_FILE_SYSTEM_LOCK         BIT0

/**
  Perform filtering based on device basis.
  Ignores all devices by default.
  Remove this bit to allow any device type.
**/
#define OC_SCAN_DEVICE_LOCK              BIT1

/**
  Perform filtering based on booter origin.
  Ignores all blessed options not on the same partition.
  Remove this bit to allow foreign booters.
**/
#define OC_SCAN_SELF_TRUST_LOCK          BIT2

/**
  Allow scanning APFS filesystems.
**/
#define OC_SCAN_ALLOW_FS_APFS            BIT8

/**
  Allow scanning HFS filesystems.
**/
#define OC_SCAN_ALLOW_FS_HFS             BIT9

/**
  Allow scanning ESP filesystems.
**/
#define OC_SCAN_ALLOW_FS_ESP             BIT10

/**
  Allow scanning NTFS filesystems.
**/
#define OC_SCAN_ALLOW_FS_NTFS            BIT11

/**
  Allow scanning EXT filesystems (e.g. EXT4).
**/
#define OC_SCAN_ALLOW_FS_EXT             BIT12

/**
  Allow scanning SATA devices.
**/
#define OC_SCAN_ALLOW_DEVICE_SATA        BIT16

/**
  Allow scanning SAS and Mac NVMe devices.
**/
#define OC_SCAN_ALLOW_DEVICE_SASEX       BIT17

/**
  Allow scanning SCSI devices.
**/
#define OC_SCAN_ALLOW_DEVICE_SCSI        BIT18

/**
  Allow scanning NVMe devices.
**/
#define OC_SCAN_ALLOW_DEVICE_NVME        BIT19

/**
  Allow scanning ATAPI devices.
**/
#define OC_SCAN_ALLOW_DEVICE_ATAPI       BIT20

/**
  Allow scanning USB devices.
**/
#define OC_SCAN_ALLOW_DEVICE_USB         BIT21

/**
  Allow scanning FireWire devices.
**/
#define OC_SCAN_ALLOW_DEVICE_FIREWIRE    BIT22

/**
  Allow scanning SD card devices.
**/
#define OC_SCAN_ALLOW_DEVICE_SDCARD      BIT23

/**
  All device bits used by OC_SCAN_DEVICE_LOCK.
**/
#define OC_SCAN_DEVICE_BITS ( \
  OC_SCAN_ALLOW_DEVICE_SATA     | OC_SCAN_ALLOW_DEVICE_SASEX | \
  OC_SCAN_ALLOW_DEVICE_SCSI     | OC_SCAN_ALLOW_DEVICE_NVME  | \
  OC_SCAN_ALLOW_DEVICE_ATAPI    | OC_SCAN_ALLOW_DEVICE_USB   | \
  OC_SCAN_ALLOW_DEVICE_FIREWIRE | OC_SCAN_ALLOW_DEVICE_SDCARD)

/**
  All device bits used by OC_SCAN_DEVICE_LOCK.
**/
#define OC_SCAN_FILE_SYSTEM_BITS ( \
  OC_SCAN_ALLOW_FS_APFS | OC_SCAN_ALLOW_FS_HFS | OC_SCAN_ALLOW_FS_ESP | \
  OC_SCAN_ALLOW_FS_NTFS | OC_SCAN_ALLOW_FS_EXT)

/**
  By default allow booting from APFS from internal drives.
**/
#define OC_SCAN_DEFAULT_POLICY ( \
  OC_SCAN_FILE_SYSTEM_LOCK   | OC_SCAN_DEVICE_LOCK | \
  OC_SCAN_SELF_TRUST_LOCK    | OC_SCAN_ALLOW_FS_APFS | \
  OC_SCAN_ALLOW_DEVICE_SATA  | OC_SCAN_ALLOW_DEVICE_SASEX | \
  OC_SCAN_ALLOW_DEVICE_SCSI  | OC_SCAN_ALLOW_DEVICE_NVME)

/**
  OcLoadBootEntry Mode policy bits allow to configure OcLoadBootEntry behaviour.
**/

/**
  Thin EFI image loading (normal PE) is allowed.
**/
#define OC_LOAD_ALLOW_EFI_THIN_BOOT  BIT0
/**
  FAT EFI image loading (Apple FAT PE) is allowed.
  These can be found on macOS 10.8 and below.
**/
#define OC_LOAD_ALLOW_EFI_FAT_BOOT   BIT1
/**
  One level recursion into dmg file is allowed.
  It is assumed that dmg contains a single volume and a single blessed entry.
  Loading dmg from dmg is not allowed in any case.
**/
#define OC_LOAD_ALLOW_DMG_BOOT       BIT2
/**
  Abort loading on invalid Apple-like signature.
  If file is signed with Apple-like signature, and it is mismatched, then abort.
  @warn Unsigned files or UEFI-signed files will skip this check.
  @warn It is ignored what certificate was used for signing.
**/
#define OC_LOAD_VERIFY_APPLE_SIGN    BIT8
/**
  Abort loading on missing Apple-like signature.
  If file is not signed with Apple-like signature (valid or not) then abort.
  @warn Unsigned files or UEFI-signed files will not load with this check.
  @warn Without OC_LOAD_VERIFY_APPLE_SIGN corrupted binaries may still load.
**/
#define OC_LOAD_REQUIRE_APPLE_SIGN   BIT9
/**
  Abort loading on untrusted key (otherwise may warn).
  @warn Unsigned files or UEFI-signed files will skip this check.
**/
#define OC_LOAD_REQUIRE_TRUSTED_KEY  BIT10
/**
  Trust specified (as OcLoadBootEntry argument) custom keys.
**/
#define OC_LOAD_TRUST_CUSTOM_KEY     BIT16
/**
  Trust Apple CFFD3E6B public key.
  TODO: Move certificates from ApplePublicKeyDb.h to EfiPkg?
**/
#define OC_LOAD_TRUST_APPLE_V1_KEY   BIT17
/**
  Trust Apple E50AC288 public key.
  TODO: Move certificates from ApplePublicKeyDb.h to EfiPkg?
**/
#define OC_LOAD_TRUST_APPLE_V2_KEY   BIT18
/**
  Default moderate policy meant to augment secure boot facilities.
  Loads almost everything and bypasses secure boot for Apple and Custom signed binaries.
**/
#define OC_LOAD_DEFAULT_POLICY ( \
  OC_LOAD_ALLOW_EFI_THIN_BOOT | OC_LOAD_ALLOW_DMG_BOOT      | OC_LOAD_REQUIRE_APPLE_SIGN | \
  OC_LOAD_VERIFY_APPLE_SIGN   | OC_LOAD_REQUIRE_TRUSTED_KEY | \
  OC_LOAD_TRUST_CUSTOM_KEY    | OC_LOAD_TRUST_APPLE_V1_KEY  | OC_LOAD_TRUST_APPLE_V2_KEY)

/**
  Exposed start interface with chosen boot entry but otherwise equivalent
  to EFI_BOOT_SERVICES StartImage.
**/
typedef
EFI_STATUS
(EFIAPI *OC_IMAGE_START) (
  IN  OC_BOOT_ENTRY               *ChosenEntry,
  IN  EFI_HANDLE                  ImageHandle,
  OUT UINTN                       *ExitDataSize,
  OUT CHAR16                      **ExitData    OPTIONAL
  );

/**
  Exposed custom entry load interface.
  Must return allocated file buffer from pool.
**/
typedef
EFI_STATUS
(EFIAPI *OC_CUSTOM_READ) (
  IN  VOID                        *Context,
  IN  OC_BOOT_ENTRY               *ChosenEntry,
  OUT VOID                        **Data,
  OUT UINT32                      *DataSize,
  OUT EFI_DEVICE_PATH_PROTOCOL    **DevicePath OPTIONAL
  );

/**
  Custom picker entry.
**/
typedef struct {
  //
  // Entry name.
  //
  CONST CHAR8  *Name;
  //
  // Entry path.
  //
  CONST CHAR8  *Path;
  //
  // Entry boot arguments.
  //
  CONST CHAR8  *Arguments;
} OC_PICKER_ENTRY;

/**
  Privilege levels to escalate to
**/
typedef enum {
  OcPrivilegeUnauthorized = 0,
  OcPrivilegeAuthorized   = 1
} OC_PRIVILEGE_LEVEL;

/**
  Request a privilege escalation, for example by prompting for a password.
**/
typedef
EFI_STATUS
(EFIAPI *OC_REQ_PRIVILEGE)(
  IN VOID                *Context,
  IN OC_PRIVILEGE_LEVEL  Level
  );

/**
  Picker behaviour action.
**/
typedef enum {
  OcPickerDefault           = 0,
  OcPickerShowPicker        = 1,
  OcPickerResetNvram        = 2,
  OcPickerBootApple         = 3,
  OcPickerBootAppleRecovery = 4,
} OC_PICKER_CMD;

/**
  Boot picker context describing picker behaviour.
**/
typedef struct {
  //
  // Scan policy (e.g. OC_SCAN_DEFAULT_POLICY).
  //
  UINT32           ScanPolicy;
  //
  // Load policy (e.g. OC_LOAD_DEFAULT_POLICY).
  //
  UINT32           LoadPolicy;
  //
  // Default entry selection timeout (pass 0 to ignore).
  //
  UINT32           TimeoutSeconds;
  //
  // Define picker behaviour.
  // For example, show boot menu or just boot the default option.
  //
  OC_PICKER_CMD    PickerCommand;
  //
  // Use custom (gOcVendorVariableGuid) for Boot#### variables.
  //
  BOOLEAN          CustomBootGuid;
  //
  // Custom entry reading routine, optional for no custom entries.
  //
  OC_CUSTOM_READ   CustomRead;
  //
  // Context to pass to CustomRead, optional.
  //
  VOID             *CustomEntryContext;
  //
  // Image starting routine used, required.
  //
  OC_IMAGE_START   StartImage;
  //
  // Handle to exclude scanning from, optional.
  //
  EFI_HANDLE       ExcludeHandle;
  //
  // Privilege escalation requesting routine.
  //
  OC_REQ_PRIVILEGE RequestPrivilege;
  //
  // Context to pass to RequestPrivilege, optional.
  //
  VOID             *PrivilegeContext;
  //
  // Additional suffix to include by the interface.
  //
  CONST CHAR8      *TitleSuffix;
  //
  // Enable polling boot arguments.
  //
  BOOLEAN          PollAppleHotKeys;
  //
  // Append the "Reset NVRAM" option to the boot entry list.
  //
  BOOLEAN          ShowNvramReset;
  //
  // Additional boot arguments for Apple loaders.
  //
  CHAR8            AppleBootArgs[BOOT_LINE_LENGTH];
  //
  // Number of custom boot paths (bless override).
  //
  UINTN            NumCustomBootPaths;
  //
  // Custom boot paths (bless override).  Must start with '\'.
  //
  CHAR16           **CustomBootPaths;
  //
  // Number of absolute custom entries.
  //
  UINT32           AbsoluteEntryCount;
  //
  // Number of total custom entries (absolute and tools).
  //
  UINT32           AllCustomEntryCount;
  //
  // Custom picker entries.  Absolute entries come first.
  //
  OC_PICKER_ENTRY  CustomEntries[];
} OC_PICKER_CONTEXT;

/**
  Hibernate detection bit mask for hibernate source usage.
**/
#define HIBERNATE_MODE_NONE   0U
#define HIBERNATE_MODE_RTC    1U
#define HIBERNATE_MODE_NVRAM  2U

/**
  Describe boot entry contents by setting fields other than DevicePath.

  @param[in]  BootPolicy     Apple Boot Policy Protocol.
  @param[in]  BootEntry      Located boot entry.

  @retval EFI_SUCCESS          The entry point is described successfully.
**/
EFI_STATUS
OcDescribeBootEntry (
  IN     APPLE_BOOT_POLICY_PROTOCOL *BootPolicy,
  IN OUT OC_BOOT_ENTRY              *BootEntry
  );

/**
  Release boot entry contents allocated from pool.

  @param[in,out]  BootEntry      Located boot entry.
**/
VOID
OcResetBootEntry (
  IN OUT OC_BOOT_ENTRY              *BootEntry
  );

/**
  Release boot entries.

  @param[in,out]  BootEntry      Located boot entry array from pool.
  @param[in]      Count          Boot entry count.
**/
VOID
OcFreeBootEntries (
  IN OUT OC_BOOT_ENTRY              *BootEntries,
  IN     UINTN                      Count
  );

/**
  Scan system for boot entries.

  @param[in]  BootPolicy     Apple Boot Policy Protocol.
  @param[in]  Context        Picker context.
  @param[out] BootEntries    List of boot entries (allocated from pool).
  @param[out] Count          Number of boot entries.
  @param[out] AllocCount     Number of allocated boot entries.
  @param[in]  LoadHandle     Load handle to skip.
  @param[in]  Describe       Automatically fill description fields

  @retval EFI_SUCCESS        Executed successfully and found entries.
**/
EFI_STATUS
OcScanForBootEntries (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  OC_PICKER_CONTEXT           *Context,
  OUT OC_BOOT_ENTRY               **BootEntries,
  OUT UINTN                       *Count,
  OUT UINTN                       *AllocCount OPTIONAL,
  IN  BOOLEAN                     Describe
  );

/**
  Obtain default entry from picker context.

  @param[in]      Context          Picker context.
  @param[in,out]  BootEntries      Described list of entries, may get updated.
  @param[in]      NumBootEntries   Positive number of boot entries.

  @retval  boot entry or 0.
**/
UINT32
OcGetDefaultBootEntry (
  IN     OC_PICKER_CONTEXT  *Context,
  IN OUT OC_BOOT_ENTRY      *BootEntries,
  IN     UINTN              NumBootEntries
  );

typedef struct {
  OC_PRIVILEGE_LEVEL CurrentLevel;
  CONST UINT8        *Salt;
  UINT32             SaltSize;
  CONST UINT8        *Hash;
} OC_PRIVILEGE_CONTEXT;

/**
  Show simple password prompt and return verification status.

  @param[in] Context  Privilege context.
  @param[in] Level    The privilege level to request escalating to.

  @retval EFI_SUCCESS  The privilege level has been escalated successfully.
  @retval EFI_ABORTED  The privilege escalation has been aborted.
  @retval other        The system must be considered compromised.

**/
EFI_STATUS
EFIAPI
OcShowSimplePasswordRequest (
  IN VOID                *Context,
  IN OC_PRIVILEGE_LEVEL  Level
  );

/**
  Show simple boot entry selection menu and return chosen entry.

  @param[in]  Context          Picker context.
  @param[in]  BootEntries      Described list of entries.
  @param[in]  Count            Positive number of boot entries.
  @param[in]  DefaultEntry     Default boot entry (DefaultEntry < Count).
  @param[in]  ChosenBootEntry  Chosen boot entry from BootEntries on success.

  @retval EFI_SUCCESS          Executed successfully and picked up an entry.
  @retval EFI_ABORTED          When the user chose to by pressing Esc or 0.
**/
EFI_STATUS
OcShowSimpleBootMenu (
  IN  OC_PICKER_CONTEXT           *Context,
  IN  OC_BOOT_ENTRY               *BootEntries,
  IN  UINTN                       Count,
  IN  UINTN                       DefaultEntry,
  OUT OC_BOOT_ENTRY               **ChosenBootEntry
  );

/**
  Load & start boot entry loader image with given options.

  @param[in]  BootPolicy     Apple Boot Policy Protocol.
  @param[in]  Context        Picker context.
  @param[in]  BootEntry      Located boot entry.
  @param[in]  ParentHandle   Parent image handle.

  @retval EFI_SUCCESS        The image was found, started, and ended succesfully.
**/
EFI_STATUS
OcLoadBootEntry (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  OC_PICKER_CONTEXT           *Context,
  IN  OC_BOOT_ENTRY               *BootEntry,
  IN  EFI_HANDLE                  ParentHandle
  );

/**
  Handle hibernation detection for later loading.

  @param[in]  HibernateMask  Hibernate detection mask.

  @retval EFI_SUCCESS        Hibernation mode was found and activated.
**/
EFI_STATUS
OcActivateHibernateWake (
  IN UINT32                       HibernateMask
  );

/**
  Check if active hibernation is happening.

  @retval TRUE on waking from hibernation.
**/
BOOLEAN
OcIsAppleHibernateWake (
  VOID
  );

/**
  Check pressed hotkeys and update booter context based on this.

  @param[in,out]  Context       Picker context.
**/
VOID
OcLoadPickerHotKeys (
  IN OUT OC_PICKER_CONTEXT  *Context
  );

/**
  Obtains key index from user input.

  @param[in,out]  Context   Picker context.
  @param[in]      Time      Timeout to wait for.

  @returns key index [0, OC_INPUT_MAX), OC_INPUT_ABORTED, or OC_INPUT_INVALID.
**/
INTN
OcWaitForAppleKeyIndex (
  IN OUT OC_PICKER_CONTEXT  *Context,
  IN     UINTN              Timeout
  );

/**
  Install missing boot policy, scan, and show simple boot menu.

  @param[in]  Context       Picker context.

  @retval does not return unless a fatal error happened.
**/
EFI_STATUS
OcRunSimpleBootPicker (
  IN  OC_PICKER_CONTEXT  *Context
  );

/**
  Get device scan policy type.

  @param[in]  Handle        Device/partition handle.
  @param[out] External      Check whether device is external.

  @retval required policy or 0 on mismatch.
**/
UINT32
OcGetDevicePolicyType (
  IN  EFI_HANDLE   Handle,
  OUT BOOLEAN      *External  OPTIONAL
  );

/**
  Get file system scan policy type.

  @param[in]  Handle        Partition handle.

  @retval required policy or 0 on mismatch.
**/
UINT32
OcGetFileSystemPolicyType (
  IN  EFI_HANDLE   Handle
  );

/**
  Check if supplied device path contains Apple bootloader.

  @param[in]  DevicePath        Device path.

  @retval TRUE for potentially Apple images.
**/
BOOLEAN
OcIsAppleBootDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Get loaded image protocol for Apple bootloader.

  @param[in]  Handle        Image handle.

  @retval loaded image protocol or NULL for non Apple images.
**/
EFI_LOADED_IMAGE_PROTOCOL *
OcGetAppleBootLoadedImage (
  IN EFI_HANDLE  ImageHandle
  );

/**
  Unified structure to hold macOS kernel boot arguments to make the code
  independent of their format version. Several values need changing
  by other libraries, so values are often pointers to original fields.
**/
typedef struct OC_BOOT_ARGUMENTS_ {
  UINT32  *MemoryMap;
  UINT32  *MemoryMapSize;
  UINT32  *MemoryMapDescriptorSize;
  UINT32  *MemoryMapDescriptorVersion;
  CHAR8   *CommandLine;
  UINT32  *DeviceTreeP;
  UINT32  *DeviceTreeLength;
  UINT32  *CsrActiveConfig;
} OC_BOOT_ARGUMENTS;

/**
  Parse macOS kernel into unified boot arguments structure.

  @param[out]  Arguments  Unified boot arguments structure.
  @param[in]   BootArgs   Kernel boot arguments strucutre.
**/
VOID
OcParseBootArgs (
  OUT OC_BOOT_ARGUMENTS *Arguments,
  IN  VOID              *BootArgs
  );

/**
  Check if boot argument is currently passed (via image options or NVRAM).

  @param[in]  LoadImage    UEFI loaded image protocol instance, optional.
  @param[in]  GetVariable  Preferred UEFI NVRAM reader, optional.
  @param[in]  Argument        Argument, e.g. -v, slide=, debug=, etc.
  @param[in]  ArgumentLength  Argument length, e.g. L_STR_LEN ("-v").

  @retval TRUE if argument is present.
**/
BOOLEAN
OcCheckArgumentFromEnv (
  IN EFI_LOADED_IMAGE  *LoadedImage  OPTIONAL,
  IN EFI_GET_VARIABLE  GetVariable  OPTIONAL,
  IN CONST CHAR8       *Argument,
  IN CONST UINTN       ArgumentLength
  );

/**
  Get argument value from command line.

  @param[in]  CommandLine     Argument command line, e.g. for boot.efi.
  @param[in]  Argument        Argument, e.g. -v, slide=, debug=, etc.
  @param[in]  ArgumentLength  Argument length, e.g. L_STR_LEN ("-v").

  @retval pointer to argument value or NULL.
**/
CONST CHAR8 *
OcGetArgumentFromCmd (
  IN CONST CHAR8  *CommandLine,
  IN CONST CHAR8  *Argument,
  IN CONST UINTN  ArgumentLength
  );

/**
  Remove argument from command line if present.

  @param[in, out] CommandLine  Argument command line, e.g. for boot.efi.
  @param[in]      Argument     Argument, e.g. -v, slide=, debug=, etc.
**/
VOID
OcRemoveArgumentFromCmd (
  IN OUT CHAR8        *CommandLine,
  IN     CONST CHAR8  *Argument
  );

/**
  Append argument to command line without deduplication.

  @param[in, out] Context         Picker context. NULL, if a privilege escalation is not required.
  @param[in, out] CommandLine     Argument command line of BOOT_LINE_LENGTH bytes.
  @param[in]      Argument        Argument, e.g. -v, slide=0, debug=0x100, etc.
  @param[in]      ArgumentLength  Argument length, e.g. L_STR_LEN ("-v").

  @retval TRUE on success.
**/
BOOLEAN
OcAppendArgumentToCmd (
  IN OUT OC_PICKER_CONTEXT  *Context OPTIONAL,
  IN OUT CHAR8              *CommandLine,
  IN     CONST CHAR8        *Argument,
  IN     CONST UINTN        ArgumentLength
  );

/**
  Perform NVRAM UEFI variable deletion.
**/
VOID
OcDeleteVariables (
  VOID
  );

#endif // OC_BOOT_MANAGEMENT_LIB_H
