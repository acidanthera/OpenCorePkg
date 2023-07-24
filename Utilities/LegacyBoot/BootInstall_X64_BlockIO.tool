#!/bin/bash

cd "$(dirname "$0")" || exit 1
export ARCHS=X64
export DUET_SUFFIX="-blockio"
source BootInstallBase.sh
