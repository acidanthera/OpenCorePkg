/** @file
  HMAC SHA-1 support for OcCryptoLib.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "CryptoInternal.h"

VOID
HmacSha1Init (
  IN HMAC_SHA1_CONTEXT  *Context,
  IN CONST UINT8        *Key,
  IN UINTN              KeyLen
  )
{
  ASSERT (Context != NULL);
  ASSERT (Key != NULL || KeyLen == 0);

  UINT8  KHash[SHA1_DIGEST_SIZE];
  UINT8  IKeyPad[SHA1_BLOCK_SIZE];

  //
  // If the key size exceeds the block size, it should be reduced through hashing
  //
  if (KeyLen > SHA1_BLOCK_SIZE) {
    SHA1_CONTEXT  Sha1Context;
    Sha1Init (&Sha1Context);
    Sha1Update (&Sha1Context, Key, KeyLen);
    Sha1Final (&Sha1Context, KHash);
    Key    = KHash;
    KeyLen = SHA1_DIGEST_SIZE;
  }

  SetMem (IKeyPad, SHA1_BLOCK_SIZE, 0);
  SetMem (Context->OKeyPad, SHA1_BLOCK_SIZE, 0);
  CopyMem (IKeyPad, Key, KeyLen);
  CopyMem (Context->OKeyPad, Key, KeyLen);

  for (UINTN i = 0; i < SHA1_BLOCK_SIZE; i++) {
    IKeyPad[i]          ^= 0x36;
    Context->OKeyPad[i] ^= 0x5c;
  }

  Sha1Init (&Context->Inner);
  Sha1Update (&Context->Inner, IKeyPad, SHA1_BLOCK_SIZE);

  ZeroMem (IKeyPad, sizeof (IKeyPad));
  ZeroMem (KHash, sizeof (KHash));
}

VOID
HmacSha1Update (
  IN OUT   HMAC_SHA1_CONTEXT  *Context,
  IN CONST UINT8              *Msg,
  IN       UINTN              MsgLen
  )
{
  Sha1Update (&Context->Inner, Msg, MsgLen);
}

VOID
HmacSha1Final (
  IN OUT HMAC_SHA1_CONTEXT  *Context,
  OUT    UINT8              *Dest
  )
{
  UINT8         InnerHash[SHA1_DIGEST_SIZE];
  SHA1_CONTEXT  OuterContext;

  Sha1Final (&Context->Inner, InnerHash);

  Sha1Init (&OuterContext);
  Sha1Update (&OuterContext, Context->OKeyPad, SHA1_BLOCK_SIZE);
  Sha1Update (&OuterContext, InnerHash, SHA1_DIGEST_SIZE);
  Sha1Final (&OuterContext, Dest);

  ZeroMem (InnerHash, sizeof (InnerHash));
  ZeroMem (Context, sizeof (*Context));
}

VOID
HmacSha1 (
  IN  CONST UINT8  *Key,
  IN  UINTN        KeyLen,
  IN  CONST UINT8  *Msg,
  IN  UINTN        MsgLen,
  OUT UINT8        *Dest
  )
{
  HMAC_SHA1_CONTEXT  Context;

  HmacSha1Init (&Context, Key, KeyLen);
  HmacSha1Update (&Context, Msg, MsgLen);
  HmacSha1Final (&Context, Dest);
}
