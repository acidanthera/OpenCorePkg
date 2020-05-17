/** @file
  This library implements HDA device information.

  Copyright (c) 2018 John Davis. All rights reserved.<BR>
  Copyright (c) 2020, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef OC_HDA_DEVICES_LIB_H
#define OC_HDA_DEVICES_LIB_H

#include <Uefi.h>

/**
  Based on FreeBSD audio driver later borrowed by VoodooHDA.
**/

/**
  Generic names.
**/
#define HDA_CONTROLLER_MODEL_GENERIC    "HD Audio Controller"
#define HDA_CODEC_MODEL_GENERIC         "Unknown Codec"

/**
  Vendor IDs.
**/
#define VEN_AMD_ID              0x1002
#define VEN_ANALOGDEVICES_ID    0x11D4
#define VEN_AGERE_ID            0x11C1
#define VEN_CIRRUSLOGIC_ID      0x1013
#define VEN_CHRONTEL_ID         0x17E8
#define VEN_CONEXANT_ID         0x14F1
#define VEN_CREATIVE_ID         0x1102
#define VEN_IDT_ID              0x111D
#define VEN_INTEL_ID            0x8086
#define VEN_LG_ID               0x1854
#define VEN_NVIDIA_ID           0x10DE
#define VEN_QEMU_ID             0x1AF4
#define VEN_REALTEK_ID          0x10EC
#define VEN_SIGMATEL_ID         0x8384
#define VEN_VIA_ID              0x1106
#define VEN_CMEDIA_ID           0x13F6
#define VEN_CMEDIA2_ID          0x434D
#define VEN_RDC_ID              0x17F3
#define VEN_SIS_ID              0x1039
#define VEN_ULI_ID              0x10B9
#define VEN_MOTO_ID             0x1057
#define VEN_SII_ID              0x1095
#define VEN_VMWARE_ID           0x15AD
#define VEN_WOLFSON_ID          0x14EC

#define VEN_INVALID_ID          0xFFFF

#define GET_PCI_VENDOR_ID(a)    (a & 0xFFFFU)
#define GET_PCI_DEVICE_ID(a)    ((a >> 16U) & 0xFFFFU)
#define GET_PCI_GENERIC_ID(a)   ((0xFFFFU << 16U) | a)
#define GET_CODEC_VENDOR_ID(a)  ((a >> 16U) & 0xFFFFU)
#define GET_CODEC_DEVICE_ID(a)  (a & 0xFFFFU)
#define GET_CODEC_GENERIC_ID(a) (a | 0xFFFFU)

/**
  Get controller name.

  @param[in]  ControllerId  Controller identifier.

  @retval Controller name or NULL.
**/
CONST CHAR8 *
OcHdaControllerGetName (
  IN  UINT32  ControllerId
  );

/**
  Get codec name.

  @param[in]  CodecId     Codec identifier.
  @param[in]  RevisionId  Codec revision.

  @retval Controller name or NULL.
**/
CONST CHAR8 *
OcHdaCodecGetName (
  IN  UINT32  CodecId,
  IN  UINT16  RevisionId
  );

#endif // OC_HDA_DEVICES_LIB_H
