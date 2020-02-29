#!/bin/bash

cd $(dirname "$0")
ARCHS=(X64 IA32)
SELFPKG=OcSupportPkg
DEPNAMES=('EfiPkg')
DEPURLS=('https://github.com/acidanthera/EfiPkg')
DEPBRANCHES=('master')
src=$(/usr/bin/curl -Lfs https://raw.githubusercontent.com/acidanthera/ocbuild/master/efibuild.sh) && eval "$src" || exit 1

UTILS=(
  "AppleEfiSignTool"
  "EfiResTool"
  "readlabel"
  "RsaTool"
)

cd Utilities || exit 1
for util in "${UTILS[@]}"; do
  cd "$util" || exit 1
  make || exit 1
  cd - || exit 1
done

cd ../Library/OcConfigurationLib || exit 1
./CheckSchema.py OcConfigurationLib.c || exit 1
