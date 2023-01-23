#!/bin/bash

#
# Copyright © 2023 Mike Beaton. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#
# Insert EFI into Nvidia VBIOS.
#
# TODO: Check that original GOP is present, and locate it in the original file automatically.
#

usage() {
  echo "Usage: ./${SELFNAME} {rom-file} {efi-file} {GOP offset} {out-file}"
  echo "E.g.:"
  echo "  ./${SELFNAME} nv.rom GOP.efi 0xFC00 mod.rom"
  echo "  ./${SELFNAME} nv.rom GOP.efi 64512 mod.rom"
  echo ""
}

SELFNAME="$(/usr/bin/basename "${0}")"

if [ "$#" -ne 4 ] ; then
  usage
  exit 0
fi

commands=(
  "EfiRom"
  "UEFIRomExtract"
)
FOUND=1
for command in "${commands[@]}"; do
  if ! command -v "$command" 1>/dev/null ; then
    echo "${command} not available!"
    FOUND=0
  fi
done

if [ "$FOUND" -eq 0 ] ; then
  exit 1
fi

ROM_FILE="$1"
EFI_FILE="$2"
GOP_OFFSET="$3"
OUT_FILE="$4"

# https://unix.stackexchange.com/a/84980/340732
tmpdir=$(mktemp -d 2>/dev/null || mktemp -d -t 'vbios') || exit 1

echo "Splitting original ROM..."

dd bs=1 if="$ROM_FILE" of="$tmpdir/original_first_part.rom" count=$(($GOP_OFFSET)) status=none || exit 1
dd bs=1 if="$ROM_FILE" of="$tmpdir/original_last_part.rom" skip=$(($GOP_OFFSET)) status=none || exit 1

echo "Compressing ${EFI_FILE} using EfiRom..."
EfiRom -o "$tmpdir/insert.rom" -ec "$EFI_FILE" -f 0xAAAA -i 0xBBBB -l 0x30000 || exit 1

echo "Adding Nvidia header..."
dd bs=1 if="$tmpdir/insert.rom" of="$tmpdir/insert_first_part" count=$((0x38)) status=none || exit 1
dd bs=1 if="$tmpdir/insert.rom" of="$tmpdir/insert_last_part" skip=$((0x38)) status=none || exit 1

INSERT_SIZE=$(stat -f%z "$tmpdir/insert.rom") || exit 1

# add NPDE from original GOP
dd bs=1 if="$tmpdir/original_last_part.rom" of="$tmpdir/insert_first_part" skip=$((0x38)) seek=$((0x38)) count=$((0x18)) status=none || exit 1
cat "$tmpdir/insert_first_part" "$tmpdir/insert_last_part" > "$tmpdir/insert_oversize.rom" || exit 1
# `truncate` not present by default on macOS
dd bs=1 if="$tmpdir/insert_oversize.rom" of="$tmpdir/insert_fixed.rom" count="$INSERT_SIZE" status=none || exit 1

# patch size in NPDE
dd bs=1 if="$tmpdir/insert.rom" of="$tmpdir/insert_fixed.rom" skip=$((0x2)) seek=$((0x48)) count=1 conv=notrunc status=none || exit 1

# patch with vendor and device id from original GOP
dd bs=1 if="$tmpdir/original_last_part.rom" of="$tmpdir/insert_fixed.rom" skip=$((0x20)) seek=$((0x20)) count=4 conv=notrunc status=none || exit 1

# patch size in PCIR
dd bs=1 if="$tmpdir/original_last_part.rom" of="$tmpdir/insert_fixed.rom" skip=$((0x16)) seek=$((0x16)) count=1 conv=notrunc status=none || exit 1

# patch end marker in NPDE in fixed ROM (leave PCIR correct and EFI extractable from fixed ROM)
echo -n -e '\x00' | dd bs=1 of="$tmpdir/insert_fixed.rom" seek=$((0x4A)) conv=notrunc status=none || exit 1

echo "Writing ${OUT_FILE}..."
cat "$tmpdir/original_first_part.rom" "$tmpdir/insert_fixed.rom" "$tmpdir/original_last_part.rom" > "$OUT_FILE" || exit 1

# patch end marker in PCIR in out file
echo -n -e '\x00' | dd bs=1 of="$OUT_FILE" seek=$(($GOP_OFFSET + 0x31)) conv=notrunc status=none || exit 1

echo "Verifying ${OUT_FILE}..."
dd bs=1 if="$OUT_FILE" of="$tmpdir/out_efi_part.rom" skip=$(($GOP_OFFSET)) status=none || exit 1
UEFIRomExtract "$tmpdir/out_efi_part.rom" "$tmpdir/extracted.efi" 1>/dev/null || exit 1
ERROR=0
diff "$tmpdir/extracted.efi" "$EFI_FILE" 1>/dev/null || ERROR=1

if [ "$ERROR" -ne 0 ] ; then
  echo " - Failure comparing extracted EFI to original!"
fi

OLD_EFI_COUNT=$(EfiRom -d "$tmpdir/original_last_part.rom" | grep "0x0EF1" | wc -l) || exit 1
OLD_EFI_COUNT=$(($OLD_EFI_COUNT)) || exit 1

NEW_EFI_COUNT=$(EfiRom -d "$tmpdir/out_efi_part.rom" | grep "0x0EF1" | wc -l) || exit 1
NEW_EFI_COUNT=$(($NEW_EFI_COUNT)) || exit 1

if [ "$NEW_EFI_COUNT" -ne $(($OLD_EFI_COUNT + 1)) ] ; then
  echo " - Found ${NEW_EFI_COUNT} EFI parts, expected $(($OLD_EFI_COUNT + 1))!"
fi

if [ "$ERROR" -eq 0 ] ; then
  echo "SUCCESS."
else
  echo "*** WARNING - FAIL ***"
fi

rm -rf "$tmpdir" || exit 1

echo "Done."
