#!/bin/bash

#
# Copyright © 2023 Mike Beaton. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#
# Insert EFI into AMD or Nvidia VBIOS.
# Tested back to Mac OS X 10.11 El Capitan.
#

usage() {
  echo "Usage: ./${SELFNAME} [args] {rom-file} {efi-file} {out-file}"
  echo "Args:"
  echo "  -a              : AMD"
  echo "  -n              : Nvidia"
  echo "  -o {GOP offset} : GOP offset (auto-detected if Homebrew grep is installed)"
  echo "                    Can specify 0x{hex} or {decimal}"
  echo "  -t {temp dir}   : Specify temporary directory, and keep temp files"
  echo "  -m {max size}   : Specify VBIOS max size (Nvidia only)"
  echo "                    (For AMD first 128KB is modified, any remainder is kept)"
  echo "Examples:"
  echo "  ./${SELFNAME} -n -o 0xFC00 nv.rom EnableGop.efi nv_mod.rom"
  echo "  ./${SELFNAME} -n nv.rom EnableGop.efi nv_mod.rom"
  echo "  ./${SELFNAME} -a amd.rom EnableGop.efi amd_mod.rom"
  echo ""
}

SELFNAME="$(/usr/bin/basename "${0}")"

commands=(
  "EfiRom"
  "UEFIRomExtract"
  "hexdump"
  "grep"
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

AMD=0
AMD_SAFE_SIZE="0x20000"
NVIDIA_SAFE_SIZE="0x40000"
GOP_OFFSET="-"
NVIDIA=0
POS=0
TRUNCATE=0

while true; do
  if [ "$1" = "-a" ] ; then
    AMD=1
    NVIDIA=0
    shift
  elif [ "$1" = "-n" ] ; then
    AMD=0
    NVIDIA=1
    shift
  elif [ "$1" = "-o" ] ; then
    shift
    if [ "$1" != "" ] && ! [ "${1:0:1}" = "-" ] ; then
      GOP_OFFSET=$1
      shift
    else
      echo "No GOP offset specified" && exit 1
    fi
  elif [ "$1" = "-s" ] ; then # semi-secret option to modify AMD safe size
    shift
    if [ "$1" != "" ] && ! [ "${1:0:1}" = "-" ] ; then
      AMD_SAFE_SIZE=$1
      shift
    else
      echo "No AMD safe size specified" && exit 1
    fi
  elif [ "$1" = "-m" ] ; then
    shift
    if [ "$1" != "" ] && ! [ "${1:0:1}" = "-" ] ; then
      TRUNCATE=1
      TRUNCATE_SIZE=$1
      NVIDIA_SAFE_SIZE=$TRUNCATE_SIZE
      shift
    else
      echo "No max size specified" && exit 1
    fi
  elif [ "$1" = "-t" ] ; then
    shift
    if [ "$1" != "" ] && ! [ "${1:0:1}" = "-" ] ; then
      TEMP_DIR=$1
      shift
    else
      echo "No temp dir specified" && exit 1
    fi
  elif [ "${1:0:1}" = "-" ] ; then
    echo "Unknown option: ${1}" && exit 1
  elif [ "$1" != "" ] ; then
    case "$POS" in
      0 )
        ROM_FILE="$1"
      ;;
      1 )
        EFI_FILE="$1"
      ;;
      2 )
        OUT_FILE="$1"
      ;;
      * )
        echo "Too many filenames specified" && exit 1
      ;;
    esac
    POS=$(($POS+1))
    shift
  else
    break
  fi
done

if [ "$ROM_FILE" = "" ] ||
   [ "$EFI_FILE" = "" ] ||
   [ "$OUT_FILE" = "" ] ; then
  usage
  exit 0
fi

if [ "$AMD" -eq 0 ] && [ "$NVIDIA" -eq 0 ] ; then
  echo "Must specify -a or -n" && exit 1
fi

if [ "$AMD" -eq 1 ] && [ "$TRUNCATE" -eq 1 ] ; then
  echo "-m is not valid with -a" && exit 1
fi

if [ "$TEMP_DIR" != "" ] ; then
  mkdir -p "$TEMP_DIR" || exit 1
  tmpdir="$TEMP_DIR"
else
  # https://unix.stackexchange.com/a/84980/340732
  tmpdir=$(mktemp -d 2>/dev/null || mktemp -d -t 'vbios') || exit 1
fi

ORIGINAL_SIZE=$(stat -f%z "$ROM_FILE") || exit 1

if [ "$AMD" -eq 1 ] ; then
  # For AMD we can only modify the first 128KB, anything above 128KB
  # should not be moved, and is not mapped to memory visible by the CPU
  # for loading EFI drivers.
  TRUNCATE=1
  TRUNCATE_SIZE="$AMD_SAFE_SIZE"

  # Also works, with empty keep_part.rom, in the atypical case where we
  # are provided with only the used part of the ROM below 128KB.
  dd bs=1 if="$ROM_FILE" of="$tmpdir/modify_part.rom" count="$AMD_SAFE_SIZE" 2>/dev/null || exit 1
  dd bs=1 if="$ROM_FILE" of="$tmpdir/keep_part.rom" skip="$AMD_SAFE_SIZE" 2>/dev/null || exit 1
else
  if [ "$TRUNCATE" -eq 0 ] ; then
    # If original size is a plausible ROM size (exact power of two, 64KB or
    # larger; 64KB chosen partly for neat regexp) treat it as the full available
    # size of the VBIOS chip unless overridden with -m.
    printf '%x' "$ORIGINAL_SIZE" | grep -Eq "^(1|2|4|8)0000+$" && TRUNCATE=1
    if [ "$TRUNCATE" -eq 1 ] ; then
      echo "Detected standard ROM size."
      TRUNCATE_SIZE="$ORIGINAL_SIZE"
    else
      if [ "$ORIGINAL_SIZE" -gt "$((NVIDIA_SAFE_SIZE))" ] ; then
        echo " - File size of ${ORIGINAL_SIZE} bytes must be no more than $((NVIDIA_SAFE_SIZE)) bytes; use -m or check file" && exit 1
      fi
      TRUNCATE=1
      TRUNCATE_SIZE="$NVIDIA_SAFE_SIZE"
    fi
  fi

  cp "$ROM_FILE" "$tmpdir/modify_part.rom" || exit 1
fi

if [ "$GOP_OFFSET" = "-" ] ; then
  echo "Auto-detecting GOP offset..."

  # nicer techniques which do not assume nice alignment of what is being searched for do not work on older Mac OS X
  OUTPUT=$(hexdump -C "$tmpdir/modify_part.rom" | grep '55 aa .. .. f1 0e 00 00' | head -1)
  # Make macOS bash to split as expected:
  # shellcheck disable=SC2206
  GOP_ARRAY=($OUTPUT)
  GOP_OFFSET=${GOP_ARRAY[0]}
  if [ "$GOP_OFFSET" != "" ] ; then
    GOP_OFFSET="0x${GOP_OFFSET}"
    GOP_OFFSET=$(($GOP_OFFSET))
  else
    GOP_OFFSET=-1
  fi

  if [ "$GOP_OFFSET" -eq -1 ] ; then
    echo " - No GOP found in ROM!" && exit 1
  fi
fi

dd bs=1 if="$tmpdir/modify_part.rom" of="$tmpdir/original_first_part.rom" count=$(($GOP_OFFSET)) 2>/dev/null || exit 1
dd bs=1 if="$tmpdir/modify_part.rom" of="$tmpdir/original_last_part.rom" skip=$(($GOP_OFFSET)) 2>/dev/null || exit 1

echo "Compressing EFI using EfiRom..."
if [ "$AMD" -eq 1 ] ; then
  EfiRom -o "$tmpdir/insert.rom" -ec "$EFI_FILE" -f 0xAAAA -i 0xBBBB -l 0x30000 -p || exit 1
else
  EfiRom -o "$tmpdir/insert.rom" -ec "$EFI_FILE" -f 0xAAAA -i 0xBBBB -l 0x30000 || exit 1
fi

if [ "$NVIDIA" -eq 1 ] ; then
  dd bs=1 if="$tmpdir/insert.rom" of="$tmpdir/insert_first_part" count=$((0x38)) 2>/dev/null || exit 1
  dd bs=1 if="$tmpdir/insert.rom" of="$tmpdir/insert_last_part" skip=$((0x38)) 2>/dev/null || exit 1

  # TODO: truncation logic should be fixed for when there is not enough spare padding in output of EfiRom;
  # we currently assume without checking that there is enough space to fit in the NPDE header (if not,
  # script will report failure to verify with UEFIRomExtract below).
  INSERT_SIZE=$(stat -f%z "$tmpdir/insert.rom") || exit 1

  # Calculate NPDE size from original GOP and add to new image
  EfiImageOffset=$(dd if="$tmpdir/original_last_part.rom" ibs=1 skip=$((0x16)) count=2 2>/dev/null | od -t u4 -An | xargs)
  if [ "$EfiImageOffset" -eq $((0x50)) ] ; then
    NpdeSize=$((0x18))
  elif [ "$EfiImageOffset" -eq $((0x4C)) ] ; then
    NpdeSize=$((0x14))
  elif [ "$EfiImageOffset" -eq $((0x38)) ] ; then
    NpdeSize=0
  else
    # Note: We need at least 0x14 NPDE size for the patch-ups we do below for size and end-marker to make sense
    printf "Unsupported EFI Image Offset 0x%x, cannot calculate NPDE Size!\n" "$EfiImageOffset"
    exit 1
  fi
fi

if [ "$NVIDIA" -eq 1 ] && [ "$NpdeSize" -ne 0 ] ; then
  echo "Adding Nvidia header..."

  dd bs=1 if="$tmpdir/original_last_part.rom" of="$tmpdir/insert_first_part" skip=$((0x38)) seek=$((0x38)) count="$NpdeSize" 2>/dev/null || exit 1
  cat "$tmpdir/insert_first_part" "$tmpdir/insert_last_part" > "$tmpdir/insert_oversize.rom" || exit 1
  # Note: `truncate` command is not present by default on macOS
  dd bs=1 if="$tmpdir/insert_oversize.rom" of="$tmpdir/insert_fixed.rom" count="$INSERT_SIZE" 2>/dev/null || exit 1

  # Patch size in NPDE
  dd bs=1 if="$tmpdir/insert.rom" of="$tmpdir/insert_fixed.rom" skip=$((0x2)) seek=$((0x48)) count=1 conv=notrunc 2>/dev/null || exit 1
else
  cp "$tmpdir/insert.rom" "$tmpdir/insert_fixed.rom" || exit 1
fi

# patch with vendor and device id from original GOP
dd bs=1 if="$tmpdir/original_last_part.rom" of="$tmpdir/insert_fixed.rom" skip=$((0x20)) seek=$((0x20)) count=4 conv=notrunc 2>/dev/null || exit 1

if [ "$NVIDIA" -eq 1 ] && [ "$NpdeSize" -ne 0 ] ; then
  # Patch EFI image offset in PCIR
  dd bs=1 if="$tmpdir/original_last_part.rom" of="$tmpdir/insert_fixed.rom" skip=$((0x16)) seek=$((0x16)) count=1 conv=notrunc 2>/dev/null || exit 1

  # Patch end marker in NPDE in fixed ROM (leave PCIR correct and EFI extractable from fixed ROM)
  echo -n -e '\x00' | dd bs=1 of="$tmpdir/insert_fixed.rom" seek=$((0x4A)) conv=notrunc 2>/dev/null || exit 1
fi

echo "Combining..."
cat "$tmpdir/original_first_part.rom" "$tmpdir/insert_fixed.rom" "$tmpdir/original_last_part.rom" > "$tmpdir/combined.rom" || exit 1

# If new ROM is larger than truncate size, determine overflow by seeing
# whether truncated ROM still has padding.
# For Nvidia we would need to parse and navigate NVGI and NPDE headers
# to calculate true occupied VBIOS space. To calcuate AMD occupation below
# 128KB limit, we could navigate normal PCI expansion ROM headers.
COMBINED_SIZE=$(stat -f%z "$tmpdir/combined.rom") || exit 1
if [ "$COMBINED_SIZE" -le "$(($TRUNCATE_SIZE))" ] ; then
  TRUNCATE=0
fi
if [ "$TRUNCATE" -eq 1 ] ; then
  echo "Truncating to original size..."

  dd bs=1 if="$tmpdir/combined.rom" of="$tmpdir/truncated.rom" count="$TRUNCATE_SIZE" 2>/dev/null || exit 1

  COUNT=$(hexdump -v -e '1/8 " %016X\n"' "$tmpdir/truncated.rom" | tail -n 8 | grep "FFFFFFFFFFFFFFFF" | wc -l)
  if [ "$COUNT" -ne 8 ] ; then
    # Some Nvidia ROMs, at least, incorrectly have 00000000 padding after active contents
    # (it is incorrect, since writing only active contents using nvflash resets the rest to ffffffff).
    # May also be relevant if we ever have any truly 00000000 default ROM images.
    COUNT=$(hexdump -v -e '1/8 " %016X\n"' "$tmpdir/truncated.rom" | tail -n 8 | grep "0000000000000000" | wc -l)
  fi

  if [ "$COUNT" -ne 8 ] ; then
    echo " - Not enough space within $((TRUNCATE_SIZE / 1024))k limit - aborting!" && exit 1
  fi

  if [ "$AMD" -eq 1 ] ; then
    cat "$tmpdir/truncated.rom" "$tmpdir/keep_part.rom" > "$OUT_FILE" || exit 1
  else
    cp "$tmpdir/truncated.rom" "$OUT_FILE" || exit 1
  fi
else
  cp "$tmpdir/combined.rom" "$OUT_FILE" || exit 1
fi

# patch end marker in PCIR in out file
echo -n -e '\x00' | dd bs=1 of="$OUT_FILE" seek=$(($GOP_OFFSET + 0x31)) conv=notrunc 2>/dev/null || exit 1

printf "Verifying (starting at 0x%X)...\n" "$GOP_OFFSET"
dd bs=1 if="$OUT_FILE" of="$tmpdir/out_efi_part.rom" skip=$(($GOP_OFFSET)) 2>/dev/null || exit 1
# UEFIRomExtract error messages are on stdout, so we cannot suppress unwanted normal output here
UEFIRomExtract "$tmpdir/out_efi_part.rom" "$tmpdir/extracted.efi" || exit 1
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
  echo " - ${OLD_EFI_COUNT} EFI parts in original ROM, and detected ${NEW_EFI_COUNT} EFI parts in modified ROM, expected $(($OLD_EFI_COUNT + 1))!"
  ERROR=1
fi

if [ "$ERROR" -eq 0 ] ; then
  echo "SUCCESS."
else
  echo "*** WARNING - FAIL ***"
fi

if [ "$TEMP_DIR" = "" ] ; then
  rm -rf "$tmpdir" || exit 1
fi

echo "Done."
