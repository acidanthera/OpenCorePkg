/** @file
  Copyright (C) 2017, vit9696. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_BOOT_ARGS_H
#define APPLE_BOOT_ARGS_H

///
/// Keep in sync with XNU pexpert/pexpert/i386/boot.h.
/// Last sync time: 4903.221.2.
///

/*
 * Video information...
 */

struct Boot_VideoV1 {
  UINT32  v_baseAddr; /* Base address of video memory */
  UINT32  v_display;  /* Display Code (if Applicable */
  UINT32  v_rowBytes; /* Number of bytes per pixel row */
  UINT32  v_width;    /* Width */
  UINT32  v_height;   /* Height */
  UINT32  v_depth;    /* Pixel Depth */
};
typedef struct Boot_VideoV1 Boot_VideoV1;

struct Boot_Video {
  UINT32 v_display;   /* Display Code (if Applicable */
  UINT32 v_rowBytes;  /* Number of bytes per pixel row */
  UINT32 v_width;     /* Width */
  UINT32 v_height;    /* Height */
  UINT32 v_depth;     /* Pixel Depth */
  UINT32 v_resv[7];   /* Reserved */
  UINT64 v_baseAddr;  /* Base address of video memory */
};
typedef struct Boot_Video Boot_Video;

/* Values for v_display */

#define GRAPHICS_MODE           1
#define FB_TEXT_MODE            2

/* Boot argument structure - passed into Mach kernel at boot time.
 * "Revision" can be incremented for compatible changes
 */
#define kBootArgsRevision    0
#define kBootArgsRevision0   kBootArgsRevision
#define kBootArgsRevision1   1 /* added KC_hdrs_addr */
#define kBootArgsVersion     2

/* Snapshot constants of previous revisions that are supported */
#define kBootArgsVersion1       1
#define kBootArgsRevision1_4    4
#define kBootArgsRevision1_5    5
#define kBootArgsRevision1_6    6

#define kBootArgsVersion2       2
#define kBootArgsRevision2_0    0

#define kBootArgsEfiMode32      32
#define kBootArgsEfiMode64      64

/* Bitfields for boot_args->flags */
#define kBootArgsFlagRebootOnPanic   (1U << 0U)
#define kBootArgsFlagHiDPI           (1U << 1U)
#define kBootArgsFlagBlack           (1U << 2U)
#define kBootArgsFlagCSRActiveConfig (1U << 3U)
#define kBootArgsFlagCSRConfigMode   (1U << 4U)
#define kBootArgsFlagCSRBoot         (1U << 5U)
#define kBootArgsFlagBlackBg         (1U << 6U)
#define kBootArgsFlagLoginUI         (1U << 7U)
#define kBootArgsFlagInstallUI       (1U << 8U)
#define kBootArgsFlagRecoveryBoot    (1U << 10U)

#define BOOT_LINE_LENGTH   1024

/* version 1 before Lion */
typedef struct {
  UINT16          Revision;            /* Revision of boot_args structure */
  UINT16          Version;             /* Version of boot_args structure */

  CHAR8           CommandLine[BOOT_LINE_LENGTH]; /* Passed in command line */

  UINT32          MemoryMap;           /* Physical address of memory map */
  UINT32          MemoryMapSize;
  UINT32          MemoryMapDescriptorSize;
  UINT32          MemoryMapDescriptorVersion;

  Boot_VideoV1    Video;                /* Video Information */

  UINT32          deviceTreeP;          /* Physical address of flattened device tree */
  UINT32          deviceTreeLength;     /* Length of flattened tree */

  UINT32          kaddr;                /* Physical address of beginning of kernel text */
  UINT32          ksize;                /* Size of combined kernel text+data+efi */

  UINT32          efiRuntimeServicesPageStart;   /* physical address of defragmented runtime pages */
  UINT32          efiRuntimeServicesPageCount;
  UINT32          efiSystemTable;       /* physical address of system table in runtime area */

  UINT8           efiMode;              /* 32 = 32-bit, 64 = 64-bit */
  UINT8           __reserved1[3];
  UINT32          __reserved2[1];
  UINT32          performanceDataStart; /* physical address of log */
  UINT32          performanceDataSize;
  UINT64          efiRuntimeServicesVirtualPageStart; /* virtual address of defragmented runtime pages */
  UINT32          __reserved3[2];

} BootArgs1;

/* version2 as used in Lion, updated with High Sierra fields */
typedef struct {

  UINT16          Revision;        /* Revision of boot_args structure */
  UINT16          Version;         /* Version of boot_args structure */

  UINT8           efiMode;         /* 32 = 32-bit, 64 = 64-bit */
  UINT8           debugMode;       /* Bit field with behavior changes */
  UINT16          flags;

  CHAR8           CommandLine[BOOT_LINE_LENGTH]; /* Passed in command line */

  UINT32          MemoryMap;       /* Physical address of memory map */
  UINT32          MemoryMapSize;
  UINT32          MemoryMapDescriptorSize;
  UINT32          MemoryMapDescriptorVersion;

  Boot_VideoV1    VideoV1;            /* Video Information */

  UINT32          deviceTreeP;         /* Physical address of flattened device tree */
  UINT32          deviceTreeLength;    /* Length of flattened tree */

  UINT32          kaddr;               /* Physical address of beginning of kernel text */
  UINT32          ksize;               /* Size of combined kernel text+data+efi */

  UINT32          efiRuntimeServicesPageStart;        /* physical address of defragmented runtime pages */
  UINT32          efiRuntimeServicesPageCount;
  UINT64          efiRuntimeServicesVirtualPageStart; /* virtual address of defragmented runtime pages */

  UINT32          efiSystemTable;       /* physical address of system table in runtime area */
  UINT32          kslide;

  UINT32          performanceDataStart; /* physical address of log */
  UINT32          performanceDataSize;

  UINT32          keyStoreDataStart;    /* physical address of key store data */
  UINT32          keyStoreDataSize;
  UINT64          bootMemStart;         /* physical address of interpreter boot memory */
  UINT64          bootMemSize;
  UINT64          PhysicalMemorySize;
  UINT64          FSBFrequency;
  UINT64          pciConfigSpaceBaseAddress;
  UINT32          pciConfigSpaceStartBusNumber;
  UINT32          pciConfigSpaceEndBusNumber;
  UINT32          csrActiveConfig;
  UINT32          csrCapabilities;
  UINT32          boot_SMC_plimit;
  UINT16          bootProgressMeterStart;
  UINT16          bootProgressMeterEnd;
  Boot_Video      Video;                /* Video Information */

  UINT32          apfsDataStart;        /* Physical address of apfs volume key structure */
  UINT32          apfsDataSize;

  /* Version 2, Revision 1 */
  UINT64          KC_hdrs_vaddr; /* First kernel virtual address pointing to Mach-O headers */

  UINT64          arvRootHashStart; /* Physical address of root hash file */
  UINT64          arvRootHashSize;

  UINT64          arvManifestStart; /* Physical address of manifest file */
  UINT64          arvManifestSize;

  /* Reserved */
  UINT32          __reserved4[700];
} BootArgs2;

#endif // APPLE_BOOT_ARGS_H
