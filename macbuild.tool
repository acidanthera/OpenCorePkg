#!/bin/bash

cd $(dirname "$0")
ARCHS=(X64 IA32)
SELFPKG=OcSupportPkg
DEPNAMES=('EfiPkg')
DEPURLS=('https://github.com/acidanthera/EfiPkg')
DEPBRANCHES=('master')
src=$(/usr/bin/curl -Lfs https://raw.githubusercontent.com/acidanthera/ocbuild/master/efibuild.sh) && eval "$src" || exit 1
