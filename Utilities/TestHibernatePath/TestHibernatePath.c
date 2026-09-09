/** @file
  Regression tests for the actual hibernation path resolver with mocked disks.
  SPDX-License-Identifier: BSD-3-Clause
**/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Uefi.h>
#include <Protocol/BlockIo.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "BootManagementInternal.h"

static const unsigned char    image[] = {
  2,    1,    12, 0, 0xd0, 0x41, 3,   0x0a, 0,   0, 0,   0,
  1,    1,    6,  0, 0,    0x1c, 1,   1,    6,   0, 0,   0,
  3,    1,    8,  0, 0,    1,    0,   0,
  4,    4,    16, 0, '1',  0,    '0', 0,    '0', 0, ':', 0,'A',0, 0, 0,
  0x7f, 0xff, 4,  0
};
static unsigned char          paths[3][44];
static EFI_BLOCK_IO_MEDIA     media[3];
static EFI_BLOCK_IO_PROTOCOL  io[3];
static unsigned               count;
static int                    already_valid, reject_fixed, no_handles;
static unsigned               tests;

static void
check (
  int         ok,
  const char  *what
  )
{
  if (!ok) {
    fprintf (stderr, "FAIL: %s\n", what);
    exit (1);
  }
}

static EFI_STATUS EFIAPI
handle (
  EFI_HANDLE  h,
  EFI_GUID    *g,
  VOID        **p
  )
{
  UINTN  n = (UINTN)h - 1;

  if (n >= count) {
    return EFI_NOT_FOUND;
  }

  if (CompareGuid (g, &gEfiBlockIoProtocolGuid)) {
    *p = &io[n];
  } else if (CompareGuid (g, &gEfiDevicePathProtocolGuid)) {
    *p = paths[n];
  } else {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI
handles (
  EFI_LOCATE_SEARCH_TYPE  t,
  EFI_GUID                *g,
  VOID                    *k,
  UINTN                   *n,
  EFI_HANDLE              **h
  )
{
  unsigned  i;

  if (no_handles || !count) {
    return EFI_NOT_FOUND;
  }

  *n = count;
  *h = AllocatePool (count * sizeof (**h));
  for (i = 0; i < count; ++i) {
    (*h)[i] = (EFI_HANDLE)(UINTN)(i + 1);
  }

  return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI
locate (
  EFI_GUID                  *g,
  EFI_DEVICE_PATH_PROTOCOL  **p,
  EFI_HANDLE                *h
  )
{
  unsigned       i;
  unsigned char  *b = (unsigned char *)*p;

  if (already_valid && (b[25] == MSG_ATAPI_DP)) {
    *p = (EFI_DEVICE_PATH_PROTOCOL *)(b + 32);
    *h = (EFI_HANDLE)1;
    return EFI_SUCCESS;
  }

  if (reject_fixed) {
    return EFI_NOT_FOUND;
  }

  for (i = 0; i < count; ++i) {
    if (memcmp (b, paths[i], 40) == 0) {
      *p = (EFI_DEVICE_PATH_PROTOCOL *)(b + 40);
      *h = (EFI_HANDLE)(UINTN)(i + 1);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

static void
reset (
  void
  )
{
  unsigned  i;

  count         = 1;
  already_valid = reject_fixed = no_handles = 0;
  memset (paths, 0, sizeof (paths));
  memset (media, 0, sizeof (media));
  memset (io, 0, sizeof (io));
  for (i = 0; i < 3; ++i) {
    memcpy (paths[i], image, 24);
    paths[i][24] = 3;
    paths[i][25] = MSG_SASEX_DP;
    paths[i][26] = 16;
    paths[i][28] = (unsigned char)(i + 1);
    memcpy (paths[i] + 40, image + sizeof (image) - 4, 4);
    media[i].MediaPresent = TRUE;
    media[i].BlockSize    = 4096;
    media[i].LastBlock    = 1000000;
    io[i].Media           = &media[i];
  }

  gBS->HandleProtocol     = handle;
  gBS->LocateHandleBuffer = handles;
  gBS->LocateDevicePath   = locate;
}

static void
run (
  const char           *name,
  const unsigned char  *input,
  UINTN                size,
  int                  expected
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *p      = AllocateCopyPool (size, input);
  EFI_DEVICE_PATH_PROTOCOL  *before = p;
  int                       result  = InternalFixAppleHibernateDevicePath (&p);

  check (result == expected, name);
  if (expected) {
    check (GetDevicePathSize (p) == size + 8, "correct replacement size");
    check (memcmp (p, paths[0], 40) == 0, "exact firmware disk path");
    check (memcmp ((unsigned char *)p + 40, input + 32, size - 32) == 0, "unchanged image suffix");
  } else {
    check (p == before && memcmp (p, input, size) == 0, "failure preserves allocation and bytes");
  }

  FreePool (p);
  ++tests;
  printf ("PASS: %s\n", name);
}

int
main (
  int   argc,
  char  **argv
  )
{
  unsigned char  bad[sizeof (image)];
  unsigned char  multi[sizeof (image) * 2];

  reset ();
  run ("Apple NVMe node", image, sizeof (image), 1);
  reset ();
  paths[0][25] = MSG_NVME_NAMESPACE_DP;
  run ("UEFI NVMe node", image, sizeof (image), 1);
  reset ();
  count = 2;
  run ("two namespaces rejected", image, sizeof (image), 0);
  reset ();
  count        = 2;
  paths[1][17] = 0x1d;
  run ("other PCI controller ignored", image, sizeof (image), 1);
  reset ();
  paths[0][17] = 0x1d;
  run ("wrong PCI controller rejected", image, sizeof (image), 0);
  reset ();
  media[0].LogicalPartition = TRUE;
  run ("partition rejected", image, sizeof (image), 0);
  reset ();
  media[0].RemovableMedia = TRUE;
  run ("removable disk rejected", image, sizeof (image), 0);
  reset ();
  media[0].MediaPresent = FALSE;
  run ("absent media rejected", image, sizeof (image), 0);
  reset ();
  io[0].Media = NULL;
  run ("missing media rejected", image, sizeof (image), 0);
  reset ();
  no_handles = 1;
  run ("enumeration failure unchanged", image, sizeof (image), 0);
  reset ();
  already_valid = 1;
  run ("working original path unchanged", image, sizeof (image), 0);
  reset ();
  reject_fixed = 1;
  run ("replacement must resolve", image, sizeof (image), 0);
  reset ();
  paths[0][25] = MSG_USB_DP;
  run ("non-NVMe disk rejected", image, sizeof (image), 0);
  reset ();
  memcpy (bad, image, sizeof (image));
  bad[25] = MSG_SATA_DP;
  run ("non-ATAPI input unchanged", bad, sizeof (bad), 0);
  reset ();
  memcpy (bad, image, sizeof (image));
  bad[33] = MEDIA_HARDDRIVE_DP;
  run ("partition suffix unchanged", bad, sizeof (bad), 0);
  reset ();
  memcpy (multi, image, sizeof (image));
  memcpy (multi+sizeof (image), image, sizeof (image));
  multi[49] = 1;
  run ("multi-instance input unchanged", multi, sizeof (multi), 0);
  reset ();
  check (!InternalFixAppleHibernateDevicePath (NULL), "null input");
  ++tests;
  if (argc == 2) {
    unsigned char  captured[4096];
    FILE           *f = fopen (argv[1], "rb");
    check (f != NULL, "open captured fixture");
    size_t  size = fread (captured, 1, sizeof (captured), f);
    check (feof (f) && size >= sizeof (image), "read captured fixture");
    fclose (f);
    reset ();
    run ("actual captured boot-image", captured, size, 1);
  }

  printf ("%u regression checks passed\n", tests);
  return 0;
}
