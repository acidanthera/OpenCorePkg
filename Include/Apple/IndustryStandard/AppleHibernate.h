/** @file
  Copyright (C) 2017, vit9696. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_HIBERNATE_H
#define APPLE_HIBERNATE_H

///
/// Hibernate related definitions.
/// Keep in sync with XNU iokit/IOKit/IOHibernatePrivate.h.
/// Last sync time: 4903.221.2.
///

#pragma pack(push, 1)

typedef struct
{
  UINT64  start;
  UINT64  length;
} IOPolledFileExtent;

typedef struct
{
  UINT64  imageSize;
  UINT64  image1Size;

  UINT32  restore1CodePhysPage;
  UINT32  reserved1;
  UINT64  restore1CodeVirt;
  UINT32  restore1PageCount;
  UINT32  restore1CodeOffset;
  UINT32  restore1StackOffset;

  UINT32  pageCount;
  UINT32  bitmapSize;

  UINT32  restore1Sum;
  UINT32  image1Sum;
  UINT32  image2Sum;

  UINT32  actualRestore1Sum;
  UINT32  actualImage1Sum;
  UINT32  actualImage2Sum;

  UINT32  actualUncompressedPages;
  UINT32  conflictCount;
  UINT32  nextFree;

  UINT32  signature;
  UINT32  processorFlags;

  UINT32  runtimePages;
  UINT32  runtimePageCount;
  UINT64  runtimeVirtualPages;

  UINT32  performanceDataStart;
  UINT32  performanceDataSize;

  UINT64  encryptStart;
  UINT64  machineSignature;

  UINT32  previewSize;
  UINT32  previewPageListSize;

  UINT32  diag[4];

  UINT32  handoffPages;
  UINT32  handoffPageCount;

  UINT32  systemTableOffset;

  UINT32  debugFlags;
  UINT32  options;
  UINT32  sleepTime;
  UINT32  compression;

  UINT8   bridgeBootSessionUUID[16];

  UINT32  reserved[54];    // make sizeof == 512
  UINT32  booterTime0;
  UINT32  booterTime1;
  UINT32  booterTime2;

  UINT32  booterStart;
  UINT32  smcStart;
  UINT32  connectDisplayTime;
  UINT32  splashTime;
  UINT32  booterTime;
  UINT32  trampolineTime;

  UINT64  encryptEnd;
  UINT64  deviceBase;
  UINT32  deviceBlockSize;

  UINT32  fileExtentMapSize;
  IOPolledFileExtent  fileExtentMap[2];
} IOHibernateImageHeader;

enum
{
  kIOHibernateHandoffType                 = 0x686f0000,
  kIOHibernateHandoffTypeEnd              = kIOHibernateHandoffType + 0,
  kIOHibernateHandoffTypeGraphicsInfo     = kIOHibernateHandoffType + 1,
  kIOHibernateHandoffTypeCryptVars        = kIOHibernateHandoffType + 2,
  kIOHibernateHandoffTypeMemoryMap        = kIOHibernateHandoffType + 3,
  kIOHibernateHandoffTypeDeviceTree       = kIOHibernateHandoffType + 4,
  kIOHibernateHandoffTypeDeviceProperties = kIOHibernateHandoffType + 5,
  kIOHibernateHandoffTypeKeyStore         = kIOHibernateHandoffType + 6,
  kIOHibernateHandoffTypeVolumeCryptKey   = kIOHibernateHandoffType + 7,
};

typedef struct
{
  UINT32 type;
  UINT32 bytecount;
  UINT8  data[];
} IOHibernateHandoff;

typedef struct
{
  UINT8    signature[4];
  UINT32   revision;
  UINT8    booterSignature[20];
  UINT8    wiredCryptKey[16];
} AppleRTCHibernateVars;

#pragma pack(pop)

#endif // APPLE_HIBERNATE_H
