#!/bin/bash

cd "$(dirname "$0")" || exit 1
export ARCHS=IA32
export DUET_SUFFIX="-blockio"
source BootInstallBase.sh
