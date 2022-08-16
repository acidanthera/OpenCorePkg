/** @file
  Copyright (c) 2020, Marvin Haeuser. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <stdio.h>
#ifdef _WIN32
  #include <conio.h>
#else
  #include <termios.h>
  #include <unistd.h>
#endif

#include <Library/DebugLib.h>
#include <Library/OcCryptoLib.h>
#include <UserPseudoRandom.h>

//
// Signal Interrupt Control-C symbol
//
#define CHAR_END_OF_TEXT  3
#define CHAR_DELETE       127

#ifndef _WIN32

/**
 * getch unix-compatible implementation
 *
 * @return integer with ch codenumber
 */
int
getch (
  void
  )
{
  int             Char;
  struct termios  OldTtyAttrs, NewTtyAttrs;

  tcgetattr (STDIN_FILENO, &OldTtyAttrs);
  NewTtyAttrs         = OldTtyAttrs;
  NewTtyAttrs.c_lflag = NewTtyAttrs.c_lflag & ~(ICANON|ECHO);
  tcsetattr (STDIN_FILENO, TCSANOW, &NewTtyAttrs);
  Char = getchar ();
  tcsetattr (STDIN_FILENO, TCSANOW, &OldTtyAttrs);

  return Char;
}

#endif

int
ENTRY_POINT (
  void
  )
{
  CHAR8   Char;
  UINT8   Password[OC_PASSWORD_MAX_LEN];
  UINT8   PasswordLen;
  UINT32  Salt[4];
  UINT8   Index;
  UINT8   PasswordHash[SHA512_DIGEST_SIZE];

  printf ("Please enter your password: ");

  for (PasswordLen = 0; PasswordLen < OC_PASSWORD_MAX_LEN; ++PasswordLen) {
    fflush (stdin);
    Char = getch ();

    // Handle interrupt signal for conio
    if (Char == CHAR_END_OF_TEXT) {
      exit (EXIT_FAILURE);
    }

    if ((Char == EOF) || (Char == CHAR_LINEFEED) || (Char == CHAR_CARRIAGE_RETURN)) {
      Password[PasswordLen] = '\0';
      break;
    }

    if ((Char == CHAR_BACKSPACE) || (Char == CHAR_DELETE)) {
      if (PasswordLen > 0) {
        --PasswordLen;
      }

      continue;
    }

    Password[PasswordLen] = (UINT8)Char;
  }

  for (Index = 0; Index < ARRAY_SIZE (Salt); ++Index) {
    Salt[Index] = pseudo_random ();
  }

  OcHashPasswordSha512 (
    Password,
    PasswordLen,
    (UINT8 *)Salt,
    sizeof (Salt),
    PasswordHash
    );

  printf ("\nPasswordHash: <");
  for (Index = 0; Index < sizeof (PasswordHash); ++Index) {
    printf ("%02x", PasswordHash[Index]);
  }

  printf (">\nPasswordSalt: <");
  for (Index = 0; Index < sizeof (Salt); ++Index) {
    printf ("%02x", ((UINT8 *)Salt)[Index]);
  }

  printf (">\n");

  SecureZeroMem (Password, sizeof (Password));
  SecureZeroMem (PasswordHash, sizeof (PasswordHash));
  SecureZeroMem (&PasswordLen, sizeof (PasswordLen));

  return 0;
}
