#ifndef OC_CRYPTO_CONFIG_H
#define OC_CRYPTO_CONFIG_H

#include <Library/OcCryptoLib.h>

#define _PCD_GET_MODE_16_PcdOcCryptoAllowedSigHashTypes  \
  (1U << OcSigHashTypeSha256) | (1U << OcSigHashTypeSha384) | (1U << OcSigHashTypeSha512)

#define _PCD_GET_MODE_16_PcdOcCryptoAllowedRsaModuli  (512U | 256U)

#endif // OC_CRYPTO_CONFIG_H
