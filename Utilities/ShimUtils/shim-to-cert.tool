#!/bin/bash

# shim-to-cert.tool - Extract OEM signing certificate public key (and full db, dbx if present) from GRUB shim file.
#
# Copyright (c) 2021-2023, Michael Beaton. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-3-Clause
#

LIGHT_GREEN='\033[1;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Note: binutils can be placed last on path in macOS, to provide objcopy but avoid overwriting native tools.
command -v "${BINUTILS_PREFIX}"objcopy >/dev/null 2>&1 || { echo >&2 "objcopy not found - please install binutils package or set BINUTILS_PREFIX environment variable."; exit 1; }
command -v openssl >/dev/null 2>&1 || { echo >&2 "openssl not found - please install openssl package."; exit 1; }

if [ -z "$1" ]; then
    echo "Usage: $(basename "$0") <shimfile>"
    exit 1
fi

sectfile=$(mktemp) || exit 1

# make certain we have output file name, as objcopy will trash input file without it
if [ "x$sectfile" = "x" ]; then
    echo >&2 "Error creating tempfile!"
    exit 1
fi

# extract .vendor_cert section
"${BINUTILS_PREFIX}"objcopy -O binary -j .vendor_cert "$1" "$sectfile" || exit 1

if [ ! -s "$sectfile" ] ; then
    echo >&2 "No .vendor_cert section in $1."
    rm "$sectfile"
    exit 1
fi

# xargs trims white space
vendor_authorized_size=$(dd if="$sectfile" ibs=1 skip=0 count=4 2>/dev/null | od -t u4 -An | xargs) || { rm "$sectfile"; exit 1; }
vendor_deauthorized_size=$(dd if="$sectfile" ibs=1 skip=4 count=4 2>/dev/null | od -t u4 -An | xargs) || { rm "$sectfile"; exit 1; }
vendor_authorized_offset=$(dd if="$sectfile" ibs=1 skip=8 count=4 2>/dev/null | od -t u4 -An | xargs) || { rm "$sectfile"; exit 1; }
vendor_deauthorized_offset=$(dd if="$sectfile" ibs=1 skip=12 count=4 2>/dev/null | od -t u4 -An | xargs) || { rm "$sectfile"; exit 1; }

# extract cert or db
certfile=$(mktemp) || { rm "$sectfile"; exit 1; }

# extract db
if [ "$vendor_authorized_size" -ne "0" ]; then
    dd if="$sectfile" ibs=1 skip="$vendor_authorized_offset" count="$vendor_authorized_size" 2>/dev/null > "$certfile" || { rm "$sectfile"; rm "$certfile"; exit 1; }
fi

# extract dbx
if [ "$vendor_deauthorized_size" -ne "0" ]; then
    dd if="$sectfile" ibs=1 skip="$vendor_deauthorized_offset" count="$vendor_deauthorized_size" 2>/dev/null > "vendor.dbx" || { rm "$sectfile"; rm "$certfile"; exit 1; }
    echo -e " - Secure Boot revocation list saved as ${LIGHT_GREEN}vendor.dbx${NC}"
else
    echo " - No secure Boot revocation list"
fi

rm "$sectfile"

if [ "$vendor_authorized_size" -eq "0" ]; then
    echo "!!! Empty vendor_authorized section (no secure boot signing certificates present)."
    rm "$certfile"
    exit 1
fi

# valid as single cert?
openssl x509 -noout -inform der -in "$certfile" 2>/dev/null

if [ $? -ne 0 ]; then
    cp "$certfile" vendor.db
    echo -e " - Secure Boot signing list saved as ${LIGHT_GREEN}vendor.db${NC}"
else
    # outfile name from cert CN
    certname=$(openssl x509 -noout -subject -inform der -in "$certfile" | sed 's/^subject=.*CN *=[ \"]*//' | sed 's/[,\/].*//' | sed 's/ *//g') || { rm "$certfile"; exit 1; }
    outfile="${certname}.der"

    cp "$certfile" "$outfile" || { rm "$certfile"; exit 1; }

    echo -e " - Secure Boot certificate saved as ${LIGHT_GREEN}${outfile}${NC}"

    if ! command -v cert-to-efi-sig-list >/dev/null ; then
        echo -e "   o To convert certificate to vendor.db use:"
        echo -e "    ${YELLOW}" 'cert-to-efi-sig-list <(openssl x509 -in' "'${outfile}'" '-outform PEM) vendor.db' "${NC}"
    else
        cert-to-efi-sig-list <(openssl x509 -in "${outfile}" -outform PEM) vendor.db || { rm "$certfile"; exit 1; }
        echo -e "   o Certificate has also been saved as single cert signing list ${LIGHT_GREEN}vendor.db${NC}"
    fi
fi

rm "$certfile"
