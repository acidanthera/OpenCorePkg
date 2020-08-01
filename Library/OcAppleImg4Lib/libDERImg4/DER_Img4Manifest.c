/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "libDER/libDER.h"
#include "libDER/asn1Types.h"
#include "libDER/DER_CertCrl.h"
#include "libDER/DER_Decode.h"
#include "libDER/DER_Keys.h"
#include "libDER/oids.h"

#include "libDERImg4.h"
#include "DER_Img4Manifest.h"
#include "Img4oids.h"

typedef struct {
  DERItem          certItem;
  DERSignedCertCrl certCtrl;
  DERTBSCert       tbsCert;
  DERItem          pubKeyItem;
  DERItem          manCertRoleItem;
} Image4CertificateInfo;

enum {
  Img4ManifestPropSetTypeManp = 0,
  Img4ManifestPropSetTypeObjp = 1
};

bool
DERItemCompare (
  const DERItem  *Item1,
  const DERItem  *Item2
  )
{
  assert (Item1 != NULL);
  assert (Item2 != NULL);

  if (Item1->length != Item2->length) {
    return false;
  }

  return DERMemcmp (Item1->data, Item2->data, Item1->length) == 0;
}

bool
DERItemNull (
  const DERItem  *Item
  )
{
  assert (Item != NULL);

  if (Item->length == 2 && Item->data[0] == ASN1_NULL && Item->data[1] == 0) {
    return true;
  } else if (Item->length == 0) {
    return true;
  }

  return false;
}

DERReturn
DERImg4ParseInteger64 (
  const DERItem  *Contents,
  uint64_t       *Result
  )
{
  DERSize  Index;
  DERSize  Length;
  uint64_t Value;

  assert (Contents != NULL);
  assert (Result   != NULL);

  Length = Contents->length;
  if (Length == 0 || (Contents->data[0] & 0x80U) != 0) {
    return DR_DecodeError;
  }

  Value = Contents->data[0];
  if (Length == 1) {
    *Result = Value;
    return DR_Success;
  }
  //
  // It is not perfectly clear why the different sizes are allowed in this way.
  // The original libDER only supports 32-bit integers and I assume bit
  // strings, which have their first byte signal the number of unused bits,
  // might have been or are used to implement 64-bit integers. In this case,
  // the number of unused bits must be 0 and this byte is discarded.
  //
  if (Length > 9 || (Value != 0 && Length > 8)) {
    return DR_BufOverflow;
  }

  if (Value == 0 && (Contents->data[1] & 0x80U) == 0) {
    return DR_DecodeError;
  }

  for (Index = 1; Index < Length; ++Index) {
    Value <<= 8U;
    Value += Contents->data[Index];
  }

  *Result = Value;
  return DR_Success;
}

DERReturn
DERImg4ParseInteger32 (
  const DERItem   *Contents,
  uint32_t        *Result
  )
{
  DERReturn DerResult;
  uint64_t  Result64;

  assert (Contents != NULL);
  assert (Result != NULL);

  DerResult = DERImg4ParseInteger64 (Contents, &Result64);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if ((Result64 & 0xFFFFFFFF80000000U) != 0) {
    return DR_BufOverflow;
  }

  *Result = (uint32_t)Result64;
  return DR_Success;
}

DERReturn
DERVerifySignature (
  const DERItem  *PubKeyItem,
  const DERItem  *SigItem,
  const DERItem  *DataItem,
  const DERItem  *algoOid
  )
{
  bool              Result;
  DERReturn         DerResult;

  DERRSAPubKeyPKCS1 PkPkcs1;

  DERItem           ModulusItem;
  DERByte           NumUnusedBits;

  uint32_t          Exponent;

  assert (PubKeyItem != NULL);
  assert (SigItem    != NULL);
  assert (DataItem   != NULL);
  assert (algoOid    != NULL);

  DerResult = DERParseSequence (
                PubKeyItem,
                DERNumRSAPubKeyPKCS1ItemSpecs,
                DERRSAPubKeyPKCS1ItemSpecs,
                &PkPkcs1,
                sizeof (PkPkcs1)
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  DerResult = DERParseBitString (
                &PkPkcs1.modulus,
                &ModulusItem,
                &NumUnusedBits
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (NumUnusedBits != 0) {
    return DR_DecodeError;
  }

  DerResult = DERImg4ParseInteger32 (&PkPkcs1.pubExponent, &Exponent);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  Result = DERImg4VerifySignature (
             ModulusItem.data,
             ModulusItem.length,
             Exponent,
             SigItem->data,
             SigItem->length,
             DataItem->data,
             DataItem->length,
             algoOid
             );
  if (!Result) {
    return (DERReturn)DR_SecurityError;
  }

  return DR_Success;
}

static
DERReturn
DERImg4ManifestVerifyMagic (
  const DERItem  *ManItem
  )
{
  DERReturn DerResult;
  uint32_t  ItemTag;

  assert (ManItem != NULL);

  if (ManItem->length != sizeof (ItemTag)) {
    return DR_DecodeError;
  }

  DerResult = DERImg4ParseInteger32 (ManItem, &ItemTag);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (ItemTag != DER_IMG4_TAG_MAN_MAGIC) {
    return DR_UnexpectedTag;
  }

  return DR_Success;
}

static
DERReturn
DERImg4ManifestCollectCertInfo (
  Image4CertificateInfo  *CertInfo
  )
{
  DERReturn         DerResult;

  DERTag            CertTbsTag;
  DERSubjPubKeyInfo CertPubKeyInfo;
  DERAlgorithmId    CertSigAlgoId;

  DERSequence       CertTbsSeq;
  DERDecodedInfo    CertTbsSeqInfo;
  DERDecodedInfo    CertExtValueInfo;
  DERExtension      TbsCertExt;

  DERByte           NumUnusedBits;

  assert (CertInfo != NULL);

  DerResult = DERParseSequence (
                &CertInfo->certItem,
                DERNumSignedCertCrlItemSpecs,
                DERSignedCertCrlItemSpecs,
                &CertInfo->certCtrl,
                sizeof (CertInfo->certCtrl)
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  DerResult = DERParseSequence (
                &CertInfo->certCtrl.tbs,
                DERNumTBSCertItemSpecs,
                DERTBSCertItemSpecs,
                &CertInfo->tbsCert,
                sizeof (CertInfo->tbsCert)
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  DerResult = DERParseSequenceContent (
                &CertInfo->tbsCert.subjectPubKey,
                DERNumSubjPubKeyInfoItemSpecs,
                DERSubjPubKeyInfoItemSpecs,
                &CertPubKeyInfo,
                sizeof (CertPubKeyInfo)
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  DerResult = DERParseSequenceContent (
                &CertPubKeyInfo.algId,
                DERNumAlgorithmIdItemSpecs,
                DERAlgorithmIdItemSpecs,
                &CertSigAlgoId,
                sizeof (CertSigAlgoId)
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (!DEROidCompare (&CertSigAlgoId.oid, &oidRsa)
   || !DERItemNull (&CertSigAlgoId.params)) {
    return (DERReturn)DR_SecurityError;
  }

  DerResult = DERParseBitString (
                &CertPubKeyInfo.pubKey,
                &CertInfo->pubKeyItem,
                &NumUnusedBits
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (NumUnusedBits != 0) {
    return DR_DecodeError;
  }
  //
  // The Manifest Certificate Role is only required to be declared by the last
  // certificate in the Manifest Certificate Chain.
  //
  CertInfo->manCertRoleItem.data   = NULL;
  CertInfo->manCertRoleItem.length = 0;
  if (CertInfo->tbsCert.extensions.length != 0) {
    DerResult = DERDecodeSeqInit (
                  &CertInfo->tbsCert.extensions,
                  &CertTbsTag,
                  &CertTbsSeq
                  );
    if (DerResult != DR_Success) {
      return DerResult;
    }

    if (CertTbsTag != ASN1_CONSTR_SEQUENCE) {
      return DR_UnexpectedTag;
    }

    while (DERDecodeSeqNext (&CertTbsSeq, &CertTbsSeqInfo) == DR_Success) {
      if (CertTbsSeqInfo.tag != ASN1_CONSTR_SEQUENCE) {
        return DR_UnexpectedTag;
      }

      DerResult = DERParseSequenceContent (
                    &CertTbsSeqInfo.content,
                    DERNumExtensionItemSpecs,
                    DERExtensionItemSpecs,
                    &TbsCertExt,
                    sizeof (TbsCertExt)
                    );
      if (DerResult != DR_Success) {
        return DerResult;
      }

      if (DEROidCompare (&TbsCertExt.extnID, &oidAppleImg4ManifestCertSpec)) {
        DerResult = DERDecodeItem (&TbsCertExt.extnValue, &CertExtValueInfo);
        if (DerResult != DR_Success) {
          return DerResult;
        }

        if (CertExtValueInfo.tag != ASN1_CONSTR_SET) {
          return DR_UnexpectedTag;
        }

        CertInfo->manCertRoleItem.data   = TbsCertExt.extnValue.data;
        CertInfo->manCertRoleItem.length = TbsCertExt.extnValue.length;
        break;
      }
    }
  }

  return DR_Success;
}

DERReturn
DERImg4ManifestVerifyCertIssuer (
  const Image4CertificateInfo  *ChildCertInfo,
  const Image4CertificateInfo  *ParentCertInfo
  )
{
  bool           Result;
  DERReturn      DerResult;

  DERAlgorithmId CertSigAlgoId;
  DERItem        CertSigItem;

  DERByte        NumUnusedBits;

  assert (ChildCertInfo != NULL);
  assert (ParentCertInfo != NULL);

  Result = DERItemCompare (
             &ChildCertInfo->tbsCert.issuer,
             &ParentCertInfo->tbsCert.subject
             );
  if (!Result) {
    return (DERReturn)DR_SecurityError;
  }

  DerResult = DERParseSequenceContent (
                &ChildCertInfo->certCtrl.sigAlg,
                DERNumAlgorithmIdItemSpecs,
                DERAlgorithmIdItemSpecs,
                &CertSigAlgoId,
                sizeof (CertSigAlgoId)
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (!DERItemNull (&CertSigAlgoId.params)) {
    return (DERReturn)DR_SecurityError;
  }

  DerResult = DERParseBitString (
                &ChildCertInfo->certCtrl.sig,
                &CertSigItem,
                &NumUnusedBits
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (NumUnusedBits != 0) {
    return DR_DecodeError;
  }

  DerResult = DERVerifySignature (
                &ParentCertInfo->pubKeyItem,
                &CertSigItem,
                &ChildCertInfo->certCtrl.tbs,
                &CertSigAlgoId.oid
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  return DR_Success;
}

DERReturn
DERImg4ManifestVerifySignature (
  DERItem                  *ManCertRoleItem,
  const DERImg4Manifest    *Manifest
  )
{
  DERReturn             DerResult;

  Image4CertificateInfo ParentCertInfo;
  Image4CertificateInfo CurCertInfo;

  DERItem               CertItem;
  DERDecodedInfo        CertInfo;
  DERByte               *NextCert;
  DERByte               *CertWalker;
  DERSize               LeftCertSize;
  DERSize               CertSize;
  DERSize               Index;
  
  DERAlgorithmId        CertSigAlgoId;

  assert (ManCertRoleItem != NULL);
  assert (Manifest != NULL);
  //
  // Verify the Manifest certification chain.
  //
  if (Manifest->certificates.length == 0) {
    return DR_DecodeError;
  }
  //
  // These pointers must be provided by the platform integrator, either by an
  // external variable declaration or by a macro.
  //
  assert (DERImg4RootCertificate != NULL);
  assert (DERImg4RootCertificateSize != NULL);

  CurCertInfo.certItem.data   = (DERByte *)DERImg4RootCertificate;
  CurCertInfo.certItem.length = *DERImg4RootCertificateSize;
  DerResult = DERImg4ManifestCollectCertInfo (&CurCertInfo);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  CertWalker = Manifest->certificates.data;
  for (
    Index = 1, LeftCertSize = Manifest->certificates.length;
    Index < DER_IMG4_MAN_CERT_CHAIN_MAX && LeftCertSize > 0;
    ++Index, LeftCertSize -= CertSize
    ) {
    CopyMem (&ParentCertInfo, &CurCertInfo, sizeof (ParentCertInfo));

    CertItem.data   = CertWalker;
    CertItem.length = LeftCertSize;
    DerResult = DERDecodeItem (&CertItem, &CertInfo);
    if (DerResult != DR_Success) {
      return DerResult;
    }

    NextCert = &CertInfo.content.data[CertInfo.content.length];
    CertSize = NextCert - CertWalker;
    //
    // This is a guarantee libDER is supposed to make.
    //
    assert (CertSize <= LeftCertSize);
    if (CertSize > DER_IMG4_MAX_CERT_SIZE) {
      return DR_DecodeError;
    }

    CurCertInfo.certItem.data   = CertWalker;
    CurCertInfo.certItem.length = CertSize;

    CertWalker = NextCert;

    DerResult = DERImg4ManifestCollectCertInfo (&CurCertInfo);
    if (DerResult != DR_Success) {
      return DerResult;
    }

    DerResult = DERImg4ManifestVerifyCertIssuer (
                  &CurCertInfo,
                  &ParentCertInfo
                  );
    if (DerResult != DR_Success) {
      return DerResult;
    }
  }

  if (LeftCertSize != 0) {
    return DR_DecodeError;
  }

  if (CurCertInfo.manCertRoleItem.data == NULL
   || CurCertInfo.manCertRoleItem.length == 0) {
    return DR_DecodeError;
  }
  //
  // Verify the Manifest body signature.
  //
  DerResult = DERParseSequenceContent (
                &CurCertInfo.tbsCert.tbsSigAlg,
                DERNumAlgorithmIdItemSpecs,
                DERAlgorithmIdItemSpecs,
                &CertSigAlgoId,
                sizeof (CertSigAlgoId)
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (!DERItemNull (&CertSigAlgoId.params)) {
    return (DERReturn)DR_SecurityError;
  }

  DerResult = DERVerifySignature (
                &CurCertInfo.pubKeyItem,
                &Manifest->signature,
                &Manifest->body,
                &CertSigAlgoId.oid
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  ManCertRoleItem->data   = CurCertInfo.manCertRoleItem.data;
  ManCertRoleItem->length = CurCertInfo.manCertRoleItem.length;
  return DR_Success;
}

DERReturn
DERImg4FindPropertyItem (
  const DERItem  *PropSetItem,
  DERTag         PropName,
  DERItem        *PropItem
  )
{
  DERReturn      DerResult;

  DERSequence    PropSetSeq;
  DERDecodedInfo PropInfo;

  assert (PropSetItem != NULL);
  assert (PropItem != NULL);

  DerResult = DERDecodeSeqContentInit (PropSetItem, &PropSetSeq);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  while ((DerResult = DERDecodeSeqNext (&PropSetSeq, &PropInfo)) == DR_Success) {
    if (PropInfo.tag == PropName) {
      PropItem->length = PropInfo.content.length;
      PropItem->data   = PropInfo.content.data;
      return DR_Success;
    }
  }

  return DerResult;
}

DERReturn
DERImg4FindDecodeProperty (
  const DERItem    *PropSetItem,
  DERTag           PropName,
  DERTag           PropValueTag,
  DERImg4Property  *Property
  )
{
  STATIC CONST DERItemSpec PropertyItemSpecTpl[] = DER_IMG4_PROPERTY_SPEC_INIT;

  DERReturn   DerResult;
  DERItem     PropItem;
  uint32_t    CurPropTag;
  DERItemSpec PropertyItemSpec[ARRAY_SIZE (PropertyItemSpecTpl)];

  CopyMem (PropertyItemSpec, PropertyItemSpecTpl, sizeof (PropertyItemSpecTpl));

  PropertyItemSpec[1].tag = PropValueTag;

  assert (PropSetItem != NULL);
  assert (Property != NULL);

  DerResult = DERImg4FindPropertyItem (
                PropSetItem,
                PropName,
                &PropItem
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  DerResult = DERParseSequence (
                &PropItem,
                ARRAY_SIZE (PropertyItemSpec),
                PropertyItemSpec,
                Property,
                0
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  DerResult = DERImg4ParseInteger32 (&Property->nameItem, &CurPropTag);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (DER_IMG4_ENCODE_PROPERTY_NAME (CurPropTag) != PropName) {
    return DR_UnexpectedTag;
  }

  Property->nameTag  = PropName;
  Property->valueTag = PropValueTag;
  return DR_Success;
}

DERReturn
DERImg4DecodeProperty (
  const DERItem    *PropItem,
  DERTag           PropName,
  DERImg4Property  *Property
  )
{
  DERReturn      DerResult;

  DERTag         PropTag;
  DERSequence    PropSeq;

  DERDecodedInfo PropNameInfo;
  uint32_t       PropNameTag;

  DERDecodedInfo PropValueInfo;
  DERDecodedInfo PropTailInfo;

  assert (PropItem != NULL);
  assert (Property != NULL);

  DerResult = DERDecodeSeqInit (PropItem, &PropTag, &PropSeq);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (PropTag != ASN1_CONSTR_SEQUENCE) {
    return DR_UnexpectedTag;
  }

  DerResult = DERDecodeSeqNext (&PropSeq, &PropNameInfo);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (PropNameInfo.tag != ASN1_IA5_STRING) {
    return DR_UnexpectedTag;
  }
  
  DerResult = DERImg4ParseInteger32 (&PropNameInfo.content, &PropNameTag);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (DER_IMG4_ENCODE_PROPERTY_NAME (PropNameTag) != PropName) {
    return DR_UnexpectedTag;
  }

  DerResult = DERDecodeSeqNext (&PropSeq, &PropValueInfo);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  DerResult = DERDecodeSeqNext (&PropSeq, &PropTailInfo);
  if (DerResult != DR_EndOfSequence) {
    return DR_DecodeError;
  }
  
  Property->nameTag         = PropName;
  Property->nameItem.data   = PropNameInfo.content.data;
  Property->nameItem.length = PropNameInfo.content.length;

  Property->valueTag         = PropValueInfo.tag;
  Property->valueItem.data   = PropValueInfo.content.data;
  Property->valueItem.length = PropValueInfo.content.length;
  return DR_Success;
}

DERReturn
DERImg4ManifestDecodePropertyBoolean (
  const DERItem  *PropItem,
  uint32_t       PropName,
  bool           *Value
  )
{
  DERReturn       DerResult;
  DERImg4Property Property;

  assert (PropItem != NULL);
  assert (Value != NULL);

  DerResult = DERImg4DecodeProperty (
                PropItem,
                DER_IMG4_ENCODE_PROPERTY_NAME (PropName),
                &Property
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (Property.valueTag != ASN1_BOOLEAN) {
    return DR_UnexpectedTag;
  }

  if (Property.valueItem.length != 1
   || (*Property.valueItem.data != 0 && *Property.valueItem.data != 0xFF)) {
    return DR_DecodeError;
  }

  *Value = *Property.valueItem.data != 0;
  return DR_Success;
}

DERReturn
DERImg4DecodePropertyInteger32 (
  const DERItem  *PropItem,
  uint32_t       PropName,
  uint32_t       *Value
  )
{
  DERReturn       DerResult;
  DERImg4Property Property;

  assert (PropItem != NULL);
  assert (Value != NULL);

  DerResult = DERImg4DecodeProperty (
                PropItem,
                DER_IMG4_ENCODE_PROPERTY_NAME (PropName),
                &Property
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (Property.valueTag != ASN1_INTEGER) {
    return DR_UnexpectedTag;
  }
  
  return DERImg4ParseInteger32 (&Property.valueItem, Value);
}

DERReturn
DERImg4DecodePropertyInteger64 (
  const DERItem  *PropItem,
  uint32_t       PropName,
  uint64_t       *Value
  )
{
  DERReturn       DerResult;
  DERImg4Property Property;

  assert (PropItem != NULL);
  assert (Value != NULL);

  DerResult = DERImg4DecodeProperty (
                PropItem,
                DER_IMG4_ENCODE_PROPERTY_NAME (PropName),
                &Property
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (Property.valueTag != ASN1_INTEGER) {
    return DR_UnexpectedTag;
  }
  
  return DERImg4ParseInteger64 (&Property.valueItem, Value);
}

DERReturn
DERImg4ValidateCertificateRole (
  const DERItem  *ManPropSetItem,
  const DERItem  *ObjPropSetItem,
  const DERItem  *ManBodyCertItem
  )
{
  DERReturn       DerResult;
  DERReturn       LoopDerResult;
  DERReturn       LoopDerResult2;

  DERTag          ManBodyCertTag;
  DERSequence     ManBodyCertSet;

  DERDecodedInfo  CertPropSetInfo;
  DERImg4Property CertPropSet;
  DERSequence     CertPropSetSeq;

  DERDecodedInfo  CertPropInfo;
  DERImg4Property CurCertProp;

  DERItem         BodyPropSetItem;
  DERItem         BodyPropItem;

  assert (ManPropSetItem != NULL);
  assert (ObjPropSetItem != NULL);
  assert (ManBodyCertItem != NULL);

  DerResult = DERDecodeSeqInit (
                ManBodyCertItem,
                &ManBodyCertTag,
                &ManBodyCertSet
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (ManBodyCertTag != ASN1_CONSTR_SET) {
    return DR_UnexpectedTag;
  }

  while ((LoopDerResult = DERDecodeSeqNext (&ManBodyCertSet, &CertPropSetInfo)) == DR_Success) {
    if (CertPropSetInfo.tag == DER_IMG4_ENCODE_PROPERTY_NAME (DER_IMG4_TAG_OBJ_PROPS)) {
      BodyPropSetItem.data   = ObjPropSetItem->data;
      BodyPropSetItem.length = ObjPropSetItem->length;
    } else if (CertPropSetInfo.tag == DER_IMG4_ENCODE_PROPERTY_NAME (DER_IMG4_TAG_MAN_PROPS)) {
      BodyPropSetItem.data   = ManPropSetItem->data;
      BodyPropSetItem.length = ManPropSetItem->length;
    } else {
      return DR_UnexpectedTag;
    }

    DerResult = DERImg4DecodeProperty (
                  &CertPropSetInfo.content,
                  CertPropSetInfo.tag,
                  &CertPropSet
                  );
    if (DerResult != DR_Success) {
      return DerResult;
    }

    if (CertPropSet.valueTag != ASN1_CONSTR_SET) {
      return DR_UnexpectedTag;
    }

    DerResult = DERDecodeSeqContentInit (
                  &CertPropSet.valueItem,
                  &CertPropSetSeq
                  );
    if (DerResult != DR_Success) {
      return DerResult;
    }

    while ((LoopDerResult2 = DERDecodeSeqNext (&CertPropSetSeq, &CertPropInfo)) == DR_Success) {
      DerResult = DERImg4DecodeProperty (
                    &CertPropInfo.content,
                    CertPropInfo.tag,
                    &CurCertProp
                    );
      if (DerResult != DR_Success) {
        return DerResult;
      }

      DerResult = DERImg4FindPropertyItem (
                    &BodyPropSetItem,
                    CertPropInfo.tag,
                    &BodyPropItem
                    );
      if (CurCertProp.valueTag == ASN1_BOOLEAN
       || CurCertProp.valueTag == ASN1_INTEGER
       || CurCertProp.valueTag == ASN1_OCTET_STRING) {
        //
        // The specified key must be present with the exact role value.
        //
        if (DerResult != DR_Success) {
          return DerResult;
        }

        if (!DERItemCompare (&CertPropInfo.content, &BodyPropItem)) {
          return (DERReturn)DR_SecurityError;
        }
      } else if (CurCertProp.valueTag == (ASN1_CONSTRUCTED | ASN1_CONTEXT_SPECIFIC | 0)) {
        //
        // The specified key must be present with an arbitrary value.
        //
        if (DerResult != DR_Success) {
          return DerResult;
        }
      } else if (CurCertProp.valueTag == (ASN1_CONSTRUCTED | ASN1_CONTEXT_SPECIFIC | 1)) {
        //
        // The specified key must not be present.
        //
        if (DerResult != DR_EndOfSequence) {
          return DR_UnexpectedTag;
        }
      } else {
        //
        // An unexpected tag has been encountered.
        //
        return DR_UnexpectedTag;
      }
    }

    if (LoopDerResult2 != DR_EndOfSequence) {
      return DR_DecodeError;
    }
  }

  if (LoopDerResult != DR_EndOfSequence) {
    return LoopDerResult;
  }

  return DR_Success;
}

DERReturn
DERImg4ManifestDecodeProperty (
  const DERItem        *PropItem,
  uint32_t             PropName,
  DERImg4ManifestInfo  *ManInfo,
  uint32_t             PropSetType
  )
{
  DERReturn       DerResult;
  DERImg4Property Property;
  uint64_t        Ecid;

  assert (PropItem != NULL);
  assert (ManInfo != NULL);

  if (PropSetType == Img4ManifestPropSetTypeObjp) {
    switch (PropName) {
      case DER_IMG4_TAG_OBJ_EPRO:
      {
        ManInfo->hasEffectiveProductionStatus = true;
        return DERImg4ManifestDecodePropertyBoolean (
                 PropItem,
                 PropName,
                 &ManInfo->environment.effectiveProductionStatus
                 );
      }

      case DER_IMG4_TAG_OBJ_ESEC:
      {
        ManInfo->hasEffectiveSecurityMode = true;
        return DERImg4ManifestDecodePropertyBoolean (
                 PropItem,
                 PropName,
                 &ManInfo->environment.effectiveSecurityMode
                 );
      }

      case DER_IMG4_TAG_OBJ_DGST:
      {
        DerResult = DERImg4DecodeProperty (
                      PropItem,
                      DER_IMG4_ENCODE_PROPERTY_NAME (DER_IMG4_TAG_OBJ_DGST),
                      &Property
                      );
        if (DerResult != DR_Success) {
          return DerResult;
        }

        if (Property.valueTag != ASN1_OCTET_STRING) {
          return DR_UnexpectedTag;
        }

        if (Property.valueItem.length > sizeof (ManInfo->imageDigest)) {
          return DR_BufOverflow;
        }

        if (Property.valueItem.length == 0) {
          return DR_DecodeError;
        }

        DERMemmove (
          ManInfo->imageDigest,
          Property.valueItem.data,
          Property.valueItem.length
          );
        ManInfo->imageDigestSize = Property.valueItem.length;

        return DR_Success;
      }

      case DER_IMG4_TAG_OBJ_EKEY:
      {
        return DERImg4ManifestDecodePropertyBoolean (
                 PropItem,
                 PropName,
                 &ManInfo->environment.enableKeys
                 );
      }

      default:
      {
        //
        // For forward compatibility, ignore unknown keys.
        //
        return DR_Success;
      }
    }
  } else {
    //
    // The conditional branches must be modified when more types are added.
    //
    assert (PropSetType == Img4ManifestPropSetTypeManp);

    switch (PropName) {
      case DER_IMG4_TAG_MAN_CEPO:
      {
        return DERImg4DecodePropertyInteger32 (
                 PropItem,
                 PropName,
                 &ManInfo->environment.certificateEpoch
                 );
      }

      case DER_IMG4_TAG_MAN_ECID:
      {
        ManInfo->hasEcid = true;
        DerResult = DERImg4DecodePropertyInteger64 (PropItem, PropName, &Ecid);
        if (DerResult == DR_Success) {
          ManInfo->environment.ecid = Ecid;
        }

        return DerResult;
      }

      case DER_IMG4_TAG_MAN_CHIP:
      {
        return DERImg4DecodePropertyInteger32 (
                 PropItem,
                 PropName,
                 &ManInfo->environment.chipId
                 );
      }

      case DER_IMG4_TAG_MAN_BORD:
      {
        return DERImg4DecodePropertyInteger32 (
                 PropItem,
                 PropName,
                 &ManInfo->environment.boardId
                 );
      }

      case DER_IMG4_TAG_MAN_AMNM:
      {
        return DERImg4DecodePropertyInteger32 (
                 PropItem,
                 PropName,
                 &ManInfo->environment.allowMixNMatch
                 );
      }

      case DER_IMG4_TAG_MAN_SDOM:
      {
        return DERImg4DecodePropertyInteger32 (
                 PropItem,
                 PropName,
                 &ManInfo->environment.securityDomain
                 );
      }

      case DER_IMG4_TAG_MAN_IUOB:
      {
        return DERImg4ManifestDecodePropertyBoolean (
                 PropItem,
                 PropName,
                 &ManInfo->environment.internalUseOnlyUnit
                 );
      }

      case DER_IMG4_TAG_MAN_MPRO:
      {
        return DERImg4ManifestDecodePropertyBoolean (
                 PropItem,
                 PropName,
                 &ManInfo->environment.productionStatus
                 );
      }

      case DER_IMG4_TAG_MAN_MSEC:
      {
        return DERImg4ManifestDecodePropertyBoolean (
                 PropItem,
                 PropName,
                 &ManInfo->environment.securityMode
                 );
      }

      case DER_IMG4_TAG_MAN_XUGS:
      {
        ManInfo->hasXugs = true;
        return DERImg4DecodePropertyInteger32 (
                 PropItem,
                 PropName,
                 &ManInfo->environment.xugs
                 );
      }

      default:
      {
        //
        // For forward compatibility, ignore unknown keys.
        //
        return DR_Success;
      }
    }
  }
}

DERReturn
DERImg4ManifestDecodeProperties (
  DERImg4ManifestInfo  *ManInfo,
  const DERItem        *PropSetItem,
  uint32_t             PropSetType
  )
{
  DERReturn       DerResult;
  DERReturn       LoopDerResult;

  DERSequence     PropSetSeq;
  DERDecodedInfo  PropInfo;
  DERImg4Property Property;

  assert (ManInfo != NULL);
  assert (PropSetItem != NULL);

  DerResult = DERDecodeSeqContentInit (PropSetItem, &PropSetSeq);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  while ((LoopDerResult = DERDecodeSeqNext (&PropSetSeq, &PropInfo)) == DR_Success) {
    DerResult = DERImg4DecodeProperty (
                  &PropInfo.content,
                  PropInfo.tag,
                  &Property
                  );
    if (DerResult != DR_Success) {
      return DerResult;
    }

    if (Property.valueTag != ASN1_BOOLEAN
     && Property.valueTag != ASN1_INTEGER
     && Property.valueTag != ASN1_OCTET_STRING
     && Property.valueTag != ASN1_IA5_STRING
      ) {
      return DR_UnexpectedTag;
    }

    if ((PropInfo.tag & ASN1_CLASS_MASK) == 0
     || (PropInfo.tag & ASN1_CONSTRUCTED) == 0) {
      return DR_UnexpectedTag;
    }

    DerResult = DERImg4ManifestDecodeProperty (
                  &PropInfo.content,
                  (uint32_t)PropInfo.tag,
                  ManInfo,
                  PropSetType
                  );
    if (DerResult != DR_Success) {
      return DerResult;
    }
  }

  if (LoopDerResult != DR_EndOfSequence) {
    return LoopDerResult;
  }

  return DR_Success;
}

/**
  Verify and parse the IMG4 Manifest in ManBuffer and output its information.
  On success, the Manifest is guaranteed to be digitally signed with the
  platform-provided DERImg4RootCertificate.

  @param[out] ManInfo    Output Manifest information structure.
  @param[in]  ManBuffer  Buffer containing the Manifest data.
  @param[in]  ManSize    Size, in bytes, of ManBuffer.
  @param[in]  ObjType    The object type to inspect.

  @retval DR_Success  ManBuffer contains a valid, signed IMG4 Manifest and its
                      information has been returned into ManInfo.
  @retval other       An error has occured.

**/
DERReturn
DERImg4ParseManifest (
  DERImg4ManifestInfo  *ManInfo,
  const void           *ManBuffer,
  size_t               ManSize,
  uint32_t             ObjType
  )
{
  DERReturn       DerResult;

  DERItem         ManifestItem;
  DERSize         DerManifestSize;

  DERDecodedInfo  ManifestInfo;
  DERImg4Manifest Manifest;
  uint32_t        ManVersion;

  DERDecodedInfo  ManBodyInfo;
  DERImg4Property ManBodyProp;

  DERImg4Property ManPropSetProp;
  DERImg4Property ObjPropSetProp;

  DERItem         ManBodyCertRoleItem;

  assert (ManInfo != NULL);
  assert (ManBuffer != NULL);
  assert (ManSize > 0);
  //
  // Verify the Manifest header integrity.
  //
  ManifestItem.data   = (DERByte *)ManBuffer;
  ManifestItem.length = ManSize;
  DerResult = DERDecodeItem (&ManifestItem, &ManifestInfo);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (ManifestInfo.tag != ASN1_CONSTR_SEQUENCE) {
    return DR_UnexpectedTag;
  }
  //
  // This is a guarantee libDER is supposed to make.
  //
  assert (ManifestInfo.content.data >= (DERByte *)ManBuffer);

  DerManifestSize  = (DERByte *)ManifestInfo.content.data - (DERByte *)ManBuffer;
  DerManifestSize += ManifestInfo.content.length;
  if (DerManifestSize != ManSize) {
    return DR_DecodeError;
  }

  DerResult = DERParseSequenceContent (
                &ManifestInfo.content,
                DERNumImg4ManifestItemSpecs,
                DERImg4ManifestItemSpecs,
                &Manifest,
                0
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (Manifest.body.data == NULL) {
    return DR_DecodeError;
  }

  DerResult = DERImg4ManifestVerifyMagic (&Manifest.magic);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  DerResult = DERImg4ParseInteger32 (&Manifest.version, &ManVersion);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (ManVersion != 0) {
    return DR_DecodeError;
  }
  //
  // Verify the Manifest body.
  //
  DerResult = DERImg4ManifestVerifySignature (&ManBodyCertRoleItem, &Manifest);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  DerResult = DERDecodeItem (&Manifest.body, &ManBodyInfo);
  if (DerResult != DR_Success) {
    return DerResult;
  }

  if (ManBodyInfo.tag != ASN1_CONSTR_SET) {
    return DR_UnexpectedTag;
  }

  DerResult = DERImg4FindDecodeProperty (
                &ManBodyInfo.content,
                DER_IMG4_ENCODE_PROPERTY_NAME (DER_IMG4_TAG_MAN_BODY),
                ASN1_CONSTR_SET,
                &ManBodyProp
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  DerResult = DERImg4FindDecodeProperty (
                &ManBodyProp.valueItem,
                DER_IMG4_ENCODE_PROPERTY_NAME (DER_IMG4_TAG_MAN_PROPS),
                ASN1_CONSTR_SET,
                &ManPropSetProp
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  DerResult = DERImg4FindDecodeProperty (
                &ManBodyProp.valueItem,
                DER_IMG4_ENCODE_PROPERTY_NAME (ObjType),
                ASN1_CONSTR_SET,
                &ObjPropSetProp
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  DerResult = DERImg4ValidateCertificateRole (
                &ManPropSetProp.valueItem,
                &ObjPropSetProp.valueItem,
                &ManBodyCertRoleItem
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  DERMemset (ManInfo, 0, sizeof (*ManInfo));
  //
  // Decode the Manifest body.
  //
  DerResult = DERImg4ManifestDecodeProperties (
                ManInfo,
                &ManPropSetProp.valueItem,
                Img4ManifestPropSetTypeManp
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  DerResult = DERImg4ManifestDecodeProperties (
                ManInfo,
                &ObjPropSetProp.valueItem,
                Img4ManifestPropSetTypeObjp
                );
  if (DerResult != DR_Success) {
    return DerResult;
  }

  return DR_Success;
}
