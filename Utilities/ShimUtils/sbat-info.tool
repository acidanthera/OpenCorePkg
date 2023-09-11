#!/bin/bash

# sbat-info.tool - Dump SBAT information from .efi executable, with additional
# version and SBAT enforcement level information if the executable is Shim.
#
# Copyright (c) 2023, Michael Beaton. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-3-Clause
#

LIGHT_GREEN='\033[1;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Note: binutils can simply be placed last on the path in macOS, to provide objcopy but avoid hiding native objdump (this script parses LLVM or GNU objdump output).
# Alternatively BINUTILS_PREFIX environment variable can be set to enabled prefixed co-existing GNU tools.
command -v "${BINUTILS_PREFIX}"objcopy >/dev/null 2>&1 || { echo >&2 "objcopy not found - please install binutils package or set BINUTILS_PREFIX environment variable."; exit 1; }

usage () {
    echo "Usage: $(basename "$0") <efifile>"
    echo "  Display SBAT information, if present, for any .efi executable."
    echo "  If the file is a version of Shim, also dislay Shim version info and SBAT enforcement level."
}

has_section () {
    "${BINUTILS_PREFIX}"objdump -h "$1" | sed -nr 's/^ *[0-9]+ ([^ ]+).*/\1/p' | grep "$2" 1>/dev/null
}

if [ -z "$1" ]; then
    usage
    exit 1
fi

# https://unix.stackexchange.com/a/84980/340732
op_dir=$(mktemp -d 2>/dev/null || mktemp -d -t 'shim_info') || exit 1

# objcopy and objdump do not like writing to stdout when part of a pipe, so just write to temp files
# Note: these get created as empty files even if the section does not exist
"${BINUTILS_PREFIX}"objcopy -O binary -j ".sbat" "$1" "${op_dir}/sbat" || { rm -rf "${op_dir}"; exit 1; }
"${BINUTILS_PREFIX}"objcopy -O binary -j ".sbatlevel" "$1" "${op_dir}/sbatlevel" || { rm -rf "${op_dir}"; exit 1; }
"${BINUTILS_PREFIX}"objcopy -O binary -j ".data.ident" "$1" "${op_dir}/data.ident" || { rm -rf "${op_dir}"; exit 1; }

if ! has_section "$1" ".data.ident" ; then
    IS_SHIM=0
    echo -e "${YELLOW}Shim version info not present (probably not Shim)${NC}"
else
    # Treat this section's presence as enough for the warning colours below
    IS_SHIM=1
    FILE_HEADER="$(head -n 1 "${op_dir}/data.ident")"
    SHIM_HEADER="UEFI SHIM"
    if [ ! "$FILE_HEADER" = "$SHIM_HEADER" ] ; then
        echo -e "${RED}!!!Version header found '${FILE_HEADER}' expected '${SHIM_HEADER}'${NC}"
    else
        echo -e "${YELLOW}Found ${SHIM_HEADER} version header${NC}"
    fi

    # shellcheck disable=SC2016
    VERSION="$(grep -a 'Version' "${op_dir}/data.ident" | sed -n 's/^\$Version: *\([^[:space:]]*\) *\$$/\1/p')"
    if [ "$VERSION" = "" ] ; then
        echo -e "${RED}Expected version number not present${NC}"
    else
        echo -e "Version: ${LIGHT_GREEN}${VERSION}${NC} (${VERSION}+, if user build)"
    fi

    # shellcheck disable=SC2016
    COMMIT="$(grep -a 'Commit' "${op_dir}/data.ident" | sed -n 's/^\$Commit: *\([^[:space:]]*\) *\$$/\1/p')"
    if [ "$COMMIT" = "" ] ; then
        echo -e "${RED}Expected commit id not present${NC}"
    else
        echo "Commit: ${COMMIT}"
    fi
fi

echo
echo -e "${YELLOW}SBAT (when tested by other files or by self):${NC}"

if ! has_section "$1" ".sbat" ; then
    echo
    echo -e "${RED}No .sbat section${NC}"
else
    echo
    echo -n -e "$LIGHT_GREEN"
    cat "${op_dir}/sbat"
    echo -n -e "$NC"
    if ! cat "${op_dir}/sbat" | grep -E "[a-z]|[A-Z]" 1>/dev/null ; then
        # Red for shim; yellow for other file
        if [ "$IS_SHIM" -eq 1 ] ; then
            echo -n -e "$RED"
        else
            echo -n -e "$LIGHT_GREEN"
        fi
        echo -e "Present but empty .sbat section"
    fi
fi

echo
echo -e "${YELLOW}SBAT Level (when testing other files):${NC}"

if ! has_section "$1" ".sbatlevel" ; then
    echo
    # Yellow for shim (not present in earlier shims with .sbat, ~15.6); green for other file
    if [ "$IS_SHIM" -eq 1 ] ; then
        echo -n -e "$YELLOW"
    else
        echo -n -e "$LIGHT_GREEN"
    fi
    echo -n "No .sbatlevel section"
    echo -e "$NC"
else
    sbat_level_version=$(dd if="${op_dir}/sbatlevel" ibs=1 skip=0 count=4 2>/dev/null | od -t u4 -An | xargs) || { rm -rf "${op_dir}"; exit 1; }
    sbat_level_previous=$(dd if="${op_dir}/sbatlevel" ibs=1 skip=4 count=4 2>/dev/null | od -t u4 -An | xargs) || { rm -rf "${op_dir}"; exit 1; }
    sbat_level_latest=$(dd if="${op_dir}/sbatlevel" ibs=1 skip=8 count=4 2>/dev/null | od -t u4 -An | xargs) || { rm -rf "${op_dir}"; exit 1; }

    if [ "$sbat_level_version" -ne 0 ] ; then
        echo -e "${RED}!!! Unexpected .sbatlevel version $sbat_level_version != 0 !!!${NC}"
    fi

    echo
    echo -e "${YELLOW}Previous (more permissive boot, default):${NC}"
    echo -n -e "$LIGHT_GREEN"
    dd if="${op_dir}/sbatlevel" ibs=1 skip="$(($sbat_level_previous + 4))" count="$(($sbat_level_latest - $sbat_level_previous))" 2>/dev/null
    echo -e "$NC"

    echo -e "${YELLOW}Latest (stricter boot, optional):${NC}"
    echo -n -e "$LIGHT_GREEN"
    dd if="${op_dir}/sbatlevel" ibs=1 skip="$(($sbat_level_latest + 4))" 2>/dev/null
    echo -e "$NC"
fi

rm -rf "${op_dir}"

exit 0
