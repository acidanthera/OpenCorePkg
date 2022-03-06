/** @file
  Test crypto algos support.

Copyright (c) 2018, savvas. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcDebugLogLib.h>

#include <Library/OcMiscLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/SimpleTextInEx.h>

#include "CryptoSamples.h"

EFI_STATUS
EFIAPI
TestRsa2048Sha256Verify (
  VOID
  )
{
  EFI_STATUS Status;
  UINT8      DataSha256Hash[SHA256_DIGEST_SIZE];
  BOOLEAN    SignatureVerified = FALSE;

  Sha256 (
    DataSha256Hash,
    Rsa2048Sha256Sample.Data,
    SIGNED_DATA_LEN
    );

  CONST OC_RSA_PUBLIC_KEY *PubKey =
    (CONST OC_RSA_PUBLIC_KEY *) Rsa2048Sha256Sample.PublicKey;

  void *Scratch = AllocatePool (
      RSA_SCRATCH_BUFFER_SIZE (PubKey->Hdr.NumQwords * sizeof (UINT64))
    );

  if (Scratch == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SignatureVerified = RsaVerifySigHashFromKey (
    PubKey,
    Rsa2048Sha256Sample.Signature,
    sizeof (Rsa2048Sha256Sample.Signature),
    DataSha256Hash,
    sizeof (DataSha256Hash),
    OcSigHashTypeSha256,
    Scratch
    );

  FreePool (Scratch);

  if (SignatureVerified) {
    Status = EFI_SUCCESS;
    Print(L"Rsa2048Sha256 signature verifying passed!\n");
  } else {
    Status = EFI_INVALID_PARAMETER;
    Print(L"Rsa2048Sha256 signature verifying failed!\n");
  }

  return Status;
}

EFI_STATUS
EFIAPI
TestAesCtr (
  VOID
  )
{
  EFI_STATUS   Status;
  AES_CONTEXT  Ctx;
  UINT8        PlainText[AES_SAMPLE_DATA_LEN];
  UINT8        CipherText[AES_SAMPLE_DATA_LEN];
  BOOLEAN      AesTestPassed = TRUE;

  CopyMem(PlainText, AesCtrSample.PlainText, AES_SAMPLE_DATA_LEN);
  CopyMem(CipherText, AesCtrSample.CipherText, AES_SAMPLE_DATA_LEN);

  //
  // Init AES context
  //
  AesInitCtxIv (&Ctx, AesCtrSample.Key, AesCtrSample.IV);

  //
  // Encrypt plain text data sample
  //
  AesCtrXcryptBuffer (&Ctx, PlainText, AES_SAMPLE_DATA_LEN);

  //
  // Compare data
  //
  if (CompareMem (PlainText, AesCtrSample.CipherText, AES_SAMPLE_DATA_LEN) == 0) {
    Print (L"AES-128 CTR encryption test passed\n");
  } else {
    Print (L"AES-128 CTR encryption test failed\n");
    AesTestPassed = FALSE;
  }

  //
  // Reinit context
  //
  ZeroMem (&Ctx, sizeof (Ctx));
  AesInitCtxIv (&Ctx, AesCtrSample.Key, AesCtrSample.IV);

  //
  // Decrypt cipher text data sample
  //
  AesCtrXcryptBuffer (&Ctx, CipherText, AES_SAMPLE_DATA_LEN);

  //
  // Compare data
  //
  if (CompareMem (CipherText, AesCtrSample.PlainText, AES_SAMPLE_DATA_LEN) == 0) {
    Print (L"AES-128 CTR decryption test passed\n");
  } else {
    Print (L"AES-128 CTR decryption test failed\n");
    AesTestPassed = FALSE;
  }

  //
  // Clean context on exit
  //
  ZeroMem (&Ctx, sizeof (Ctx));

  if (AesTestPassed) {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

EFI_STATUS
EFIAPI
TestAesCbc (
  VOID
  )
{
  EFI_STATUS   Status;
  AES_CONTEXT  Ctx;
  UINT8        PlainText[AES_SAMPLE_DATA_LEN];
  UINT8        CipherText[AES_SAMPLE_DATA_LEN];
  BOOLEAN      AesTestPassed = TRUE;

  CopyMem(PlainText, AesCbcSample.PlainText, AES_SAMPLE_DATA_LEN);
  CopyMem(CipherText, AesCbcSample.CipherText, AES_SAMPLE_DATA_LEN);

  //
  // Init AES context
  //
  AesInitCtxIv (&Ctx, AesCbcSample.Key, AesCbcSample.IV);

  //
  // Encrypt plain text data sample
  //
  AesCbcEncryptBuffer (&Ctx, PlainText, AES_SAMPLE_DATA_LEN);

  //
  // Compare data
  //
  if (CompareMem (PlainText, AesCbcSample.CipherText, AES_SAMPLE_DATA_LEN) == 0) {
    Print (L"AES-128 CBC encryption test passed\n");
  } else {
    Print (L"AES-128 CBC encryption test failed\n");
    AesTestPassed = FALSE;
  }

  //
  // Reinit context
  //
  ZeroMem (&Ctx, sizeof (Ctx));
  AesInitCtxIv (&Ctx, AesCbcSample.Key, AesCbcSample.IV);

  //
  // Decrypt cipher text data sample
  //
  AesCbcDecryptBuffer (&Ctx, CipherText, AES_SAMPLE_DATA_LEN);

  //
  // Compare data
  //
  if (CompareMem (CipherText, AesCbcSample.PlainText, AES_SAMPLE_DATA_LEN) == 0) {
    Print (L"AES-128 CBC decryption test passed\n");
  } else {
    Print (L"AES-128 CBC decryption test failed\n");
    AesTestPassed = FALSE;
  }

  //
  // Clean context on exit
  //
  ZeroMem (&Ctx, sizeof (Ctx));

  if (AesTestPassed) {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

EFI_STATUS
EFIAPI
TestChaCha (
  VOID
  )
{
  CHACHA_CONTEXT  Context;
  BOOLEAN         EncryptionOk;
  BOOLEAN         DecryptionOk;
  UINT8 TmpBuffer1[sizeof (ChaChaCipherText)];
  UINT8 TmpBuffer2[sizeof (ChaChaCipherText)];

  ZeroMem (&Context, sizeof (Context));

  ChaChaInitCtx (
    &Context,
    ChaChaEncryptionKey,
    ChaChaInitVector,
    ChaChaCounter
    );

  ZeroMem (TmpBuffer1, sizeof (TmpBuffer1));

  ChaChaCryptBuffer (
    &Context,
    ChaChaPlainText,
    TmpBuffer1,
    sizeof (TmpBuffer1)
    );

  EncryptionOk = CompareMem (TmpBuffer1, ChaChaCipherText, sizeof (TmpBuffer1)) == 0;
  if (EncryptionOk) {
    Print (L"ChaCha encryption test passed\n");
  } else {
    Print (L"ChaCha encryption test failed\n");
  }

  ZeroMem (&Context, sizeof (Context));

  ChaChaInitCtx (
    &Context,
    ChaChaEncryptionKey,
    ChaChaInitVector,
    ChaChaCounter
    );

  ZeroMem (TmpBuffer2, sizeof (TmpBuffer2));

  ChaChaCryptBuffer (
    &Context,
    TmpBuffer1,
    TmpBuffer2,
    sizeof (TmpBuffer2)
    );

  DecryptionOk = CompareMem (TmpBuffer2, ChaChaPlainText, sizeof (TmpBuffer2)) == 0;
  if (DecryptionOk) {
    Print (L"ChaCha decryption test passed\n");
  } else {
    Print (L"ChaCha decryption test failed\n");
  }

  if (EncryptionOk && DecryptionOk) {
    return EFI_SUCCESS;
  }

  return EFI_INVALID_PARAMETER;
}


EFI_STATUS
EFIAPI
TestHash (
  VOID
  )
{
  EFI_STATUS   Status          = EFI_INVALID_PARAMETER;
  UINTN        Index           = 0;
  UINTN        Index2          = 0;
  BOOLEAN      HashTestPassed  = TRUE;
  UINT8        Md5Hash[MD5_DIGEST_SIZE];
  UINT8        Sha1Hash[SHA1_DIGEST_SIZE];
  UINT8        Sha256Hash[SHA256_DIGEST_SIZE];
  UINT8        Sha512Hash[SHA512_DIGEST_SIZE];
  UINT8        Sha384Hash[SHA384_DIGEST_SIZE];

  //
  // Iterate through hash samples
  //
  for (Index = 0; Index < HASH_SAMPLES_NUM; Index++) {
    Print(L"#########################\n");
    Print(L"Running hash test №%lu\n", Index);
    Print(L"Test data:\n");
    for (Index2 = 0; Index2 < HashSamples[Index].PlainTextLen; Index2++) {
      if (Index2 % 8 == 0 && Index2 > 0) {
        Print(L"\n");
      }
      Print(L"%02x ",HashSamples[Index].PlainText[Index2]);
    }
    Print(L"\n-------------------------\n");
    Md5 (
      Md5Hash,
      HashSamples[Index].PlainText,
      HashSamples[Index].PlainTextLen
      );

    Sha1 (
      Sha1Hash,
      HashSamples[Index].PlainText,
      HashSamples[Index].PlainTextLen
      );

    Sha256 (
      Sha256Hash,
      HashSamples[Index].PlainText,
      HashSamples[Index].PlainTextLen
      );

    Sha512 (
      Sha512Hash,
      HashSamples[Index].PlainText,
      HashSamples[Index].PlainTextLen
      );

    Sha384 (
      Sha384Hash,
      HashSamples[Index].PlainText,
      HashSamples[Index].PlainTextLen
      );


    if (CompareMem ( Md5Hash, HashSamples[Index].Md5Hash, MD5_DIGEST_SIZE) == 0) {
      Print (L"Md5 hash test passed\n");
    } else {
      Print (L"Md5 hash test failed\n");
      HashTestPassed = FALSE;
    }

    if (CompareMem ( Sha1Hash, HashSamples[Index].Sha1Hash, SHA1_DIGEST_SIZE) == 0) {
      Print (L"Sha1 hash test passed\n");
    } else {
      Print (L"Sha1 hash test failed\n");
      HashTestPassed = FALSE;
    }

    if (CompareMem ( Sha256Hash, HashSamples[Index].Sha256Hash, SHA256_DIGEST_SIZE) == 0) {
      Print (L"Sha256 hash test passed\n");
    } else {
      Print (L"Sha256 hash test failed\n");
      HashTestPassed = FALSE;
    }

    if (CompareMem ( Sha512Hash, HashSamples[Index].Sha512Hash, SHA512_DIGEST_SIZE) == 0) {
      Print (L"Sha512 hash test passed\n");
    } else {
      Print (L"Sha512 hash test failed\n");
      HashTestPassed = FALSE;
    }

    if (CompareMem ( Sha384Hash, HashSamples[Index].Sha384Hash, SHA384_DIGEST_SIZE) == 0) {
      Print (L"Sha384 hash test passed\n");
    } else {
      Print (L"Sha384 hash test failed\n");
      HashTestPassed = FALSE;
    }

    ZeroMem (Md5Hash, MD5_DIGEST_SIZE);
    ZeroMem (Sha1Hash, SHA1_DIGEST_SIZE);
    ZeroMem (Sha256Hash, SHA256_DIGEST_SIZE);
    ZeroMem (Sha512Hash, SHA512_DIGEST_SIZE);
    ZeroMem (Sha384Hash, SHA384_DIGEST_SIZE);
  }

  if (HashTestPassed) {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_INVALID_PARAMETER;
  }

  //
  // Zeroes buffers
  //
  ZeroMem (Md5Hash, MD5_DIGEST_SIZE);
  ZeroMem (Sha1Hash, SHA1_DIGEST_SIZE);
  ZeroMem (Sha256Hash, SHA256_DIGEST_SIZE);
  ZeroMem (Sha512Hash, SHA256_DIGEST_SIZE);
  ZeroMem (Sha384Hash, SHA256_DIGEST_SIZE);

  return Status;
}

EFI_STATUS
EFIAPI
UefiDriverMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  BOOLEAN    Failure;
  EFI_STATUS Status;

  Failure = FALSE;

  //
  // Test hash algorithms
  //
  Status = TestHash ();
  if (EFI_ERROR (Status)) {
    Print (L"HashTest failed!\n");
    Failure = TRUE;
  } else {
    Print (L"All hash tests passed!\n");
  }

  //
  // Test AES-128-CBC
  //
  Status = TestAesCbc ();
  if (EFI_ERROR (Status)) {
    Print (L"AES-128-CBC failed!\n");
    Failure = TRUE;
  } else {
    Print (L"AES-128-CBC passed!\n");
  }

  //
  // Test AES-128-CTR
  //
  Status = TestAesCtr ();
  if (EFI_ERROR (Status)) {
    Print (L"AES-128-CTR failed!\n");
    Failure = TRUE;
  } else {
    Print (L"AES-128-CTR passed!\n");
  }

  Status = TestChaCha ();
  if (EFI_ERROR (Status)) {
    Print (L"ChaCha failed!\n");
    Failure = TRUE;
  } else {
    Print (L"ChaCha passed!\n");
  }

  //
  // Test Rsa2048Sha256 signature
  //
  Status = TestRsa2048Sha256Verify ();
  if (EFI_ERROR (Status)) {
    Print (L"Rsa2048Sha256 failed!\n");
    Failure = TRUE;
  } else {
    Print (L"Rsa2048Sha256 passed!\n");
  }

  if (Failure) {
    Print (L"Some tests failed\n");
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UefiAppMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  BOOLEAN    Failure;
  EFI_STATUS Status;

  Failure = FALSE;

  WaitForKeyPress (L"Press any key...");
  Print (L"This is test app...\n");

  //
  // Test hash algorithms
  //
  Status = TestHash ();
  if (EFI_ERROR (Status)) {
    Print(L"HashTest failed!\n");
    Failure = TRUE;
  } else {
    Print(L"All hash tests passed!\n");
  }

  WaitForKeyPress (L"Press any key...");

  //
  // Test AES-128-CBC
  //
  Status = TestAesCbc ();
  if (EFI_ERROR (Status)) {
    Print(L"AES-128-CBC failed!\n");
    Failure = TRUE;
  } else {
    Print(L"AES-128-CBC passed!\n");
  }

  WaitForKeyPress (L"Press any key...");

  //
  // Test AES-128-CTR
  //
  Status = TestAesCtr ();
  if (EFI_ERROR (Status)) {
    Print(L"AES-128-CTR failed!\n");
    Failure = TRUE;
  } else {
    Print(L"AES-128-CTR passed!\n");
  }

  WaitForKeyPress (L"Press any key...");

  //
  // Test ChaCha
  //
  Status = TestChaCha ();
  if (EFI_ERROR (Status)) {
    Print (L"ChaCha failed!\n");
    Failure = TRUE;
  } else {
    Print (L"ChaCha passed!\n");
  }

  WaitForKeyPress (L"Press any key...");

  //
  // Test Rsa2048Sha256 signature
  //
  Status = TestRsa2048Sha256Verify ();
  if (EFI_ERROR (Status)) {
    Print(L"Rsa2048Sha256 failed!\n");
    Failure = TRUE;
  } else {
    Print(L"Rsa2048Sha256 passed!\n");
  }
  WaitForKeyPress (L"Press any key to exit");


  if (Failure) {
    Print (L"Some tests failed\n");
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}
