#!/bin/sh

# shim-to-cert.tool - Extract OEM signing certificate public key (and full db, dbx if present) from GRUB shim file.
#
# Copyright (c) 2021, Michael Beaton. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-3-Clause
#

if [ -z "$1" ]; then
    echo "Usage: $0 {shimfile}"
    exit 1
fi

# require binutils and openssl
command -v objcopy >/dev/null 2>&1 || { echo >&2 "objcopy not found - please install binutils package."; exit 1; }
command -v openssl >/dev/null 2>&1 || { echo >&2 "openssl not found - please install openssl package."; exit 1; }

sectfile=$(mktemp) || exit 1

# make certain we have output file name, as objcopy will trash input file without it
if [ "x$sectfile" = "x" ]; then
    echo >&2 "Error creating tempfile!"
    exit 1
fi

# extract .vendor_cert section
objcopy -O binary -j .vendor_cert "$1" "$sectfile" || exit 1

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

dd if="$sectfile" ibs=1 skip="$vendor_authorized_offset" count="$vendor_authorized_size" 2>/dev/null > "$certfile" || { rm "$sectfile"; rm "$certfile"; exit 1; }

# extract dbx
if [ "$vendor_deauthorized_size" -ne "0" ]; then
    dd if="$sectfile" ibs=1 skip="$vendor_deauthorized_offset" count="$vendor_deauthorized_size" 2>/dev/null > "vendor.dbx" || { rm "$sectfile"; rm "$certfile"; exit 1; }
    echo "Secure Boot block list found and saved as vendor.dbx."
fi

rm "$sectfile"

# valid as single cert?
openssl x509 -noout -inform der -in "$certfile" 2>/dev/null

if [ $? -ne 0 ]; then
    # require efitools
    command -v sig-list-to-certs >/dev/null 2>&1 || { echo >&2 "sig-list-to-certs not found - please install efitools package."; rm "$certfile"; exit 1; }

    certsdir=$(mktemp -d) || { rm "$certfile"; exit 1; }

    sig-list-to-certs "$certfile" "${certsdir}/vendor" 1>/dev/null

    if [ $? -ne 0 ]; then
        echo >&2 "ERROR: vendor_authorized contents cannot be processed as cert file or sig list."

        rm -rf "$certsdir"
        rm "$certfile"

        exit 1
    fi

    cp "$certfile" vendor.db
    echo "Secure Boot allow list found and saved as vendor.db - single cert may not be sufficient to start distro."

    # fails when count .der files != 1
    cp "$certsdir"/*.der "$certfile" 2>/dev/null

    if [ $? -ne 0 ]; then
        certcount=$(find "$certsdir" -maxdepth 1 -name "*.der" | wc -l)

        if [ "$certcount" -ne "0" ]; then
            cp "$certsdir"/*.der .

            echo "Extracted multiple signing keys:"
            pwd=$(pwd)
            cd "$certsdir" || { rm -rf "$certsdir"; rm "$certfile"; exit 1; }
            ls -1 ./*.der
            cd "$pwd" || { rm -rf "$certsdir"; rm "$certfile"; exit 1; }
        fi

        rm -rf "$certsdir"
        rm "$certfile"

        exit 0
    fi

    rm -rf "$certsdir"
fi

# outfile name from cert CN
certname=$(openssl x509 -noout -subject -inform der -in "$certfile" | sed 's/^subject=.*CN *=[ \"]*//' | sed 's/[,\/].*//' | sed 's/ *//g') || { rm "$certfile"; exit 1; }
outfile="${certname}.pem"

openssl x509 -inform der -in "$certfile" -out "$outfile" || { rm "$certfile"; exit 1; }

rm "$certfile"

echo "Certificate extracted as ${outfile}."
