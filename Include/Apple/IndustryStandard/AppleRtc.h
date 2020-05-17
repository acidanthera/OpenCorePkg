/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_RTC_H
#define APPLE_RTC_H

/**
  Sample RTC memory dump from MBP12,1 running 10.15.4:

  00: 33 02 01 00 12 22 04 16 04 20 26 02 70 80 FF FF
  10: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
  20: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
  30: 01 FE 20 FF FF FF FF FF F8 11 FF FF FF 00 1A 7C
  40: 00 00 00 00 FE 00 00 00 00 00 00 00 00 00 00 00
  50: 00 00 00 00 00 00 00 FF 0B D1 80 00 05 00 00 00
  60: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  70: 00 00 00 00 00 00 00 00 FF FF FF FF FF FF FF FF
  80: 44 45 41 44 00 00 00 00 00 00 00 00 00 00 00 00
  90: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  A0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  B0: 76 CF 85 13 00 00 00 00 00 00 00 00 00 00 00 00
  C0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  D0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  E0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  F0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
**/

/**
  APPLE_RTC_BG_COLOR_ADDR       - background colour.
  APPLE_RTC_BG_COMPLEMENT_ADDR  - complementary byte to background colour.

  0x00 - Grey background (applies to 10.9 and older).
  0x01 - Black background (applies to 10.10 and newer).

  Valid pair gives APPLE_RTC_BG_COLOR_ADDR ^ APPLE_RTC_BG_COMPLEMENT_ADDR == 0xFF.
  Otherwise background colour should be treated as 0, grey.

  Prior to rebooting to MacEFI firmare upgrade /usr/libexec/efiupdater
  removes DefaultBackgroundColor NVRAM variable and resets background to black.

  When performing hibernate wake OSInfo writes chooses the background colour
  based on reported macOS version (grey for 10.9 and below, black otherwise).
**/
#define APPLE_RTC_BG_COLOR_ADDR          0x30
#define APPLE_RTC_BG_COMPLEMENT_ADDR     0x31

/**
  APPLE_RTC_BG_COLOR_ADDR values.
**/
#define APPLE_RTC_BG_COLOR_GRAY          0x00
#define APPLE_RTC_BG_COLOR_BLACK         0x01

/**
  By default masked out with 0xF8.
  Connected thunderbolt devices can set 3 lower bits.
  BIT3 seems to be set when InstallWindowsUEFI was present at boot.

  See ThunderboltNhi and PchInitDxe.
**/
#define APPLE_RTC_FIRMWARE_STATE_ADDR    0x38

/**
  Used by ApplePlatformInit, 1 byte.
**/
#define APPLE_RTC_FIRMWARE_3D_ADDR       0x3D

/**
  Bitmask of started test indices.
  Little endian 32-bit number.
  Note: memory testing statuses are a bit more complicated,
  but we do not need too much detail atm.
**/
#define APPLE_RTC_MEM_TEST_ACCEPT_ADDR   0x44

/**
  Bitmask of request test indices.
  Little endian 32-bit number.
**/
#define APPLE_RTC_MEM_TEST_REQUEST_ADDR  0x48

/**
  Bitmask of performed test indices.
  Little endian 32-bit number.
**/
#define APPLE_RTC_MEM_TEST_RESULT_ADDR   0x4C

/**
  Another unknown reserved address.
**/
#define APPLE_RTC_FIRMWARE_57_ADDR       0x57

/**
  Firmware boot progress, mostly maintained by AppleBds.

  0x00, 0x01, 0x02 - Set by RomIntegrityPei.
  0x04 - Normal boot started. At this step AppleBds will process hibernation,
         enable video output, check battery state, perform memory test, etc.
  0x05 - Exit Boot Services called. Set in AppleBds event handler.
**/
#define APPLE_RTC_FIRMWARE_PROGRESS_ADDR 0x5C

/**
  Apple RTC reserved memory area for unclear purposes, normally filled with FF.
  Should not be erased during PRAM reset (CMD+OPT+P+R).
**/
#define APPLE_RTC_RESERVED_ADDR          0x78

/**
  Apple RTC reserved memory size.
**/
#define APPLE_RTC_RESERVED_LENGTH        8

/**
  Hibernation image encryption data, a copy of IOHibernateRTCVariables.
  See AppleRTCHibernateVars type in AppleHibernate.h for the definition.
  - Valid hibernation image encryption data starts with "AAPL" magic.
  - Erased hibernation image encryption data starts with "DEAD" magic.
  Normally handled by AppleBds prior to transfering control to the bootloader.
**/
#define APPLE_RTC_HIBERNATION_KEY_ADDR   0x80

/**
  Effectively equals to sizeof (AppleRTCHibernateVars).
**/
#define APPLE_RTC_HIBERNATION_KEY_LENGTH 0x2C

/**
  Firmware upgrade control and status byte.

  0x00 - Normal boot. AppleBds resets to this value upon BOOT_ON_FLASH_UPDATE
         prior to running BootRomFlash. BootRomFlash may itself reset to this
         value on successful(?) flashing. Mirrored to NOBP SMC key.
  0x01 - Alternative unsuccessful (?) result of firmware flashing written
         by BootRomFlash. Mirrored to NOBP SMC key. Also set in ApplePlatformInit.
  0x02 - Schedule firmware upgrade. Closed-source bless part writes this value
         when intending to upgrade MacEFI firmware.
  0x03 - Peform firmware upgrade. BootRomFlash writes this value prior to
         rebooting into BOOT_ON_FLASH_UPDATE mode. Might be needed due to region
         unlock.
**/
#define APPLE_RTC_FIRMWARE_UPGRADE_ADDR  0xAC

/**
  AppleBds zeroes this byte at the same time it reaches normal booting phase (0x4)
  in APPLE_RTC_FIRMWARE_PROGRESS_ADDR. This may be set to non-zero value by ApplePlatformInit.
**/
#define APPLE_RTC_FIRMWARE_CHECK_ADDR    0xAF

/**
  Data contained in RTC allowing to trace various power management events.
  Called via AppleRTC::rtcRecordTracePoint and also used for EfiBoot wake log.
**/
#define APPLE_RTC_TRACE_POINT_ADDR       0xB0

/**
  Trace point data contains a total of 8 bytes, but they seem to have varying purpose.
**/
#define APPLE_RTC_TRACE_POINT_LENGTH     8

/**
  Trace data is a 32-bit value, which format depends on the phase
  (APPLE_RTC_TRACE_PHASE_ADDR) the platform is currently in.

  For the kernel this may e.g. contain PCI device index currently put
  in sleep or being awoken (kIOPMTracePointSleepPowerPlaneDrivers
  or kIOPMTracePointWakePowerPlaneDrivers).

  For EfiBoot this is named hibernatin wake log. It is also written to
  wake-failure NVRAM variable (and later DeviceTree).
**/
#define APPLE_RTC_TRACE_DATA_ADDR        0xB0

/**
  Trace data length.
**/
#define APPLE_RTC_TRACE_DATA_LENGTH      4

/**
  Mask of events, which happened during booting.
  These events are appended during the boot progress.

  Note, unlike APPLE_RTC_WL_EVENT_ADDR/APPLE_RTC_WL_EVENT_EXTRA_ADDR,
  which only get updated in hibernate wake, this value is always written
  when a non-zero event is supplied.
**/
#define APPLE_RTC_WL_MASK_ADDR   0xB1

/**
  Known event bits.
**/
#define APPLE_RTC_WL_MASK_BOOT_STARTED   BIT0 ///< Called early at boot.efi startup.
#define APPLE_RTC_WL_MASK_BOOT_FAILED    BIT1 ///< gBS->Exit: Boot failed; will sleep for 10 seconds before exiting.
#define APPLE_RTC_WL_MASK_BOOT_KERNEL    BIT2 ///< Called right before jumping to the kernel code.
#define APPLE_RTC_WL_MASK_CS_UNLOCKED    BIT3 ///< Set along with APPLE_RTC_WL_CS_VOLUME_UNLOCKED.
#define APPLE_RTC_WL_MASK_HIB_CLEAR_KEYS BIT4 ///< Removed boot-image-key, EFI_STATUS in EXTRA.
#define APPLE_RTC_WL_MASK_HIB_CLEAR_IMG  BIT5 ///< Removed boot-image, will reboot now.
#define APPLE_RTC_WL_MASK_BOOT_RESET     BIT6 ///< gBS->Reset was called.

/**
  Event index that happened last during booting.
  0xB2 is the event type and 0xB3 is its subtype.
**/
#define APPLE_RTC_WL_EVENT_ADDR        0xB2
#define APPLE_RTC_WL_EVENT_EXTRA_ADDR  0xB3

/**
  Known events.
**/
#define APPLE_RTC_WL_EVENT_DEFAULT            0  ///< For everything.
#define APPLE_RTC_WL_INIT_DEVICE_TREE         2  ///< Before first "Start InitDeviceTree".
#define APPLE_RTC_WL_KERNEL_ALLOC_CALL_GATE   3  ///< After "End InitDeviceTree".
#define APPLE_RTC_WL_INIT_MEMORY_CONFIG       4  ///< Before "Start InitMemoryConfig".
#define APPLE_RTC_WL_HIB_CHECK                5  ///< Before "Start CheckHibernate".
#define APPLE_RTC_WL_CS_LOAD_CONFIGURATION    6  ///< Before "Start LoadCoreStorageConfiguration".
#define APPLE_RTC_WL_GET_FDE_KEY              7  ///< Before reading HBKP key.
#define APPLE_RTC_WL_HIB_WAKE_START           8  ///< Before reading FACP table.
#define APPLE_RTC_WL_HIB_GET_MMAP             9  ///< Before GetMemoryMap for hibernation.
#define APPLE_RTC_WL_HIB_HWSIG_VALID         10  ///< After confirming FACP signature validity with the image.
#define APPLE_RTC_WL_HIB_MEM_ALLOC           11  ///< Before restore1CodePhysPage / runtimePages allocation.
#define APPLE_RTC_WL_HIB_SPLASH              12  ///< Before "Start OpenBootGraphics".
#define APPLE_RTC_WL_HIB_READ                13  ///< After "Start hibernate read".
#define APPLE_RTC_WL_HIB_HANDOFF             14  ///< Before adjusting IOHibernateImageHeader and booting kernel.
#define APPLE_RTC_WL_HIB_EXIT_BOOT_SERVICES  15  ///< Done ExitBootServices for hibernate, EFI_STATUS in EXTRA.
#define APPLE_RTC_WL_HIB_KERNEL_START        16  ///< Calling kernel entry point for hibernate.
#define APPLE_RTC_WL_HIB_FINISH_FDE          17  ///< After "Start FinishFDEHibernate".
#define APPLE_RTC_WL_HIB_WIRED_KEY_VALID     18  ///< Before APPLE_RTC_WL_HIBERNATE_WAKE_START with valid FDE key.
#define APPLE_RTC_WL_RECOVERY_OS_FOUND       19  ///< After detecting com.apple.recovery.boot in boot path.
#define APPLE_RTC_WL_CS_VOLUME_UNLOCKED      20  ///< After "End UnlockCoreStorageVolumeKey" with EFI_STATUS in EXTRA.
#define APPLE_RTC_WL_KERNEL_LOAD_CACHE_FAIL  22  ///< Failed to load kernel cache, EFI_STATUS in EXTRA.
#define APPLE_RTC_WL_HIB_BS_MISSING          23  ///< Missing boot-signature, EFI_STATUS in EXTRA.
#define APPLE_RTC_WL_HIB_BIK_MISSING         24  ///< Missing boot-image-key, EFI_STATUS in EXTRA.
#define APPLE_RTC_WL_RECOVERY_OS_REBOOT      25  ///< Before rebooting into recovery with APPLE_RTC_WL_MASK_BOOT_RESET.
#define APPLE_RTC_WL_LOGIN_WINDOW_PWRESET    26  ///< Requested FDE password recovery.
#define APPLE_RTC_WL_LOGIN_WINDOW_GUEST      27  ///< Chose guest in FV2 login window, will gBS->Reset now.
#define APPLE_RTC_WL_LOGIN_WINDOW_RESET      28  ///< Chose reset in FV2 login window, will gBS->Reset now.
#define APPLE_RTC_WL_LOGIN_WINDOW_HIB        29  ///< Will save hibernation data after FV2 login window and shutdown.
#define APPLE_RTC_WL_LOGIN_WINDOW_SHUTDOWN   30  ///< Chose shutdown in FV2 login window, will gBS->Reset now.
#define APPLE_RTC_WL_LOGIN_WINDOW_FAIL       31  ///< After "End LoginWindow Initialize", EFI_STATUS in EXTRA.
#define APPLE_RTC_WL_HIB_ABORT_SAFE_MODE     32  ///< Hibernation aborted due to safe mode, after "Start CheckHibernate".
#define APPLE_RTC_WL_HIB_BI_ALLOC            33  ///< Failed to allocate memory for boot-image, EFI_STATUS in EXTRA.
#define APPLE_RTC_WL_HIB_BI_MISSING          34  ///< Missing boot-image variable or 0 size, EFI_STATUS in EXTRA.
#define APPLE_RTC_WL_HIB_BI_INVALID          35  ///< Did not find valid data in boot-image.
#define APPLE_RTC_WL_HIB_BI_DP_ALLOC         36  ///< Failed to allocate boot-image device path(?).
#define APPLE_RTC_WL_HIB_BI_DP_ZERO          37  ///< boot-image contains empty DP(?).
#define APPLE_RTC_WL_HIB_BI_DP_MISSING       38  ///< Failed to locate boot-image device path, EFI_STATUS in EXTRA.
#define APPLE_RTC_WL_HIB_BI_DP_NO_DISK_IO    39  ///< Failed to find Disk I/O protocol, EFI_STATUS in EXTRA.
#define APPLE_RTC_WL_HIB_BI_DP_NI_BLOCK_IO   40  ///< Failed to find Block I/O protocol, EFI_STATUS in EXTRA.

/**
  Certain components have the ability to log their wake progress
  directly via RTC. See XNU IOPMPrivate.h for more details.

  LoginWindow  - 4 bits
  CoreDisplay  - 8 bits
  CoreGraphics - 8 bits

  Apparently these look mostly unused, at least in release builds.
  CoreDisplay is known to report 0 (very helpful) on screen wake.
  I guess we can abuse them if we want.
**/
#define APPLE_RTC_TRACE_LOGIN_WINDOW_ADDR  0xB4
#define APPLE_RTC_TRACE_CORE_DISPLAY_ADDR  0xB5
#define APPLE_RTC_TRACE_CORE_GRAPHICS_ADDR 0xB6

/**
  Trace point power phase e.g. kIOPMTracePointSystemUp.
  See XNU IOPMPrivate.h for more details.
**/
#define APPLE_RTC_TRACE_PHASE_ADDR         0xB7

/**
  256-bit full disk encryption (FDE) key used for authenticated restart.
  AppleRTC and other kexts name it key stash.

  Written by AppleRTC at the request of IOHibernateSMCVariables I/O registry
  entry submitted either by XNU kernel or AppleFDEKeyStore.kext. The variable
  itself is a pair of 64-bit values: key size and key address in kernel memory.

  This was replaced with HBKP SMC key in modern Macs.

  *** VirtualSMC addendum ***

  Since this value is no longer used due to HBKP present in almost every Mac
  we badly abuse this area when vertualising SMC.

  When HBKP SMC key is written VirtualSMC AES-128 encrypts the submitted value
  with a unique key and writes the result to an NVRAM variable (vsmc-key).
  The AES encryption key goes to RTC at this addrress instead.
  Afterwards the EFI module implementing SMC protocol performs the decryption.

  TODO: Replace AES with one-time pad, i.e. increase the temporary key size
        to 256 bits and just XOR the value. This will break old VirtualSmc.efi
        module, but we do not care anymore.
**/
#define APPLE_RTC_FDE_KEY_ADDR           0xD0

/**
  FDE key maximum and default, as AES-256-XTS has 256-bit key.
  For smaller keys remaining bytes are zeroed.
**/
#define APPLE_RTC_FDE_KEY_LENGTH         0x20

/**
  FDE key state. Normally written by ApplePlatformInit.

  0x00 - The key is either present or not.
  0x41 - The key is not present, and should be copied to RTC from HBKP
         SMC key prior to transfering control to the bootloader. After the
         copying succeeds this value is overwritten with 0x00. Normally
         this is done by AppleBds.
**/
#define APPLE_RTC_FDE_STATE_ADDR         0xF0

/**
  Known values for APPLE_RTC_FDE_STATE_ADDR.
**/
#define APPLE_RTC_FDE_STATE_DEFAULT      0x00
#define APPLE_RTC_FDE_STATE_NEED_KEY     0x41

/**
  Checksum is calculated starting from this address.
**/
#define APPLE_RTC_CHECKSUM_START         0x0E

/**
  First 64 bytes are RTC core and are hashed separately.
**/
#define APPLE_RTC_CORE_SIZE              0x40

/**
  All Apple hardware cares about 256 bytes of RTC memory.
**/
#define APPLE_RTC_TOTAL_SIZE             0x100

/**
  Apple checksum custom CRC polynomial.
  Apple checksum is a modified version of ANSI CRC16 in REFIN mode (0xA001 poly).
  See http://zlib.net/crc_v3.txt for more details.

  Effective poly is 0x2001 due to a bitwise OR with BIT15.
  The change turns CRC16 into CRC14, making BIT14 and BIT15 always zero.
  This modification is commonly found in legacy Phoenix firmwares,
  where it was used for password hashing as found by dogbert:
  http://sites.google.com/site/dogber1/blag/pwgen-5dec.py
**/
#define APPLE_RTC_CHECKSUM_POLYNOMIAL    0x2001

/**
  Apple checksum rounds.

  Only 7 shifts per byte are performed instead of the usual 8.
  This might improve checksum quality against specific data, but the exact
  reasons are unknown. The algorithm did not change since its introduction
  in 10.4.x, and since Apple Developer Transition Kit was based on Phoenix
  firmware, this could just be a quick change to get a different checksum.
**/
#define APPLE_RTC_CHECKSUM_ROUNDS        7

/**
  Checksum for Apple RTC with core part of the memory hashed:
  [APPLE_RTC_CHECKSUM_SART, APPLE_RTC_CORE_SIZE).
**/
#define APPLE_RTC_CORE_CHECKSUM_ADDR1    0x3E
#define APPLE_RTC_CORE_CHECKSUM_ADDR2    0x3F

/**
  Apple RTC core checksum to byte convertion macros.
  BYTE1 BIT0~BIT7 = CHECKSUM BIT0~BIT7
  BYTE2 BIT1~BIT7 = CHECKSUM BIT8~BIT14
**/
#define APPLE_RTC_CORE_CHECKSUM_BYTE1(Checksum) ((UINT8) ((UINT32) (Checksum) & 0xFFU))
#define APPLE_RTC_CORE_CHECKSUM_BYTE2(Checksum) ((UINT8) (((UINT32) (Checksum) >> 7U) & 0xFEU))

/**
  Checksum for Apple RTC with all the memory hashed:
  [APPLE_RTC_CHECKSUM_SART, APPLE_RTC_TOTAL_SIZE).
**/
#define APPLE_RTC_MAIN_CHECKSUM_ADDR1    0x58
#define APPLE_RTC_MAIN_CHECKSUM_ADDR2    0x59

/**
  Apple RTC main checksum to byte convertion macros.
  BYTE1 BIT0~BIT7 = CHECKSUM BIT0~BIT7
  BYTE2 BIT0~BIT7 = CHECKSUM BIT8~BIT15
**/
#define APPLE_RTC_MAIN_CHECKSUM_BYTE1(Checksum) ((UINT8) (((UINT32) (Checksum) >> 8U) & 0xFFU))
#define APPLE_RTC_MAIN_CHECKSUM_BYTE2(Checksum) ((UINT8) ((UINT32) (Checksum) & 0xFFU))

#endif // APPLE_RTC_H
