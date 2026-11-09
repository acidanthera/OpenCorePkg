/** @file
  HMAC SHA-256/384/512 support for OcCryptoLib.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "CryptoInternal.h"

VOID
HmacSha256Init (
  IN HMAC_SHA256_CONTEXT  *Context,
  IN CONST UINT8          *Key,
  IN UINTN                KeyLen
  )
{
  ASSERT (Context != NULL);
  ASSERT (Key != NULL || KeyLen == 0);

  UINT8  KHash[SHA256_DIGEST_SIZE];
  UINT8  IKeyPad[SHA256_BLOCK_SIZE];

  //
  // If the key size exceeds the block size, it should be reduced through hashing
  //
  if (KeyLen > SHA256_BLOCK_SIZE) {
    SHA256_CONTEXT  Sha256Context;
    Sha256Init (&Sha256Context);
    Sha256Update (&Sha256Context, Key, KeyLen);
    Sha256Final (&Sha256Context, KHash);
    Key    = KHash;
    KeyLen = SHA256_DIGEST_SIZE;
  }

  SetMem (IKeyPad, SHA256_BLOCK_SIZE, 0);
  SetMem (Context->OKeyPad, SHA256_BLOCK_SIZE, 0);
  CopyMem (IKeyPad, Key, KeyLen);
  CopyMem (Context->OKeyPad, Key, KeyLen);

  for (UINTN i = 0; i < SHA256_BLOCK_SIZE; i++) {
    IKeyPad[i]          ^= 0x36;
    Context->OKeyPad[i] ^= 0x5c;
  }

  Sha256Init (&Context->Inner);
  Sha256Update (&Context->Inner, IKeyPad, SHA256_BLOCK_SIZE);

  ZeroMem (IKeyPad, sizeof (IKeyPad));
  ZeroMem (KHash, sizeof (KHash));
}

VOID
HmacSha256Update (
  IN OUT   HMAC_SHA256_CONTEXT  *Context,
  IN CONST UINT8                *Msg,
  IN       UINTN                MsgLen
  )
{
  Sha256Update (&Context->Inner, Msg, MsgLen);
}

VOID
HmacSha256Final (
  IN OUT HMAC_SHA256_CONTEXT  *Context,
  OUT    UINT8                *Dest
  )
{
  UINT8           InnerHash[SHA256_DIGEST_SIZE];
  SHA256_CONTEXT  OuterContext;

  Sha256Final (&Context->Inner, InnerHash);

  Sha256Init (&OuterContext);
  Sha256Update (&OuterContext, Context->OKeyPad, SHA256_BLOCK_SIZE);
  Sha256Update (&OuterContext, InnerHash, SHA256_DIGEST_SIZE);
  Sha256Final (&OuterContext, Dest);

  ZeroMem (InnerHash, sizeof (InnerHash));
  ZeroMem (Context, sizeof (*Context));
}

VOID
HmacSha256 (
  IN  CONST UINT8  *Key,
  IN  UINTN        KeyLen,
  IN  CONST UINT8  *Msg,
  IN  UINTN        MsgLen,
  OUT UINT8        *Dest
  )
{
  HMAC_SHA256_CONTEXT  Context;

  HmacSha256Init (&Context, Key, KeyLen);
  HmacSha256Update (&Context, Msg, MsgLen);
  HmacSha256Final (&Context, Dest);
}

VOID
HmacSha512Init (
  IN HMAC_SHA512_CONTEXT  *Context,
  IN CONST UINT8          *Key,
  IN UINTN                KeyLen
  )
{
  ASSERT (Context != NULL);
  ASSERT (Key != NULL || KeyLen == 0);

  UINT8  KHash[SHA512_DIGEST_SIZE];
  UINT8  IKeyPad[SHA512_BLOCK_SIZE];

  //
  // If the key size exceeds the block size, it should be reduced through hashing
  //
  if (KeyLen > SHA512_BLOCK_SIZE) {
    SHA512_CONTEXT  Sha512Context;
    Sha512Init (&Sha512Context);
    Sha512Update (&Sha512Context, Key, KeyLen);
    Sha512Final (&Sha512Context, KHash);
    Key    = KHash;
    KeyLen = SHA512_DIGEST_SIZE;
  }

  SetMem (IKeyPad, SHA512_BLOCK_SIZE, 0);
  SetMem (Context->OKeyPad, SHA512_BLOCK_SIZE, 0);
  CopyMem (IKeyPad, Key, KeyLen);
  CopyMem (Context->OKeyPad, Key, KeyLen);

  for (UINTN i = 0; i < SHA512_BLOCK_SIZE; i++) {
    IKeyPad[i]          ^= 0x36;
    Context->OKeyPad[i] ^= 0x5c;
  }

  Sha512Init (&Context->Inner);
  Sha512Update (&Context->Inner, IKeyPad, SHA512_BLOCK_SIZE);

  ZeroMem (IKeyPad, sizeof (IKeyPad));
  ZeroMem (KHash, sizeof (KHash));
}

VOID
HmacSha512Update (
  IN OUT   HMAC_SHA512_CONTEXT  *Context,
  IN CONST UINT8                *Msg,
  IN       UINTN                MsgLen
  )
{
  Sha512Update (&Context->Inner, Msg, MsgLen);
}

VOID
HmacSha512Final (
  IN OUT HMAC_SHA512_CONTEXT  *Context,
  OUT    UINT8                *Dest
  )
{
  UINT8           InnerHash[SHA512_DIGEST_SIZE];
  SHA512_CONTEXT  OuterContext;

  Sha512Final (&Context->Inner, InnerHash);

  Sha512Init (&OuterContext);
  Sha512Update (&OuterContext, Context->OKeyPad, SHA512_BLOCK_SIZE);
  Sha512Update (&OuterContext, InnerHash, SHA512_DIGEST_SIZE);
  Sha512Final (&OuterContext, Dest);

  ZeroMem (InnerHash, sizeof (InnerHash));
  ZeroMem (Context, sizeof (*Context));
}

VOID
HmacSha512 (
  IN  CONST UINT8  *Key,
  IN  UINTN        KeyLen,
  IN  CONST UINT8  *Msg,
  IN  UINTN        MsgLen,
  OUT UINT8        *Dest
  )
{
  HMAC_SHA512_CONTEXT  Context;

  HmacSha512Init (&Context, Key, KeyLen);
  HmacSha512Update (&Context, Msg, MsgLen);
  HmacSha512Final (&Context, Dest);
}

VOID
HmacSha384Init (
  IN HMAC_SHA384_CONTEXT  *Context,
  IN CONST UINT8          *Key,
  IN UINTN                KeyLen
  )
{
  ASSERT (Context != NULL);
  ASSERT (Key != NULL || KeyLen == 0);

  UINT8  KHash[SHA384_DIGEST_SIZE];
  UINT8  IKeyPad[SHA384_BLOCK_SIZE];

  //
  // If the key size exceeds the block size, it should be reduced through hashing
  //
  if (KeyLen > SHA384_BLOCK_SIZE) {
    SHA384_CONTEXT  Sha384Context;
    Sha384Init (&Sha384Context);
    Sha384Update (&Sha384Context, Key, KeyLen);
    Sha384Final (&Sha384Context, KHash);
    Key    = KHash;
    KeyLen = SHA384_DIGEST_SIZE;
  }

  SetMem (IKeyPad, SHA384_BLOCK_SIZE, 0);
  SetMem (Context->OKeyPad, SHA384_BLOCK_SIZE, 0);
  CopyMem (IKeyPad, Key, KeyLen);
  CopyMem (Context->OKeyPad, Key, KeyLen);

  for (UINTN i = 0; i < SHA384_BLOCK_SIZE; i++) {
    IKeyPad[i]          ^= 0x36;
    Context->OKeyPad[i] ^= 0x5c;
  }

  Sha384Init (&Context->Inner);
  Sha384Update (&Context->Inner, IKeyPad, SHA384_BLOCK_SIZE);

  ZeroMem (IKeyPad, sizeof (IKeyPad));
  ZeroMem (KHash, sizeof (KHash));
}

VOID
HmacSha384Update (
  IN OUT   HMAC_SHA384_CONTEXT  *Context,
  IN CONST UINT8                *Msg,
  IN       UINTN                MsgLen
  )
{
  Sha384Update (&Context->Inner, Msg, MsgLen);
}

VOID
HmacSha384Final (
  IN OUT HMAC_SHA384_CONTEXT  *Context,
  OUT    UINT8                *Dest
  )
{
  UINT8           InnerHash[SHA384_DIGEST_SIZE];
  SHA384_CONTEXT  OuterContext;

  Sha384Final (&Context->Inner, InnerHash);

  Sha384Init (&OuterContext);
  Sha384Update (&OuterContext, Context->OKeyPad, SHA384_BLOCK_SIZE);
  Sha384Update (&OuterContext, InnerHash, SHA384_DIGEST_SIZE);
  Sha384Final (&OuterContext, Dest);

  ZeroMem (InnerHash, sizeof (InnerHash));
  ZeroMem (Context, sizeof (*Context));
}

VOID
HmacSha384 (
  IN  CONST UINT8  *Key,
  IN  UINTN        KeyLen,
  IN  CONST UINT8  *Msg,
  IN  UINTN        MsgLen,
  OUT UINT8        *Dest
  )
{
  HMAC_SHA384_CONTEXT  Context;

  HmacSha384Init (&Context, Key, KeyLen);
  HmacSha384Update (&Context, Msg, MsgLen);
  HmacSha384Final (&Context, Dest);
}
