#!/bin/bash
source edksetup.sh
build -a X64 -b RELEASE -t XCODE5 -p OcSupportPkg/OcSupportPkg.dsc

