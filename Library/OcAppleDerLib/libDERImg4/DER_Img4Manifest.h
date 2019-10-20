/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef DER_IMG4_MANIFEST_H
#define DER_IMG4_MANIFEST_H

#include "libDER/libDER.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  DERItem magic;
  DERItem version;
  DERItem body;
  DERItem signature;
  DERItem certificates;
} DERImg4Manifest;

static const DERItemSpec DERImg4ManifestItemSpecs[] = {
  { DER_OFFSET (DERImg4Manifest, magic),        ASN1_IA5_STRING,      DER_DEC_NO_OPTS  },
  { DER_OFFSET (DERImg4Manifest, version),      ASN1_INTEGER,         DER_DEC_NO_OPTS  },
  { DER_OFFSET (DERImg4Manifest, body),         ASN1_CONSTR_SET,      DER_DEC_SAVE_DER },
  { DER_OFFSET (DERImg4Manifest, signature),    ASN1_OCTET_STRING,    DER_DEC_NO_OPTS  },
  { DER_OFFSET (DERImg4Manifest, certificates), ASN1_CONSTR_SEQUENCE, DER_DEC_NO_OPTS  }
};
static const DERShort DERNumImg4ManifestItemSpecs =
  sizeof (DERImg4ManifestItemSpecs) / sizeof (DERItemSpec);

#define DER_IMG4_PROP_TAG(A, B, C, D)  \
  (((uint32_t)(A) << 24U)          \
 | ((uint32_t)(B) << 16U)          \
 | ((uint32_t)(C) << 8U)           \
 | ((uint32_t)(D) << 0U))          \

#define DER_IMG4_TAG_MAN_MAGIC  (DER_IMG4_PROP_TAG ('I', 'M', '4', 'M'))
#define DER_IMG4_TAG_MAN_BODY   (DER_IMG4_PROP_TAG ('M', 'A', 'N', 'B'))
#define DER_IMG4_TAG_MAN_PROPS  (DER_IMG4_PROP_TAG ('M', 'A', 'N', 'P'))
#define DER_IMG4_TAG_OBJ_PROPS  (DER_IMG4_PROP_TAG ('O', 'B', 'J', 'P'))

#define DER_IMG4_TAG_OBJ_EPRO   (DER_IMG4_PROP_TAG ('E', 'P', 'R', 'O'))
#define DER_IMG4_TAG_OBJ_ESEC   (DER_IMG4_PROP_TAG ('E', 'S', 'E', 'C'))
#define DER_IMG4_TAG_OBJ_DGST   (DER_IMG4_PROP_TAG ('D', 'G', 'S', 'T'))
#define DER_IMG4_TAG_OBJ_EKEY   (DER_IMG4_PROP_TAG ('E', 'K', 'E', 'Y'))

#define DER_IMG4_TAG_MAN_CEPO   (DER_IMG4_PROP_TAG ('C', 'E', 'P', 'O'))
#define DER_IMG4_TAG_MAN_ECID   (DER_IMG4_PROP_TAG ('E', 'C', 'I', 'D'))
#define DER_IMG4_TAG_MAN_CHIP   (DER_IMG4_PROP_TAG ('C', 'H', 'I', 'P'))
#define DER_IMG4_TAG_MAN_BORD   (DER_IMG4_PROP_TAG ('B', 'O', 'R', 'D'))
#define DER_IMG4_TAG_MAN_AMNM   (DER_IMG4_PROP_TAG ('A', 'M', 'N', 'M'))
#define DER_IMG4_TAG_MAN_SDOM   (DER_IMG4_PROP_TAG ('S', 'D', 'O', 'M'))
#define DER_IMG4_TAG_MAN_IUOB   (DER_IMG4_PROP_TAG ('i', 'u', 'o', 'b'))
#define DER_IMG4_TAG_MAN_MPRO   (DER_IMG4_PROP_TAG ('m', 'p', 'r', 'o'))
#define DER_IMG4_TAG_MAN_MSEC   (DER_IMG4_PROP_TAG ('m', 's', 'e', 'c'))
#define DER_IMG4_TAG_MAN_XUGS   (DER_IMG4_PROP_TAG ('x', 'u', 'g', 's'))

#define DER_IMG4_ENCODE_PROPERTY_NAME(Name) ((Name) | ASN1_CONSTRUCTED | ASN1_PRIVATE)

typedef struct {
  DERItem nameItem;
  DERTag  nameTag;
  DERItem valueItem;
  DERTag  valueTag;
} DERImg4Property;

#define DERImg4PropertySpecInit {                                                 \
  { DER_OFFSET (DERImg4Property, nameItem),  ASN1_IA5_STRING, DER_DEC_NO_OPTS },  \
  { DER_OFFSET (DERImg4Property, valueItem), 0,               DER_DEC_NO_OPTS }   \
}

#ifdef __cplusplus
}
#endif

#endif // DER_IMG4_MANIFEST_H
