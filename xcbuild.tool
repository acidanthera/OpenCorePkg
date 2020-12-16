#!/bin/bash
CONFIGURATION=DEBUG

source edksetup.sh
build -a X64 -p OpenCorePkg/OpenCorePkg.dsc -t XCODE5 -b $CONFIGURATION
