/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef LIB_DER_IMG4_H
#define LIB_DER_IMG4_H

#include "libDER/libDER.h"
#include "libDERImg4_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  DR_SecurityError = -1
} DERImg4Return;

typedef struct {
  uint64_t ecid;
  uint32_t boardId;
  uint32_t chipId;
  uint32_t certificateEpoch;
  uint32_t securityDomain;
  bool     productionStatus;
  bool     securityMode;
  bool     effectiveProductionStatus;
  bool     effectiveSecurityMode;
  bool     internalUseOnlyUnit;
  bool     enableKeys;
  uint32_t xugs;
  uint32_t allowMixNMatch;
} DERImg4Environment;

typedef struct {
  DERImg4Environment environment;
  bool               hasEcid;
  bool               hasEffectiveProductionStatus;
  bool               hasEffectiveSecurityMode;
  bool               hasXugs;
  size_t             imageDigestSize;
  uint8_t            imageDigest[DER_IMG4_MAX_DIGEST_SIZE];
} DERImg4ManifestInfo;

/**
  Verify and parse the IMG4 Manifest in ManBuffer and output its information.
  On success, the Manifest is guaranteed to be digitally signed with the
  platform-provided DERImg4RootCertificate.

  @param[out] ManInfo         Output Manifest information structure.
  @param[in]  ManifestBuffer  Buffer containing the Manifest data.
  @param[in]  ManifestSize    Size, in bytes, of ManBuffer.
  @param[in]  ObjType         The object type to inspect.

  @retval DR_Success  ManBuffer contains a valid, signed IMG4 Manifest and its
                      information has been returned into ManInfo.
  @retval other       An error has occured.

**/
DERReturn
DERImg4ParseManifest (
  DERImg4ManifestInfo  *ManInfo,
  const void           *ManifestBuffer,
  size_t               ManifestSize,
  uint32_t             ObjType
  );

#ifdef __cplusplus
}
#endif

#endif // LIB_DER_IMG4_H
