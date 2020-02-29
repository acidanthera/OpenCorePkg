/** @file
  This library implements HDA device information.

  Copyright (c) 2018 John Davis. All rights reserved.<BR>
  Copyright (c) 2020, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef OC_HDA_DEVICES_INTERNAL_H
#define OC_HDA_DEVICES_INTERNAL_H

typedef struct {
  UINT32        Id;
  CONST CHAR8   *Name;
} HDA_CONTROLLER_LIST_ENTRY;

typedef struct {
  UINT32        Id;
  UINT16        Rev;
  CONST CHAR8   *Name;
} HDA_CODEC_LIST_ENTRY;

//
// Controller models.
//
#define HDA_CONTROLLER(vendor, id) (((UINT32) (id) << 16U) | ((VEN_##vendor##_ID) & 0xFFFFU))

//
// Codec models.
//
#define HDA_CODEC(vendor, id) (((UINT32) (VEN_##vendor##_ID) << 16U) | ((id) & 0xFFFFU))

#endif // OC_HDA_DEVICES_INTERNAL_H
