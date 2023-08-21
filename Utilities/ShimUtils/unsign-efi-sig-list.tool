#!/bin/bash

#
# unsign-efi-sig-list.tool
#    Removes signing key from signed certificate list,
#    also saves signing section and prints CNs found in,
#    thereby helping to indicate who signed it originally.
#
# Copyright (c) 2023, Michael Beaton. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-3-Clause
#

if [ $# -lt 1 ] || [ $# -gt 3 ] ; then
    echo "Usage:"
    echo "    $(basename "$0") <signed-sig-list> [<outfile> [<sigfile>]]"
    echo ""
    echo " - Uses infile name with .unsigned appended if outfile name not specified"
    echo " - Uses infile name with .pkcs7 appended if sigfile name not specified"
    echo ""
    exit 0
fi

if [ ! -f "$1" ] ; then
    echo "File not found: $1"
    exit 1
fi

infile="$1"
shift

if [ "$1" != "" ] ; then
    outfile="$1"
    shift
else
    outfile="${infile}.unsigned"
fi

EfiTimeSize=16
WinCertHdrSize=8
GuidSize=16
ExpectedRevision=$((0x0200))
ExpectedCertificateType=$((0x0EF1))

WinCertificateSize=$(dd if="$infile" ibs=1 skip="$EfiTimeSize" count=4 2>/dev/null | od -t u4 -An | xargs)
Revision=$(dd if="$infile" ibs=1 skip="$(($EfiTimeSize + 4))" count=2 2>/dev/null | od -t u4 -An | xargs)
CertificateType=$(dd if="$infile" ibs=1 skip="$(($EfiTimeSize + 6))" count=2 2>/dev/null | od -t u4 -An | xargs)

if [ "$CertificateType" -ne "$ExpectedCertificateType" ] ; then
    printf "Not a signed certificate file (signature 0x%04X found, 0x%04X expected) in: %s\n" "$CertificateType" "$ExpectedCertificateType" "$infile"
    exit 1
fi

if [ "$Revision" -ne "$ExpectedRevision" ] ; then
    printf "Unexpected revision (0x%04X found, 0x%04X expected) in signed certificate file: %s\n" "$Revision" "$ExpectedRevision" "$infile"
    exit 1
fi

dd if="$infile" of="$outfile" ibs=1 skip=$(($EfiTimeSize + $WinCertificateSize)) 2>/dev/null

echo "Unsigned certificate list saved to '$outfile'"

# Total size is header+guid+sig
if [ "$WinCertificateSize" -le "$(($WinCertHdrSize + $GuidSize))" ] ; then
    echo "!!! Certificate size smaller than expected header size"
    exit 1
fi

# In little-endian byte order
EFI_CERT_TYPE_PKCS7_GUID="9dd2af4adf68ee498aa9347d375665a7"

# Expect pkcs7 sig (diff piping requires bash not sh)
if ! diff <(dd if="$infile" ibs=1 skip=$(($EfiTimeSize + $WinCertHdrSize)) count=$(($GuidSize)) 2>/dev/null | xxd -p) <(echo $EFI_CERT_TYPE_PKCS7_GUID) 1>/dev/null ; then
    echo "!!! EFI_CERT_TYPE_PKCS7_GUID not found"
    ext=".sig"
else
    ext=".pkcs7"
fi

if [ "$1" != "" ] ; then
    sigfile="$1"
    shift
else
    sigfile="${infile}${ext}"
fi

dd if="$infile" of="$sigfile" ibs=1 skip=$(($EfiTimeSize + $WinCertHdrSize + $GuidSize)) count=$(($WinCertificateSize - $WinCertHdrSize - $GuidSize)) 2>/dev/null

echo "Signing data saved to '$sigfile'"

# TODO: Figure out which openssl command parses this file at the correct level
echo "Signing CNs:"
openssl asn1parse -inform der -in "$sigfile" | grep -A1 commonName | grep STRING
