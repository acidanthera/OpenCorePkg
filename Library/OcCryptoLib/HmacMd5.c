/** @file
  HMAC MD5 support for OcCryptoLib.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "CryptoInternal.h"

VOID
HmacMd5Init (
  IN HMAC_MD5_CONTEXT  *Context,
  IN CONST UINT8       *Key,
  IN UINTN             KeyLen
  )
{
  ASSERT (Context != NULL);
  ASSERT (Key != NULL || KeyLen == 0);

  UINT8  KHash[MD5_DIGEST_SIZE];
  UINT8  IKeyPad[MD5_BLOCK_SIZE];

  //
  // If the key size exceeds the block size, it should be reduced through hashing
  //
  if (KeyLen > MD5_BLOCK_SIZE) {
    MD5_CONTEXT  Md5Context;
    Md5Init (&Md5Context);
    Md5Update (&Md5Context, Key, KeyLen);
    Md5Final (&Md5Context, KHash);
    Key    = KHash;
    KeyLen = MD5_DIGEST_SIZE;
  }

  SetMem (IKeyPad, MD5_BLOCK_SIZE, 0);
  SetMem (Context->OKeyPad, MD5_BLOCK_SIZE, 0);
  CopyMem (IKeyPad, Key, KeyLen);
  CopyMem (Context->OKeyPad, Key, KeyLen);

  for (UINTN i = 0; i < MD5_BLOCK_SIZE; i++) {
    IKeyPad[i]          ^= 0x36;
    Context->OKeyPad[i] ^= 0x5c;
  }

  Md5Init (&Context->Inner);
  Md5Update (&Context->Inner, IKeyPad, MD5_BLOCK_SIZE);

  ZeroMem (IKeyPad, sizeof (IKeyPad));
  ZeroMem (KHash, sizeof (KHash));
}

VOID
HmacMd5Update (
  IN OUT   HMAC_MD5_CONTEXT  *Context,
  IN CONST UINT8             *Msg,
  IN       UINTN             MsgLen
  )
{
  Md5Update (&Context->Inner, Msg, MsgLen);
}

VOID
HmacMd5Final (
  IN OUT HMAC_MD5_CONTEXT  *Context,
  OUT    UINT8             *Dest
  )
{
  UINT8        InnerHash[MD5_DIGEST_SIZE];
  MD5_CONTEXT  OuterContext;

  Md5Final (&Context->Inner, InnerHash);

  Md5Init (&OuterContext);
  Md5Update (&OuterContext, Context->OKeyPad, MD5_BLOCK_SIZE);
  Md5Update (&OuterContext, InnerHash, MD5_DIGEST_SIZE);
  Md5Final (&OuterContext, Dest);

  ZeroMem (InnerHash, sizeof (InnerHash));
  ZeroMem (Context, sizeof (*Context));
}

VOID
HmacMd5 (
  IN  CONST UINT8  *Key,
  IN  UINTN        KeyLen,
  IN  CONST UINT8  *Msg,
  IN  UINTN        MsgLen,
  OUT UINT8        *Dest
  )
{
  HMAC_MD5_CONTEXT  Context;

  HmacMd5Init (&Context, Key, KeyLen);
  HmacMd5Update (&Context, Msg, MsgLen);
  HmacMd5Final (&Context, Dest);
}
