#!/bin/bash
#
# Copyright (c) 2023 Mike Beaton. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#
# Makes user build of shim which launches OpenCore.efi and includes specified vendor certificate lists.
# Builds on macOS using Ubuntu multipass or on Linux directly (Ubuntu & Fedora tested).
#
# To debug shim (e.g. within OVMF) make with
# ./shim-make.tool make OPTIMIZATIONS="-O0"
# and set 605dab50-e046-4300-abb6-3dd810dd8b23:SHIM_DEBUG to (UINT8)1 (<data>AQ==</data>) in NVRAM
#

ROOT=~/shim_root
SHIM=~/shim_source
OC_SHIM=oc-shim

unamer() {
  NAME="$(uname)"

  if [ "$(echo "${NAME}" | grep MINGW)" != "" ] || [ "$(echo "${NAME}" | grep MSYS)" != "" ]; then
    echo "Windows"
  else
    echo "${NAME}"
  fi
}

shim_command () {
    DIR=$1
    shift
    if [ $DARWIN -eq 1 ] ; then
        if [ $ECHO -eq 1 ] ; then
            # shellcheck disable=SC2068
            echo multipass exec ${OC_SHIM} --working-directory "'${DIR}'" -- $@ 1>/dev/stderr
        fi
        # shellcheck disable=SC2068,SC2294
        eval multipass exec ${OC_SHIM} --working-directory "'${DIR}'" -- $@
    else
        if [ $ECHO -eq 1 ] ; then
            # shellcheck disable=SC2068
            echo "[${DIR}]" $@ 1>/dev/stderr
        fi
        pushd "${DIR}" 1>/dev/null || exit 1
        # shellcheck disable=SC2068,SC2294
        eval $@
        retval=$?
        popd 1>/dev/null || exit 1
        return $retval
    fi
}

shim_command_v () {
    FOUND="$(shim_command . command -v "'${1}'" | wc -l)"
    return $((1 - $FOUND))
}

usage() {
    echo "Usage:"
    echo -n " ./${SELFNAME} [args] [setup|clean|make [options]|install [esp-root-path]"
    if [ $DARWIN -eq 1 ] ; then
        echo -n "|mount [multipass-path]]"
    fi
    echo "]"
    echo
    echo "Args:"
    echo -n "  -r : Specify shim output root, default '${ROOT}'"
    if [ $DARWIN -eq 1 ] ; then
        echo -n " (shared)"
    fi
    echo
    echo -n "  -s : Specify shim source location, default '${SHIM}'"
    if [ $DARWIN -eq 1 ] ; then
        echo -n " (VM only, unless 'shim-make mount' used for development/debugging)"
    fi
    echo
    echo "If used, -r/-s:"
    echo " - Should be specified on every call, they are not remembered from 'setup'"
    echo " - Should come before the chosen command"
    echo
    echo "Examples:"
    echo "  ./${SELFNAME} setup (sets up directories and installs rhboot/shim source)"
    echo "  ./${SELFNAME} clean (cleans after previous make)"
    LOCATION="."
    if [ $DARWIN -eq 1 ] ; then
        LOCATION="${ROOT}"
    fi
    echo -n "  ./${SELFNAME} make VENDOR_DB_FILE='${LOCATION}/combined/vendor.db' VENDOR_DBX_FILE='${LOCATION}/combined/vendor.dbx' (makes shim with db and dbx contents"
    if [ $DARWIN -eq 1 ] ; then
        echo -n "; note VENDOR_DB_FILE and VENDOR_DBX_FILE are inside a directory shared with VM"
    fi
    echo ")"
    echo "  ./${SELFNAME} install '${EXAMPLE_ESP}' (installs made files to ESP mounted at '${EXAMPLE_ESP}')"
    echo
    echo "After installation shimx64.efi and mmx64.efi must be signed by user ISK; OpenCore.efi must have an .sbat section added and be signed by user ISK."
}

mount_path () {
    SRC=$1
    if [ "${SRC}" = "" ] ; then
        echo "Incorrect call to mount_path!"
        exit 1
    fi
    DEST=$2
    if [ "${DEST}" = "" ] ; then
        DEST=${SRC}
    fi

    if [ ! -d "${SRC}" ] ; then
        echo "Adding ${SRC}..."
        mkdir -p "${SRC}" || exit 1
    else
        echo "${SRC} already present..."
    fi

    if [ $DARWIN -eq 1 ] ; then
        if ! multipass info ${OC_SHIM} | grep "${DEST}" 1>/dev/null ; then
            echo "Mounting ${SRC}..."
            multipass mount "${SRC}" "${OC_SHIM}:${DEST}" || exit 1
        else
            echo "${SRC} already mounted..."
        fi
    fi
}

get_ready () {
    if [ $DARWIN -eq 1 ] ; then
        if ! command -v multipass 1>/dev/null ; then
            echo "Installing Ubuntu multipass..."
            brew install --cask multipass || exit 1
        else
            echo "Ubuntu multipass already installed..."
        fi

        if ! multipass info ${OC_SHIM} &>/dev/null ; then
            echo "Launching ${OC_SHIM} multipass instance..."
            multipass launch -n ${OC_SHIM} || exit 1
        else
            echo "${OC_SHIM} multipass instance already launched..."
        fi
    fi

    mount_path "${ROOT}"

    if ! shim_command_v gcc || ! shim_command_v make || ! shim_command_v git || ! shim_command_v sbsign || ! shim_command_v sbsiglist ; then
        echo "Installing dependencies..."
        if shim_command_v dnf ; then
            shim_command . sudo dnf install -y gcc make git elfutils-libelf-devel sbsigntools efitools || exit 1
        else
            shim_command . sudo apt-get update || exit 1
            shim_command . sudo apt install -y gcc make git libelf-dev sbsigntool efitools || exit 1
        fi
    else
        echo "Dependencies already installed..."
    fi

    #
    # For debug/develop purposes on Darwin it would be nicer to keep the source code in
    # macOS, just mounted and built in multipass, but the build is about 1/3 the speed of
    # building in a native multipass directory.
    # For the purposes of having a fast build but code which can be opened e.g.
    # within an IDE within macOS, sshfs can be used to mount out from multipass
    # to macOS using ./shim-make mount:
    #  - https://github.com/canonical/multipass/issues/1070
    #  - https://osxfuse.github.io/
    #
    if ! shim_command . test -d "'${SHIM}'"  ; then
        # TODO: Once there is a stable version of Shim containing the commits we need
        #  - 1f38cb30a5e1dcea97b8d48915515b866ec13c32
        #  - a8b0b600ddcf02605da8582b4eac1932a3bb13fa
        # (i.e. 15.8. hopefully) we should probably check out at a fixed, stable tag, starting
        # at 15.8 and then manually updated in this script when a new stable version is released,
        # but which can be overridden with any commit or branch with a script option.
        echo "Cloning rhboot/shim..."
        shim_command . git clone https://github.com/rhboot/shim.git "'${SHIM}'" || exit 1
        shim_command "${SHIM}" git submodule update --init || exit 1
    else
        if ! shim_command "${SHIM}" git remote -v | grep "rhboot/shim" 1>/dev/null ; then
            echo "FATAL: VM subdirectory ${SHIM} is already present, but does not contain rhboot/shim!"
            exit 1
        fi
        echo "rhboot/shim already cloned..."
    fi

    echo "Make.defaults:"

    # These two modifications to Make.defaults only required for debugging
    FOUND=$(shim_command "${SHIM}" grep "gdwarf" Make.defaults | wc -l)
    if [ "$FOUND" -eq 0 ] ; then
        echo "  Updating gdwarf flags..."
        shim_command "${SHIM}" sed -i "'s^-ggdb \\\\^-ggdb -gdwarf-4 -gstrict-dwarf \\\\^g'" Make.defaults
    else
        echo "  gdwarf flags already updated..."
    fi

    FOUND=$(shim_command "${SHIM}" grep "'${ROOT}'" Make.defaults | wc -l)
    if [ "$FOUND" -eq 0 ] ; then
        echo "  Updating debug directory..."
        shim_command "${SHIM}" sed -i "\"s^-DDEBUGDIR='L\\\"/usr/lib/debug/usr/share/shim/\\\$(ARCH_SUFFIX)-\\\$(VERSION)\\\$(DASHRELEASE)/\\\"'^-DDEBUGDIR='L\\\"${ROOT}/usr/lib/debug/boot/efi/EFI/OC/\\\"'^g\"" Make.defaults
    else
        echo "  Debug directory already updated..."
    fi

    # Work-around for https://github.com/rhboot/shim/issues/596
    FOUND=$(shim_command "${SHIM}" grep "'export DEFINES'" Make.defaults | wc -l)
    if [ "$FOUND" -eq 0 ] ; then
        echo "  Updating exports..."

        # var assignment to make output piping work normally
        # shellcheck disable=SC2034
        temp=$(echo "export DEFINES" | shim_command "${SHIM}" tee -a Make.defaults) 1>/dev/null
    else
        echo "  Exports already updated..."
    fi
}

SELFNAME="$(/usr/bin/basename "${0}")"

ECHO=0

if [ "$(unamer)" = "Darwin" ] ; then
    DARWIN=1
    EXAMPLE_ESP='/Volumes/EFI'
else
    DARWIN=0
    EXAMPLE_ESP='/boot/efi'
fi

OPTS=0
while [ "${1:0:1}" = "-" ] ; do
    OPTS=1
    if [ "$1" = "-r" ] ; then
        shift
        if [ "$1" != "" ] && ! [ "${1:0:1}" = "-" ] ; then
            ROOT=$1
            shift
        else
            echo "No root directory specified" && exit 1
        fi
    elif [ "$1" = "-s" ] ; then
        shift
        if [ "$1" != "" ] && ! [ "${1:0:1}" = "-" ] ; then
            SHIM=$1
            shift
        else
            echo "No shim directory specified" && exit 1
        fi
    elif [ "$1" = "--echo" ] ; then
        ECHO=1
        shift
    else
        echo "Unknown option: $1"
        exit 1
    fi
done

if [ "$1" = "setup" ] ; then
    get_ready
    echo "Installation complete."
    exit 0
elif [ "$1" = "clean" ] ; then
    echo "Cleaning..."
    shim_command "${SHIM}" make clean
    exit 0
elif [ "$1" = "make" ] ; then
    echo "Making..."
    shift
    shim_command "${SHIM}" make DEFAULT_LOADER="\\\\\\\\OpenCore.efi" OVERRIDE_SECURITY_POLICY=1 "$@"
    exit 0
elif [ "$1" = "install" ] ; then
    echo "Installing..."
    rm -rf "${ROOT:?}"/usr
    shim_command "${SHIM}" DESTDIR="'${ROOT}'" EFIDIR='OC' OSLABEL='OpenCore' make install
    if [ ! "$2" = "" ] ; then
        echo "Installing to ESP ${2}..."
        cp "${ROOT}"/boot/efi/EFI/OC/* "${2}"/EFI/OC || exit 1
    fi
    exit 0
elif [ $DARWIN -eq 1 ] && [ "$1" = "mount" ] ; then
    MOUNT="$2"
    if [ "${MOUNT}" = "" ] ; then
        MOUNT=$SHIM
    fi

    #
    # Useful for devel/debug only.
    # Note: We are only mounting in the reverse direction because we get much faster build speeds.
    #
    if ! command -v sshfs 1>/dev/null ; then
        echo "sshfs (https://osxfuse.github.io/) is required for mounting directories from multipass into macOS (https://github.com/canonical/multipass/issues/1070)"
        exit 1
    fi

    if [ ! -d "${MOUNT}" ] ; then
        echo "Making local directory ${MOUNT}..."
        mkdir -p "${MOUNT}" || exit 1
    fi

    ls "${MOUNT}" 1>/dev/null
    if [ $? -ne 0 ] ; then
        echo "Directory may be mounted but not ready (no authorized key?)"
        echo "Try: umount ${MOUNT}"
        exit 1
    fi

    if mount | grep ":${MOUNT}" ; then
        echo "Already mounted at ${MOUNT}"
        exit 0
    fi

    # shellcheck disable=SC2012
    if [ "$(ls -A -1 "${MOUNT}" | wc -l)" -ne 0 ] ; then
        echo "Directory ${MOUNT} is not empty!"
        exit 1
    fi

    IP=$(multipass info ${OC_SHIM} | grep IPv4 | cut -d ":" -f 2 | sed 's/ //g')
    if [ "${IP}" = "" ] ; then
        echo "Cannot obtain IPv4 for ${OC_SHIM}"
        exit 1
    fi
    if sshfs "ubuntu@${IP}:$(realpath "${MOUNT}")" "${MOUNT}" ; then
        echo "Mounted at ${MOUNT}"
        exit 0
    else
        umount "${MOUNT}"
        echo "Directory cannot be mounted, append your ssh public key to .ssh/authorized_keys in the VM and try again."
        exit 1
    fi
elif [ "$1" = "" ] ; then
    if [ $OPTS -eq 0 ] ; then
        usage
    else
        echo "No command specified."
    fi
    exit 1
else
    echo "Unkown command: $1"
    exit 1
fi
