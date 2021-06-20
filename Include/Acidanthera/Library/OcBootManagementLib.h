/** @file
  Copyright (C) 2019, vit9696. All rights reserved.<BR>
  Copyright (C) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_BOOT_MANAGEMENT_LIB_H
#define OC_BOOT_MANAGEMENT_LIB_H

#include <Uefi.h>
#include <IndustryStandard/AppleBootArgs.h>
#include <IndustryStandard/AppleHid.h>
#include <Library/OcAppleBootPolicyLib.h>
#include <Library/OcAppleKeyMapLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcStorageLib.h>
#include <Library/OcTypingLib.h>
#include <Protocol/AppleKeyMapAggregator.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/AppleBeepGen.h>
#include <Protocol/OcAudio.h>

#if defined(OC_TARGET_DEBUG) || defined(OC_TARGET_NOOPT)
//#define BUILTIN_DEMONSTRATE_TYPING
#endif

/**
  Primary picker context.
**/
typedef struct OC_PICKER_CONTEXT_ OC_PICKER_CONTEXT;

/**
  Picker keyboard handling context.
**/
typedef struct OC_HOTKEY_CONTEXT_ OC_HOTKEY_CONTEXT;

/**
  Default strings for use in the interfaces.
**/
#define OC_MENU_BOOT_MENU            L"OpenCore Boot Menu"
#define OC_MENU_RESET_NVRAM_ENTRY    L"Reset NVRAM"
#define OC_MENU_UEFI_SHELL_ENTRY     L"UEFI Shell"
#define OC_MENU_PASSWORD_REQUEST     L"Password: "
#define OC_MENU_PASSWORD_PROCESSING  L"Verifying password..."
#define OC_MENU_PASSWORD_RETRY_LIMIT L"Password retry limit exceeded."
#define OC_MENU_CHOOSE_OS            L"Choose the Operating System: "
#define OC_MENU_SHOW_AUXILIARY       L"Show Auxiliary"
#define OC_MENU_RELOADING            L"Reloading"
#define OC_MENU_TIMEOUT              L"Timeout"
#define OC_MENU_OK                   L"OK"
#define OC_MENU_EXTERNAL             L" (external)"
#define OC_MENU_DISK_IMAGE           L" (dmg)"
#define OC_MENU_SHUTDOWN             L"Shutting Down"
#define OC_MENU_RESTART              L"Restarting"
#define OC_MENU_SIP_IS_DISABLED      L"Toggle SIP (Disabled)"
#define OC_MENU_SIP_IS_ENABLED       L"Toggle SIP (Enabled)"

/**
  Predefined flavours.
**/
#define OC_FLAVOUR_AUTO                 "Auto"
#define OC_FLAVOUR_RESET_NVRAM          "ResetNVRAM:NVRAMTool"
#define OC_FLAVOUR_TOGGLE_SIP           "ToggleSIP:NVRAMTool"
#define OC_FLAVOUR_APPLE_OS             "Apple"
#define OC_FLAVOUR_APPLE_RECOVERY       "AppleRecv:Apple"
#define OC_FLAVOUR_APPLE_FW             "AppleRecv:Apple"
#define OC_FLAVOUR_APPLE_TIME_MACHINE   "AppleTM:Apple"
#define OC_FLAVOUR_WINDOWS              "Windows"
#define OC_FLAVOUR_OTHER_OS             "Other"

/**
  Predefined flavour ids.
**/
#define OC_FLAVOUR_ID_RESET_NVRAM       "ResetNVRAM"
#define OC_FLAVOUR_ID_UEFI_SHELL        "UEFIShell"

/**
  Paths allowed to be accessible by the interfaces.
**/
#define OPEN_CORE_IMAGE_PATH       L"Resources\\Image\\"
#define OPEN_CORE_LABEL_PATH       L"Resources\\Label\\"
#define OPEN_CORE_AUDIO_PATH       L"Resources\\Audio\\"
#define OPEN_CORE_FONT_PATH        L"Resources\\Font\\"

/**
  Attributes supported by the interfaces.
**/
#define OC_ATTR_USE_VOLUME_ICON          BIT0
#define OC_ATTR_USE_DISK_LABEL_FILE      BIT1
#define OC_ATTR_USE_GENERIC_LABEL_IMAGE  BIT2
#define OC_ATTR_HIDE_THEMED_ICONS        BIT3
#define OC_ATTR_USE_POINTER_CONTROL      BIT4
#define OC_ATTR_SHOW_DEBUG_DISPLAY       BIT5
#define OC_ATTR_USE_MINIMAL_UI           BIT6
#define OC_ATTR_USE_FLAVOUR_ICON         BIT7
#define OC_ATTR_ALL_BITS (\
  OC_ATTR_USE_VOLUME_ICON         | OC_ATTR_USE_DISK_LABEL_FILE | \
  OC_ATTR_USE_GENERIC_LABEL_IMAGE | OC_ATTR_HIDE_THEMED_ICONS   | \
  OC_ATTR_USE_POINTER_CONTROL     | OC_ATTR_SHOW_DEBUG_DISPLAY  | \
  OC_ATTR_USE_MINIMAL_UI          | OC_ATTR_USE_FLAVOUR_ICON )

/**
  Default timeout for IDLE timeout during menu picker navigation
  before VoiceOver toggle.
**/
#define OC_VOICE_OVER_IDLE_TIMEOUT_MS     700  ///< Experimental, less is problematic.

/**
  Default VoiceOver BeepGen protocol values.
**/
#define OC_VOICE_OVER_SIGNAL_NORMAL_MS    200  ///< From boot.efi, constant.
#define OC_VOICE_OVER_SILENCE_NORMAL_MS   150  ///< From boot.efi, constant.
#define OC_VOICE_OVER_SIGNALS_NORMAL      1    ///< Username prompt or any input for boot.efi
#define OC_VOICE_OVER_SIGNALS_PASSWORD    2    ///< Password prompt for boot.efi
#define OC_VOICE_OVER_SIGNALS_PASSWORD_OK 3    ///< Password correct for boot.efi

#define OC_VOICE_OVER_SIGNAL_ERROR_MS     1000
#define OC_VOICE_OVER_SILENCE_ERROR_MS    150
#define OC_VOICE_OVER_SIGNALS_ERROR       1    ///< Password verification error or boot failure.
#define OC_VOICE_OVER_SIGNALS_HWERROR     3    ///< Hardware error

/**
  Operating system boot type.
  This flags the inferred type, but it is not definitive and should not be relied upon for security.
**/
typedef UINT32 OC_BOOT_ENTRY_TYPE;

#define OC_BOOT_UNKNOWN             BIT0
#define OC_BOOT_APPLE_OS            BIT1
#define OC_BOOT_APPLE_RECOVERY      BIT2
#define OC_BOOT_APPLE_TIME_MACHINE  BIT3
#define OC_BOOT_APPLE_FW_UPDATE     BIT4
#define OC_BOOT_APPLE_ANY           (OC_BOOT_APPLE_OS | OC_BOOT_APPLE_RECOVERY | OC_BOOT_APPLE_TIME_MACHINE | OC_BOOT_APPLE_FW_UPDATE)
#define OC_BOOT_WINDOWS             BIT5
#define OC_BOOT_EXTERNAL_OS         BIT6
#define OC_BOOT_EXTERNAL_TOOL       BIT7
#define OC_BOOT_RESET_NVRAM         BIT8
#define OC_BOOT_TOGGLE_SIP          BIT9
#define OC_BOOT_SYSTEM              (OC_BOOT_RESET_NVRAM | OC_BOOT_TOGGLE_SIP)

/**
  Picker mode.
**/
typedef enum OC_PICKER_MODE_ {
  OcPickerModeBuiltin,
  OcPickerModeExternal,
  OcPickerModeApple,
} OC_PICKER_MODE;

/**
  macOS Kernel capabilities.
  Written in pairs of kernel and user capabilities.

  On IA32 firmware:
  10.4-10.5 - K32_U32 | K32_U64.
  10.6      - K32_U32 | K32_U64.
  10.7+     - K32_U64.

  On X64 firmware:
  10.4-10.5 - K32_U32 | K32_U64.
  10.6      - K32_U32 | K32_U64 | K64_U64.
  10.7+     - K32_U64 | K64_U64.
**/
#define OC_KERN_CAPABILITY_K32_U32        BIT0 ///< Supports K32 and U32 (10.4~10.6)
#define OC_KERN_CAPABILITY_K32_U64        BIT1 ///< Supports K32 and U64 (10.4~10.7)
#define OC_KERN_CAPABILITY_K64_U64        BIT2 ///< Supports K64 and U64 (10.6+)

#define OC_KERN_CAPABILITY_K32_K64_U64    (OC_KERN_CAPABILITY_K32_U64 | OC_KERN_CAPABILITY_K64_U64)
#define OC_KERN_CAPABILITY_K32_U32_U64    (OC_KERN_CAPABILITY_K32_U32 | OC_KERN_CAPABILITY_K32_U64)
#define OC_KERN_CAPABILITY_ALL            (OC_KERN_CAPABILITY_K32_U32 | OC_KERN_CAPABILITY_K32_K64_U64)

/**
  Action to perform as part of executing a system boot entry.
**/
typedef
EFI_STATUS
(*OC_BOOT_SYSTEM_ACTION) (
  VOID
  );

/**
  Discovered boot entry.
  Note, inner resources must be freed with OcResetBootEntry.
**/
typedef struct OC_BOOT_ENTRY_ {
  //
  // Link in entry list in OC_BOOT_FILESYSTEM.
  //
  LIST_ENTRY                Link;
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
  // Content flavour.
  //
  CHAR8                     *Flavour;
  //
  // Heuristical value signaling inferred type of booted os.
  // WARNING: Non-definitive, do not rely on for any security purposes.
  //
  OC_BOOT_ENTRY_TYPE        Type;
  //
  // Entry index number, assigned by picker.
  //
  UINT32                    EntryIndex;
  //
  // Set when this entry is an externally available entry (e.g. USB).
  //
  BOOLEAN                   IsExternal;
  //
  // Should try booting from first dmg found in DevicePath.
  //
  BOOLEAN                   IsFolder;
  //
  // Set when this entry refers to a generic booter (e.g. BOOTx64.EFI).
  //
  BOOLEAN                   IsGeneric;
  //
  // Set when this entry refers to a custom boot entry.
  //
  BOOLEAN                   IsCustom;
  //
  // Should make this option default boot option.
  //
  BOOLEAN                   SetDefault;
  //
  // Should launch this entry in text mode.
  //
  BOOLEAN                   LaunchInText;
  //
  // Should expose real device path when dealing with custom entries.
  //
  BOOLEAN                   ExposeDevicePath;
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
  Boot filesystem containing boot entries.
**/
typedef struct OC_BOOT_FILESYSTEM_ OC_BOOT_FILESYSTEM;
struct OC_BOOT_FILESYSTEM_ {
  //
  // Link in filesystem list in OC_BOOT_CONTEXT.
  //
  LIST_ENTRY           Link;
  //
  // Filesystem handle.
  //
  EFI_HANDLE           Handle;
  //
  // List of boot entries (OC_BOOT_ENTRY).
  //
  LIST_ENTRY           BootEntries;
  //
  // Pointer to APFS Recovery partition (if any).
  //
  OC_BOOT_FILESYSTEM   *RecoveryFs;
  //
  // External filesystem.
  //
  BOOLEAN              External;
  //
  // Loader filesystem.
  //
  BOOLEAN              LoaderFs;
  //
  // Contains recovery on the filesystem.
  //
  BOOLEAN              HasSelfRecovery;
};

/**
  Boot context containing boot filesystems.
**/
typedef struct OC_BOOT_CONTEXT_ {
  //
  // Total boot entry count.
  //
  UINTN                       BootEntryCount;
  //
  // Total filesystem count.
  //
  UINTN                       FileSystemCount;
  //
  // List of filesystems containing boot entries (OC_BOOT_FILESYSTEM).
  //
  LIST_ENTRY                  FileSystems;
  //
  // GUID namespace for boot entries.
  //
  EFI_GUID                    *BootVariableGuid;
  //
  // Default entry to be booted.
  //
  OC_BOOT_ENTRY               *DefaultEntry;
  //
  // Picker context for externally configured parameters.
  //
  OC_PICKER_CONTEXT           *PickerContext;
} OC_BOOT_CONTEXT;

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
  Allow scanning PCI devices (e.g. virtio).
**/
#define OC_SCAN_ALLOW_DEVICE_PCI         BIT24

/**
  All device bits used by OC_SCAN_DEVICE_LOCK.
**/
#define OC_SCAN_DEVICE_BITS ( \
  OC_SCAN_ALLOW_DEVICE_SATA     | OC_SCAN_ALLOW_DEVICE_SASEX | \
  OC_SCAN_ALLOW_DEVICE_SCSI     | OC_SCAN_ALLOW_DEVICE_NVME  | \
  OC_SCAN_ALLOW_DEVICE_ATAPI    | OC_SCAN_ALLOW_DEVICE_USB   | \
  OC_SCAN_ALLOW_DEVICE_FIREWIRE | OC_SCAN_ALLOW_DEVICE_SDCARD | \
  OC_SCAN_ALLOW_DEVICE_PCI)

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
  OC_SCAN_ALLOW_FS_APFS | \
  OC_SCAN_ALLOW_DEVICE_SATA  | OC_SCAN_ALLOW_DEVICE_SASEX | \
  OC_SCAN_ALLOW_DEVICE_SCSI  | OC_SCAN_ALLOW_DEVICE_NVME | \
  OC_SCAN_ALLOW_DEVICE_PCI)

/**
  OcLoadBootEntry DMG loading policy rules.
**/
typedef enum {
  OcDmgLoadingDisabled,
  OcDmgLoadingAnyImage,
  OcDmgLoadingAppleSigned,
} OC_DMG_LOADING_SUPPORT;

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
  OUT CHAR16                      **ExitData    OPTIONAL,
  IN  BOOLEAN                     LaunchInText
  );

/**
  Exposed custom entry load interface.
  Returns allocated file buffer from pool on success.
**/
typedef
EFI_STATUS
(EFIAPI *OC_CUSTOM_READ) (
  IN  OC_STORAGE_CONTEXT          *Storage,
  IN  OC_BOOT_ENTRY               *ChosenEntry,
  OUT VOID                        **Data,
  OUT UINT32                      *DataSize,
  OUT EFI_DEVICE_PATH_PROTOCOL    **DevicePath,
  OUT EFI_HANDLE                  *StorageHandle,
  OUT EFI_DEVICE_PATH_PROTOCOL    **StoragePath
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
  //
  // Content flavour.
  //
  CONST CHAR8  *Flavour;
  //
  // Whether this entry is auxiliary.
  //
  BOOLEAN      Auxiliary;
  //
  // Whether this entry is a tool.
  //
  BOOLEAN      Tool;
  //
  // Whether it should be started in text mode.
  //
  BOOLEAN      TextMode;
  //
  // Whether we should pass the actual device path (if possible).
  //
  BOOLEAN      RealPath;
} OC_PICKER_ENTRY;

/**
  Privilege levels to escalate to
**/
typedef enum {
  OcPrivilegeUnauthorized = 0,
  OcPrivilegeAuthorized   = 1
} OC_PRIVILEGE_LEVEL;

/**
  OC picker codes.
**/
typedef INTN                    OC_KEY_CODE;

/**
  OC picker modifiers.
**/
typedef UINT16                  OC_MODIFIER_MAP;

/**
  OC picker modifiers.
**/
typedef UINTN                   OC_PICKER_KEY_MAP;

/**
  Full picker key info.
  Note: Typing is 'orthogonal' to actions, and the presence or absence of a next
  typing key should be detected by UnicodeChar != CHAR_NULL.
**/
typedef struct {
  OC_KEY_CODE         OcKeyCode;
  OC_MODIFIER_MAP     OcModifiers;
  CHAR16              UnicodeChar;
} OC_PICKER_KEY_INFO;

/**
  Request a privilege escalation, for example by prompting for a password.
**/
typedef
EFI_STATUS
(EFIAPI *OC_REQ_PRIVILEGE) (
  IN OC_PICKER_CONTEXT   *Context,
  IN OC_PRIVILEGE_LEVEL  Level
  );

/**
  Display entries onscreen.
**/
typedef
EFI_STATUS
(EFIAPI *OC_SHOW_MENU) (
  IN  OC_BOOT_CONTEXT             *BootContext,
  IN  OC_BOOT_ENTRY               **BootEntries,
  OUT OC_BOOT_ENTRY               **ChosenBootEntry
  );

/**
  Get label contents (e.g. '.disk_label' or '.disk_label_2x').
**/
typedef
EFI_STATUS
(EFIAPI *OC_GET_ENTRY_LABEL_IMAGE) (
  IN  OC_PICKER_CONTEXT          *Context,
  IN  OC_BOOT_ENTRY              *BootEntry,
  IN  UINT8                      Scale,
  OUT VOID                       **ImageData,
  OUT UINT32                     *DataLength
  );

/**
  Get icon contents (e.g. '.VolumeIcon.icns').
**/
typedef
EFI_STATUS
(EFIAPI *OC_GET_ENTRY_ICON) (
  IN  OC_PICKER_CONTEXT          *Context,
  IN  OC_BOOT_ENTRY              *BootEntry,
  OUT VOID                       **ImageData,
  OUT UINT32                     *DataLength
  );

/**
  Get picker pressed key info.
**/
typedef
VOID
(EFIAPI *OC_GET_KEY_INFO) (
  IN OUT OC_PICKER_CONTEXT                  *Context,
  IN     OC_PICKER_KEY_MAP                  KeyFilter,
     OUT OC_PICKER_KEY_INFO                 *PickerKeyInfo
  );

/**
  Request end time in units appropriate for OC_WAIT_FOR_KEY_INFO.
**/
typedef
UINT64
(EFIAPI *OC_GET_KEY_WAIT_END_TIME) (
  IN UINT64     Timeout
  );

/**
  Wait for picker pressed key info. Use zero EndTime for no timeout.
**/
typedef
BOOLEAN
(EFIAPI *OC_WAIT_FOR_KEY_INFO) (
  IN OUT OC_PICKER_CONTEXT                  *Context,
  IN     UINT64                             EndTime,
  IN     OC_PICKER_KEY_MAP                  KeyFilter,
  IN OUT OC_PICKER_KEY_INFO                 *PickerKeyInfo
  );

/**
  Flush picker typing buffer.
**/
typedef
VOID
(EFIAPI *OC_FLUSH_TYPING_BUFFER) (
  IN OUT OC_PICKER_CONTEXT                  *Context
  );

/**
  Play audio file for context.
**/
typedef
EFI_STATUS
(EFIAPI *OC_PLAY_AUDIO_FILE) (
  IN  OC_PICKER_CONTEXT  *Context,
  IN  UINT32             File,
  IN  BOOLEAN            Fallback
  );

/**
  Generate cycles of beep signals for context with silence afterwards, blocking.
**/
typedef
EFI_STATUS
(EFIAPI *OC_PLAY_AUDIO_BEEP) (
  IN  OC_PICKER_CONTEXT        *Context,
  IN  UINT32                   ToneCount,
  IN  UINT32                   ToneLength,
  IN  UINT32                   SilenceLength
  );

/**
  Play audio entry for context.
**/
typedef
EFI_STATUS
(EFIAPI *OC_PLAY_AUDIO_ENTRY) (
  IN  OC_PICKER_CONTEXT  *Context,
  IN  OC_BOOT_ENTRY      *Entry
  );

/**
  Toggle VoiceOver support.
**/
typedef
VOID
(EFIAPI *OC_TOGGLE_VOICE_OVER) (
  IN  OC_PICKER_CONTEXT  *Context,
  IN  UINT32             File  OPTIONAL
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
  Instrument kb loop delay.

  @param[in]      LoopDelayStart    Delay start in TSC asm ticks.
  @param[in]      LoopDelayEnd      Delay end in TSC asm ticks. 
**/
typedef
VOID
(EFIAPI *OC_KB_DEBUG_INSTRUMENT_LOOP_DELAY) (
  UINT64 LoopDelayStart,
  UINT64 LoopDelayEnd
  );

/**
  Running display of held keys.

  @param[in]      NumKeysDown     Number of keys that went down.
  @param[in]      NumKeysHeld     Number of keys held.
  @param[in]      Modifiers       Key modifiers.
**/
typedef
VOID
(EFIAPI *OC_KB_DEBUG_SHOW) (
  UINTN                     NumKeysDown,
  UINTN                     NumKeysHeld,
  APPLE_MODIFIER_MAP        Modifiers
  );

typedef struct {
  OC_KB_DEBUG_INSTRUMENT_LOOP_DELAY  InstrumentLoopDelay;
  OC_KB_DEBUG_SHOW                   Show;
} OC_KB_DEBUG_CALLBACKS;

typedef struct {
  OC_PRIVILEGE_LEVEL CurrentLevel;
  CONST UINT8        *Salt;
  UINT32             SaltSize;
  CONST UINT8        *Hash;
} OC_PRIVILEGE_CONTEXT;

/**
  Password verification.
**/
typedef
BOOLEAN
(EFIAPI *OC_VERIFY_PASSWORD) (
  IN CONST UINT8                  *Password,
  IN UINT32                       PasswordSize,
  IN CONST OC_PRIVILEGE_CONTEXT   *PrivilegeContext
  );

/**
  Boot picker context describing picker behaviour.
**/
struct OC_PICKER_CONTEXT_ {
  //
  // Scan policy (e.g. OC_SCAN_DEFAULT_POLICY).
  //
  UINT32                     ScanPolicy;
  //
  // DMG loading mode (e.g. OcDmgLoadingAppleSigned).
  //
  OC_DMG_LOADING_SUPPORT     DmgLoading;
  //
  // Default entry selection timeout (pass 0 to ignore).
  //
  UINT32                     TimeoutSeconds;
  //
  // Default delay prior to handling hotkeys (pass 0 to ignore).
  //
  UINT32                     TakeoffDelay;
  //
  // Define picker behaviour.
  // For example, show boot menu or just boot the default option.
  //
  OC_PICKER_CMD              PickerCommand;
  //
  // Use custom (gOcVendorVariableGuid) for Boot#### variables.
  //
  BOOLEAN                    CustomBootGuid;
  //
  // Custom entry reading routine, optional for no custom entries.
  //
  OC_CUSTOM_READ             CustomRead;
  //
  // Storage context.
  //
  OC_STORAGE_CONTEXT         *StorageContext;
  //
  // Image starting routine used, required.
  //
  OC_IMAGE_START             StartImage;
  //
  // Handle to perform loader detection, optional.
  //
  EFI_HANDLE                 LoaderHandle;
  //
  // Get entry label image.
  //
  OC_GET_ENTRY_LABEL_IMAGE   GetEntryLabelImage;
  //
  // Get entry icon.
  //
  OC_GET_ENTRY_ICON          GetEntryIcon;
  //
  // Entry display routine.
  //
  OC_SHOW_MENU               ShowMenu;
  //
  // Privilege escalation requesting routine.
  //
  OC_REQ_PRIVILEGE           RequestPrivilege;
  //
  // Password verification.
  //
  OC_VERIFY_PASSWORD         VerifyPassword;
  //
  // Picker typing context.
  //
  OC_HOTKEY_CONTEXT          *HotKeyContext;
  //
  // Keyboard debug methods.
  //
  OC_KB_DEBUG_CALLBACKS      *KbDebug;
  //
  // Context to pass to RequestPrivilege, optional.
  //
  OC_PRIVILEGE_CONTEXT       *PrivilegeContext;
  //
  // Additional suffix to include by the interface.
  //
  CONST CHAR8                *TitleSuffix;
  //
  // Used picker mode.
  //
  OC_PICKER_MODE             PickerMode;
  //
  // Console attributes. 0 is reserved as disabled.
  //
  UINT32                     ConsoleAttributes;
  //
  // Picker attribues:
  // - BIT0~BIT15  are OpenCore reserved.
  // - BIT16~BIT31 are OEM-specific.
  //
  UINT32                     PickerAttributes;
  //
  // Picker icon set variant (refer to docs for requested behaviour).
  //
  CONST CHAR8                *PickerVariant;
  //
  // Enable polling boot arguments.
  //
  BOOLEAN                    PollAppleHotKeys;
  //
  // Append the "Reset NVRAM" option to the boot entry list.
  //
  BOOLEAN                    ShowNvramReset;
  //
  // Append toggle SIP option to the boot entry list.
  //
  BOOLEAN                    ShowToggleSip;
  //
  // Allow setting default boot option from boot menu.
  //
  BOOLEAN                    AllowSetDefault;
  //
  // Hide and do not scan auxiliary entries.
  //
  BOOLEAN                    HideAuxiliary;
  //
  // Enable audio assistant during picker playback.
  //
  BOOLEAN                    PickerAudioAssist;
  //
  // Set when Apple picker cannot be used on this system.
  //
  BOOLEAN                    ApplePickerUnsupported;
  //
  // Ignore Apple peripheral firmware updates.
  //
  BOOLEAN                    BlacklistAppleUpdate;
  //
  // Recommended audio protocol, optional.
  //
  OC_AUDIO_PROTOCOL          *OcAudio;
  //
  // Recommended beeper protocol, optional.
  //
  APPLE_BEEP_GEN_PROTOCOL    *BeepGen;
  //
  // Play audio file function.
  //
  OC_PLAY_AUDIO_FILE         PlayAudioFile;
  //
  // Play audio beep function.
  //
  OC_PLAY_AUDIO_BEEP         PlayAudioBeep;
  //
  // Play audio entry function.
  //
  OC_PLAY_AUDIO_ENTRY        PlayAudioEntry;
  //
  // Toggle VoiceOver function.
  //
  OC_TOGGLE_VOICE_OVER       ToggleVoiceOver;
  //
  // Recovery initiator if present.
  //
  EFI_DEVICE_PATH_PROTOCOL   *RecoveryInitiator;
  //
  // Custom boot order updated during scanning allocated from pool.
  // Preserved here to avoid situations with losing BootNext on rescan.
  //
  UINT16                     *BootOrder;
  //
  // Number of entries in boot order.
  //
  UINTN                      BootOrderCount;
  //
  // Additional boot arguments for Apple loaders.
  //
  CHAR8                      AppleBootArgs[BOOT_LINE_LENGTH];
  //
  // Number of custom boot paths (bless override).
  //
  UINTN                      NumCustomBootPaths;
  //
  // Custom boot paths (bless override).  Must start with '\'.
  //
  CHAR16                     **CustomBootPaths;
  //
  // Number of absolute custom entries.
  //
  UINT32                     AbsoluteEntryCount;
  //
  // Number of total custom entries (absolute and tools).
  //
  UINT32                     AllCustomEntryCount;
  //
  // Custom picker entries.  Absolute entries come first.
  //
  OC_PICKER_ENTRY            CustomEntries[];
};

/**
  Boot picker keyboard handling context.
**/
struct OC_HOTKEY_CONTEXT_ {
  //
  // Get pressed key info.
  //
  OC_GET_KEY_INFO            GetKeyInfo;
  //
  // Request end time in units appropriate for WaitForKeyInfo.
  //
  OC_GET_KEY_WAIT_END_TIME   GetKeyWaitEndTime;
  //
  // Wait for pressed key info.
  //
  OC_WAIT_FOR_KEY_INFO       WaitForKeyInfo;
  //
  // Flush typing buffer.
  //
  OC_FLUSH_TYPING_BUFFER     FlushTypingBuffer;
  //
  // Apple Key Map protocol.
  //
  APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap;
  //
  // Non-repeating key context.
  //
  OC_KEY_REPEAT_CONTEXT      *DoNotRepeatContext;
  //
  // Typing context.
  //
  OC_TYPING_CONTEXT          *TypingContext;
};

/**
  Hibernate detection bit mask for hibernate source usage.
**/
#define HIBERNATE_MODE_NONE   0U
#define HIBERNATE_MODE_RTC    1U
#define HIBERNATE_MODE_NVRAM  2U

/**
  Get '.disk_label' or '.disk_label_2x' file contents, if exists.

  @param[in]   BootEntry      Located boot entry.
  @param[in]   Scale          User interface scale.
  @param[out]  ImageData      File contents.
  @param[out]  DataLength     File length.

  @retval EFI_SUCCESS   The file was read successfully.
**/
EFI_STATUS
EFIAPI
OcGetBootEntryLabelImage (
  IN  OC_PICKER_CONTEXT          *Context,
  IN  OC_BOOT_ENTRY              *BootEntry,
  IN  UINT8                      Scale,
  OUT VOID                       **ImageData,
  OUT UINT32                     *DataLength
  );

/**
  Get '.VolumeIcon.icns' file contents, if exists.

  @param[in]   BootEntry      Located boot entry.
  @param[out]  ImageData      File contents.
  @param[out]  DataLength     File length.

  @retval EFI_SUCCESS   The file was read successfully.
**/
EFI_STATUS
EFIAPI
OcGetBootEntryIcon (
  IN  OC_PICKER_CONTEXT          *Context,
  IN  OC_BOOT_ENTRY              *BootEntry,
  OUT VOID                       **ImageData,
  OUT UINT32                     *DataLength
  );

/**
  Scan system for boot entries.

  @param[in]  Context  Picker context.

  @retval boot context allocated from pool.
**/
OC_BOOT_CONTEXT *
OcScanForBootEntries (
  IN  OC_PICKER_CONTEXT  *Context
  );

/**
  Scan system for first entry to boot.
  This is likely to return an incomplete list and can even give NULL,
  when only tools and system entries are present.

  @param[in]  Context  Picker context.

  @retval boot context allocated from pool.
**/
OC_BOOT_CONTEXT *
OcScanForDefaultBootEntry (
  IN  OC_PICKER_CONTEXT  *Context
  );

/**
  Perform boot entry enumeration.

  @param[in]  BootContext    Boot context.

  @retval enumerated boot entry list allocated from pool.
**/
OC_BOOT_ENTRY  **
OcEnumerateEntries (
  IN  OC_BOOT_CONTEXT  *BootContext
  );

/**
  Free boot context.

  @param[in,out]  Context    Boot context to free.
**/
VOID
OcFreeBootContext (
  IN OUT OC_BOOT_CONTEXT  *Context
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

/**
  Set default entry to passed entry.

  @param[in]      Context          Picker context.
  @param[in,out]  Entry            Entry to make default.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcSetDefaultBootEntry (
  IN OC_PICKER_CONTEXT  *Context,
  IN OC_BOOT_ENTRY      *Entry
  );

/**
  Show simple password prompt and return verification status.

  @param[in]  Context          Picker context.
  @param[in]  Level            The privilege level to request escalating to.

  @retval EFI_SUCCESS  The privilege level has been escalated successfully.
  @retval EFI_ABORTED  The privilege escalation has been aborted.
  @retval other        The system must be considered compromised.

**/
EFI_STATUS
EFIAPI
OcShowSimplePasswordRequest (
  IN OC_PICKER_CONTEXT      *Context,
  IN OC_PRIVILEGE_LEVEL     Level
  );

/**
  Verify password.

  Shared context function to be used by all pickers rather than directly linked call
  to OcVerifyPasswordSha512, to pick up status of Avx acceleration as enabled within
  OpenCore.efi and to avoid unnecessary OcCryptoLib lib linking into external picker.


  @param[in]  Password          Password.
  @param[in]  PasswordSize      Password size.
  @param[in]  PrivilegeContext  Privilege context.

  @retval                       True if password verified successfully.
**/
BOOLEAN
EFIAPI
OcVerifyPassword (
  IN CONST UINT8                  *Password,
  IN UINT32                       PasswordSize,
  IN CONST OC_PRIVILEGE_CONTEXT   *PrivilegeContext
  );

/**
  Show simple boot entry selection menu and return chosen entry.

  @param[in]  BootContext      Boot context.
  @param[in]  BootEntries      Enumerated entries.
  @param[in]  ChosenBootEntry  Chosen boot entry from BootEntries on success.

  @retval EFI_SUCCESS          Executed successfully and picked up an entry.
  @retval EFI_ABORTED          When the user chose to by pressing Esc or 0.
**/
EFI_STATUS
EFIAPI
OcShowSimpleBootMenu (
  IN  OC_BOOT_CONTEXT             *BootContext,
  IN  OC_BOOT_ENTRY               **BootEntries,
  OUT OC_BOOT_ENTRY               **ChosenBootEntry
  );

/**
  Load & start boot entry loader image with given options.

  @param[in]  Context        Picker context.
  @param[in]  BootEntry      Located boot entry.
  @param[in]  ParentHandle   Parent image handle.

  @retval EFI_SUCCESS        The image was found, started, and ended succesfully.
**/
EFI_STATUS
OcLoadBootEntry (
  IN  OC_PICKER_CONTEXT  *Context,
  IN  OC_BOOT_ENTRY      *BootEntry,
  IN  EFI_HANDLE         ParentHandle
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
  Handle recovery detection for later loading.
  Recovery handling is required to choose the right operating system.

  @param[out]  Initiator  Recovery initiator device path, optional.

  @retval EFI_SUCCESS        Recovery boot is required.
  @retval EFI_NOT_FOUND      System should boot normally.
**/
EFI_STATUS
OcHandleRecoveryRequest (
  OUT EFI_DEVICE_PATH_PROTOCOL  **Initiator  OPTIONAL
  );

/**
  Read and expand Apple panic log if present.

  @param[out]  PanicSize    Size of the panic log on success.

  @retval panic buffer on success.
  @retval NULL on failure.
**/
VOID *
OcReadApplePanicLog (
  OUT UINT32       *PanicSize
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
  Key index mappings.
  Non-negative values may also be returned to request a specific zero-indexed boot entry.
**/
#define OC_INPUT_STR                  "123456789ABCDEFGHIJKLMNOPQRSTUVXWZ"
#define OC_INPUT_MAX                  L_STR_LEN (OC_INPUT_STR)
#define OC_INPUT_ABORTED              -1        ///< Esc or 0
#define OC_INPUT_NO_ACTION            -2        ///< Some other key
#define OC_INPUT_TIMEOUT              -3        ///< Timeout
#define OC_INPUT_CONTINUE             -4        ///< Continue (press enter)
#define OC_INPUT_UP                   -5        ///< Move up
#define OC_INPUT_DOWN                 -6        ///< Move down
#define OC_INPUT_LEFT                 -7        ///< Move left
#define OC_INPUT_RIGHT                -8        ///< Move right
#define OC_INPUT_TOP                  -9        ///< Move to top
#define OC_INPUT_BOTTOM               -10       ///< Move to bottom
#define OC_INPUT_MORE                 -11       ///< Show more entries (press space)
#define OC_INPUT_VOICE_OVER           -12       ///< Toggle VoiceOver (press CMD+F5)
#define OC_INPUT_INTERNAL             -13       ///< Accepted internal hotkey (e.g. Apple)
#define OC_INPUT_TYPING_CLEAR_ALL     -14       ///< Clear current input while typing (press esc)
#define OC_INPUT_TYPING_BACKSPACE     -15       ///< Clear last typed character while typing (press backspace)
#define OC_INPUT_TYPING_LEFT          -16       ///< Move left while typing (UI does not have to support)
#define OC_INPUT_TYPING_RIGHT         -17       ///< Move right while typing (UI does not have to support)
#define OC_INPUT_TYPING_CONFIRM       -18       ///< Confirm input while typing (press enter)
#define OC_INPUT_SWITCH_FOCUS         -19       ///< Switch UI focus (tab and shift+tab)
#define OC_INPUT_FUNCTIONAL(x)        (-50 - (x))  ///< Function hotkeys

/**
  Modifier mappings.
**/
#define OC_MODIFIERS_NONE                     0
#define OC_MODIFIERS_SET_DEFAULT              BIT0
#define OC_MODIFIERS_REVERSE_SWITCH_FOCUS     BIT1

#define OC_PICKER_KEYS_TYPING                 BIT0
#define OC_PICKER_KEYS_HOTKEYS                BIT1
#define OC_PICKER_KEYS_VOICE_OVER             BIT2
#define OC_PICKER_KEYS_TAB_CONTROL            BIT3

#define OC_PICKER_KEYS_FOR_TYPING   \
  (OC_PICKER_KEYS_TYPING | OC_PICKER_KEYS_VOICE_OVER | OC_PICKER_KEYS_TAB_CONTROL)

#define OC_PICKER_KEYS_FOR_PICKER   \
  (OC_PICKER_KEYS_HOTKEYS | OC_PICKER_KEYS_VOICE_OVER | OC_PICKER_KEYS_TAB_CONTROL)

/**
  Initialise picker keyboard handling.
  Initialises necessary handlers and updates booter context based on this.
  Call before looped calls to OcWaitForPickerKeyInfo or OcGetPickerKeyInfo.

  @param[in,out]  Context       Picker context.

  @retval EFI_SUCCESS           The keyboard handling within the context has been initialised.
  @retval EFI_NOT_FOUND         Could not find a required protocol.
  @retval other                 An error returned by a sub-operation.
**/
EFI_STATUS
OcInitHotKeys (
  IN OUT OC_PICKER_CONTEXT  *Context
  );

/**
  Free picker keyboard handling resources.

  @param[in]      Context       Picker context.
**/
VOID
OcFreeHotKeys (
  IN     OC_PICKER_CONTEXT  *Context
  );

/**
  Install missing boot policy, scan, and show simple boot menu.

  @param[in]  Context       Picker context.

  @retval does not return unless a fatal error happened.
**/
EFI_STATUS
OcRunBootPicker (
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
  Check if supplied device path contains known names (e.g. Apple bootloader).

  @param[in]   DevicePath        Device path.
  @param[out]  IsFolder          Device path represents directory, optional.
  @param[out]  IsGeneric         Device path represents generic booter, optional.

  @retval entry type for potentially known bootloaders.
  @retval OC_BOOT_UNKNOWN for unknown bootloaders.
**/
OC_BOOT_ENTRY_TYPE
OcGetBootDevicePathType (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT BOOLEAN                   *IsFolder   OPTIONAL,
  OUT BOOLEAN                   *IsGeneric  OPTIONAL
  );

/**
  Get loaded image protocol for Apple bootloader.

  @param[in]  ImageHandle        Image handle.

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
  UINT32            *MemoryMap;
  UINT32            *MemoryMapSize;
  UINT32            *MemoryMapDescriptorSize;
  UINT32            *MemoryMapDescriptorVersion;
  CHAR8             *CommandLine;
  UINT32            *KernelAddrP;
  UINT32            *SystemTableP;
  UINT32            *RuntimeServicesPG;
  UINT64            *RuntimeServicesV;
  UINT32            *DeviceTreeP;
  UINT32            *DeviceTreeLength;
  UINT32            *CsrActiveConfig;
  EFI_SYSTEM_TABLE  *SystemTable;
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

  @param[in]     LoadedImage      UEFI loaded image protocol instance, optional.
  @param[in]     GetVariable      Preferred UEFI NVRAM reader, optional.
  @param[in]     Argument         Argument, e.g. -v, slide=, debug=, etc.
  @param[in]     ArgumentLength   Argument length, e.g. L_STR_LEN ("-v").
  @param[in,out] Value            Argument value allocated from pool.

  @retval TRUE if argument is present.
**/
BOOLEAN
OcCheckArgumentFromEnv (
  IN     EFI_LOADED_IMAGE   *LoadedImage OPTIONAL,
  IN     EFI_GET_VARIABLE   GetVariable OPTIONAL,
  IN     CONST CHAR8        *Argument,
  IN     CONST UINTN        ArgumentLength,
  IN OUT CHAR8              **Value OPTIONAL
  );

/**
  Get argument value from command line.

  @param[in]  CommandLine     Argument command line, e.g. for boot.efi.
  @param[in]  Argument        Argument, e.g. -v, slide=, debug=, etc.
  @param[in]  ArgumentLength  Argument length, e.g. L_STR_LEN ("-v").
  @param[out] ValueLength     Argument value length, optional.

  @retval pointer to argument value or NULL.
**/
CONST CHAR8 *
OcGetArgumentFromCmd (
  IN  CONST CHAR8   *CommandLine,
  IN  CONST CHAR8   *Argument,
  IN  CONST UINTN   ArgumentLength,
  OUT UINTN         *ValueLength OPTIONAL
  );

/**
  Remove argument from command line if present.

  @param[in,out]  CommandLine  Argument command line, e.g. for boot.efi.
  @param[in]      Argument     Argument, e.g. -v, slide=, debug=, etc.
**/
VOID
OcRemoveArgumentFromCmd (
  IN OUT CHAR8        *CommandLine,
  IN     CONST CHAR8  *Argument
  );

/**
  Append argument to command line without deduplication.

  @param[in,out]  Context         Picker context. NULL, if a privilege escalation is not required.
  @param[in,out]  CommandLine     Argument command line of BOOT_LINE_LENGTH bytes.
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
  Append 1 or more arguments to Loaded Image protocol.

  @param[in,out]  LoadedImage    Loaded Image protocol instance.
  @param[in]      Arguments      Argument array.
  @param[in]      ArgumentCount  Number of arguments in the array.
  @param[in]      Replace        Whether to append to existing arguments or replace.

  @retval TRUE on success.
**/
BOOLEAN
OcAppendArgumentsToLoadedImage (
  IN OUT EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage,
  IN     CONST CHAR8                **Arguments,
  IN     UINT32                     ArgumentCount,
  IN     BOOLEAN                    Replace
  );

/**
  Get current SIP setting.

  @param[out]     CsrActiveConfig    Returned csr-active-config variable; uninitialised if variable
                                     not found, or other error.
  @param[out]     Attributes         If not NULL, a pointer to the memory location to return the
                                     attributes bitmask for the variable; uninitialised if variable
                                     not found, or other error.

  @retval EFI_SUCCESS, EFI_NOT_FOUND, or other error returned by called code.
**/
EFI_STATUS
OcGetSip (
  OUT UINT32 *CsrActiveConfig,
  OUT UINT32 *Attributes          OPTIONAL
  );

/**
  Set current SIP setting.

  @param[in]      CsrActiveConfig    csr-active-config value to set, or NULL to clear the variable.
  @param[in]      Attributes         Attributes to apply.

  @retval EFI_SUCCESS, EFI_NOT_FOUND, or other error returned by called code.
**/
EFI_STATUS
OcSetSip (
  IN  UINT32 *CsrActiveConfig,
  IN  UINT32 Attributes
  );

/**
  Is SIP enabled?

  @param[in]      GetStatus          Return status from previous OcGetSip or gRT->GetVariable call.
  @param[in]      CsrActiveConfig    csr-active-config value from previous OcGetSip or gRT->GetVariable call.
                                     This value is never used unless GetStatus is EFI_SUCCESS.

  @retval TRUE if SIP should be considered enabled based on the passed values.
**/
BOOLEAN
OcIsSipEnabled (
  IN  EFI_STATUS  GetStatus,
  IN  UINT32      CsrActiveConfig
  );

/**
  Toggle SIP.

  @param[in]      CsrActiveConfig    The csr-active-config value to use to disable SIP, if it was previously enabled.

  @retval TRUE on successful operation.
**/
EFI_STATUS
OcToggleSip (
  IN  UINT32 CsrActiveConfig
  );

/**
  Perform NVRAM UEFI variable deletion.
**/
VOID
OcDeleteVariables (
  VOID
  );

/**
  Launch firmware application.

  @param[in] ApplicationGuid  Application GUID identifier in the firmware.
  @param[in] SetReason        Pass enter reason (specific to Apple BootPicker).

  @retval error code, should not return. 
**/
EFI_STATUS
OcRunFirmwareApplication (
  IN EFI_GUID  *ApplicationGuid,
  IN BOOLEAN   SetReason
  );

/**
  Play audio file for context.

  @param[in]  Context   Picker context.
  @param[in]  File      File to play.
  @param[in]  Fallback  Try to fallback to beeps on failure.

  @retval EFI_SUCCESS on success or when unnecessary.
**/
EFI_STATUS
EFIAPI
OcPlayAudioFile (
  IN  OC_PICKER_CONTEXT  *Context,
  IN  UINT32             File,
  IN  BOOLEAN            Fallback
  );

/**
  Generate cycles of beep signals for context with silence afterwards, blocking.

  @param[in] Context        Picker context.
  @param[in] ToneCount      Number of signals to produce.
  @param[in] ToneLength     Signal length in milliseconds.
  @param[in] SilenceLength  Silence length in milliseconds.

  @retval EFI_SUCCESS on success or when unnecessary.
**/
EFI_STATUS
EFIAPI
OcPlayAudioBeep (
  IN  OC_PICKER_CONTEXT        *Context,
  IN  UINT32                   ToneCount,
  IN  UINT32                   ToneLength,
  IN  UINT32                   SilenceLength
  );

/**
  Play audio entry for context.

  @param[in]  Context   Picker context.
  @param[in]  Entry     Entry to play.

  @retval EFI_SUCCESS on success or when unnecessary.
**/
EFI_STATUS
EFIAPI
OcPlayAudioEntry (
  IN  OC_PICKER_CONTEXT  *Context,
  IN  OC_BOOT_ENTRY      *Entry
  );

/**
  Toggle VoiceOver support.

  @param[in]  Context   Picker context.
  @param[in]  File      File to play after enabling VoiceOver.
**/
VOID
EFIAPI
OcToggleVoiceOver (
  IN  OC_PICKER_CONTEXT  *Context,
  IN  UINT32             File  OPTIONAL
  );

/**
  Obtain BootOrder entry list.

  @param[in]   BootVariableGuid  GUID namespace for boot entries.
  @param[in]   WithBootNext      Add BootNext as the first option if available.
  @param[out]  BootOrderCount    Number of entries in boot order.
  @param[out]  Deduplicated      Whether the list was changed during deduplication, optional.
  @param[out]  HasBootNext       Whether the list starts with BootNext, optional

  @retval  boot order entry list allocated from pool or NULL.
**/
UINT16 *
OcGetBootOrder (
  IN  EFI_GUID  *BootVariableGuid,
  IN  BOOLEAN   WithBootNext,
  OUT UINTN     *BootOrderCount,
  OUT BOOLEAN   *Deduplicated  OPTIONAL,
  OUT BOOLEAN   *HasBootNext   OPTIONAL
  );

/**
  Register top-most priority boot option.
  MatchSuffix allows for fuzzy replacement of the option,
  i.e. when a new option replaces an old one with a matching suffix
  but a different prefix.

  @param[in]  OptionName       Option name to create.
  @param[in]  DeviceHandle     Device handle of the file system.
  @param[in]  FilePath         Bootloader path.
  @param[in]  ShortForm        Whether the Device Path should be written in
                               short-form.
  @param[in]   MatchSuffix     The file Device Path suffix of a matching option.
  @param[in]   MatchSuffixLen  The length, in characters, of MatchSuffix.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcRegisterBootstrapBootOption (
  IN CONST CHAR16    *OptionName,
  IN EFI_HANDLE      DeviceHandle,
  IN CONST CHAR16    *FilePath,
  IN BOOLEAN         ShortForm,
  IN CONST CHAR16    *MatchSuffix,
  IN UINTN           MatchSuffixLen
  );

/**
  Initialises custom Boot Services overrides to support direct images.
**/
VOID
OcImageLoaderInit (
  VOID
  );

/**
  Make DirectImageLoader the default for Apple Secure Boot.
**/
VOID
OcImageLoaderActivate (
  VOID
  );

/**
  Image loader callback triggered before LoadImage.
**/
typedef
VOID
(*OC_IMAGE_LOADER_PATCH) (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath  OPTIONAL,
  IN VOID                      *SourceBuffer,
  IN UINTN                     SourceSize
  );

/**
  Image loader callback triggered before StartImage.
**/
typedef
VOID
(*OC_IMAGE_LOADER_CONFIGURE) (
  IN OUT EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage,
  IN     UINT32                     Capabilities
  );


/**
  Register image loading callback.

  @param[in] Patch      Callback function to call on image load.
**/
VOID
OcImageLoaderRegisterPatch (
  IN OC_IMAGE_LOADER_PATCH  Patch      OPTIONAL
  );

/**
  Register image start callback.

  @param[in] Configure  Callback function to call on image start.
**/
VOID
OcImageLoaderRegisterConfigure (
  IN OC_IMAGE_LOADER_CONFIGURE  Configure  OPTIONAL
  );

/**
  Simplified load image routine, which bypasses UEFI and loads the image directly.

  @param[in]   BootPolicy        Ignored.
  @param[in]   ParentImageHandle The caller's image handle.
  @param[in]   DevicePath        Ignored.
  @param[in]   SourceBuffer      Pointer to the memory location containing image to be loaded.
  @param[in]   SourceSize        The size in bytes of SourceBuffer.
  @param[out]  ImageHandle       The pointer to the returned image handle created on success.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
EFIAPI
OcImageLoaderLoad (
  IN  BOOLEAN                  BootPolicy,
  IN  EFI_HANDLE               ParentImageHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  IN  VOID                     *SourceBuffer OPTIONAL,
  IN  UINTN                    SourceSize,
  OUT EFI_HANDLE               *ImageHandle
  );

#endif // OC_BOOT_MANAGEMENT_LIB_H
