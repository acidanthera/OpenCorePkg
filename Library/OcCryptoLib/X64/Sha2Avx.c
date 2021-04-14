/** @file
  Copyright (C) 2021, ISP RAS. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "../Sha2Avx.h"
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

//
// Sha 512 functions
//
VOID
EFIAPI
Sha512TransformAvx (
  IN OUT UINT64      *State,
  IN     CONST UINT8 *Data,
  IN     UINTN       BlockNb
  );

VOID
Sha512InitAvx (
  IN OUT SHA512_CONTEXT  *Context
  )
{
  UINTN  Index;

  for (Index = 0; Index < 8; ++Index) {
    Context->State[Index] = SHA512_H0[Index];
  }
  //
  // Number of bytes in the buffer
  //
  Context->Length = 0;
  //
  // Total length of the data
  //
  Context->TotalLength = 0;
}

VOID
Sha512UpdateAvx (
  IN OUT SHA512_CONTEXT *Context,
  IN     CONST UINT8    *Data,
  IN     UINTN          Len
  )
{
  UINTN        BlockNb;
  UINTN        NewLen;
  UINTN        RemLen;
  UINTN        TmpLen;
  CONST UINT8  *ShiftedMsg;

  TmpLen = SHA512_BLOCK_SIZE - Context->Length;
  RemLen = Len < TmpLen ? Len : TmpLen;

  CopyMem (&Context->Block[Context->Length], Data, RemLen);

  if (Context->Length + Len < SHA512_BLOCK_SIZE) {
    Context->Length += Len;
    return;
  }

  NewLen = Len - RemLen;
  BlockNb = NewLen / SHA512_BLOCK_SIZE;

  ShiftedMsg = Data + RemLen;

  Sha512TransformAvx (Context->State, Context->Block, 1);
  Sha512TransformAvx (Context->State, ShiftedMsg, BlockNb);

  RemLen = NewLen % SHA512_BLOCK_SIZE;

  CopyMem (Context->Block, &ShiftedMsg[BlockNb << 7], RemLen);

  Context->Length = RemLen;
  Context->TotalLength += (BlockNb + 1) << 7;
}

VOID
Sha512FinalAvx (
  IN OUT SHA512_CONTEXT *Context,
  IN OUT UINT8          *HashDigest
  )
{
  UINTN   BlockNb;
  UINTN   PmLen;
  UINT64  LenB;

  BlockNb = ((SHA512_BLOCK_SIZE - 17) < (Context->Length % SHA512_BLOCK_SIZE)) + 1;

  LenB = (Context->TotalLength + Context->Length) << 3;
  PmLen = BlockNb << 7;

  ZeroMem (Context->Block + Context->Length, PmLen - Context->Length);
  Context->Block[Context->Length] = 0x80;
  UNPACK64 (LenB, Context->Block + PmLen - 8);

  Sha512TransformAvx (Context->State, Context->Block, BlockNb);

  CopyMem (HashDigest, Context->State, SHA512_DIGEST_SIZE);
}

VOID
Sha512Avx (
  IN OUT UINT8        *Hash,
  IN     CONST UINT8  *Data,
  IN     UINTN        Len
  )
{
  SHA512_CONTEXT  Ctx;

  Sha512InitAvx (&Ctx);
  Sha512UpdateAvx (&Ctx, Data, Len);
  Sha512FinalAvx (&Ctx, Hash);
  ZeroMem (&Ctx, sizeof (Ctx));
}

//
// Sha 384 functions
//
VOID
Sha384InitAvx (
  IN OUT SHA384_CONTEXT  *Context
  )
{
  UINTN  Index;

  for (Index = 0; Index < 8; ++Index) {
    Context->State[Index] = SHA384_H0[Index];
  }

  Context->Length = 0;
  Context->TotalLength = 0;
}

VOID
Sha384UpdateAvx (
  IN OUT SHA384_CONTEXT *Context,
  IN     CONST UINT8    *Data,
  IN     UINTN          Len
  )
{
  UINTN        BlockNb;
  UINTN        NewLen;
  UINTN        RemLen;
  UINTN        TmpLen;
  CONST UINT8  *ShiftedMessage;

  TmpLen = SHA384_BLOCK_SIZE - Context->Length;
  RemLen = Len < TmpLen ? Len : TmpLen;

  CopyMem (&Context->Block[Context->Length], Data, RemLen);

  if (Context->Length + Len < SHA384_BLOCK_SIZE) {
    Context->Length += Len;
    return;
  }

  NewLen = Len - RemLen;
  BlockNb = NewLen / SHA384_BLOCK_SIZE;

  ShiftedMessage = Data + RemLen;

  Sha512TransformAvx (Context->State, Context->Block, 1);
  Sha512TransformAvx (Context->State, ShiftedMessage, BlockNb);

  RemLen = NewLen % SHA384_BLOCK_SIZE;

  CopyMem (
    Context->Block,
    &ShiftedMessage[BlockNb << 7],
    RemLen
    );

  Context->Length = RemLen;
  Context->TotalLength += (BlockNb + 1) << 7;
}

VOID
Sha384FinalAvx (
  IN OUT SHA384_CONTEXT *Context,
  IN OUT UINT8          *HashDigest
  )
{
  UINTN    BlockNb;
  UINTN    PmLen;
  UINT64   LenB;

  BlockNb = ((SHA384_BLOCK_SIZE - 17) < (Context->Length % SHA384_BLOCK_SIZE)) + 1;

  LenB = (Context->TotalLength + Context->Length) << 3;
  PmLen = BlockNb << 7;

  ZeroMem (Context->Block + Context->Length, PmLen - Context->Length);

  Context->Block[Context->Length] = 0x80;
  UNPACK64 (LenB, Context->Block + PmLen - 8);

  Sha512TransformAvx (Context->State, Context->Block, BlockNb);

  CopyMem (HashDigest, Context->State, SHA384_DIGEST_SIZE);
}

VOID
Sha384Avx (
  IN OUT UINT8        *Hash,
  IN     CONST UINT8  *Data,
  IN     UINTN        Len
  )
{
  SHA384_CONTEXT Ctx;

  Sha384InitAvx (&Ctx);
  Sha384UpdateAvx (&Ctx, Data, Len);
  Sha384FinalAvx (&Ctx, Hash);
  SecureZeroMem (&Ctx, sizeof (Ctx));
}
